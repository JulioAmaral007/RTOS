# Prompt — Etapa 4: API de PWM para o RTOS (PIC18F46K22)

Atue como engenheiro embarcado especialista em periféricos do PIC18F.

**Projeto alvo:** `RTOS/` — é este o projeto a ser entregue no t1. Os arquivos `RTOS/io.c` e `RTOS/io.h` estão **vazios** e é onde a API de PWM deve ser criada.

**Referência técnica:** `RTOS_V25.X/io.c` e `RTOS_V25.X/io.h` — implementação de outra disciplina que já fez PWM no PIC18F46K22. Use **apenas** como base do padrão de código. **Não porte** lógica de aplicação do `RTOS_V25.X` (ex.: injeção eletrônica, freio, estabilidade) — isso **não faz parte** do t1.

---

## Escopo exato pedido pelo `t1.pdf`

> "Implementação de uma API de E/S para manipular os seguintes periféricos: PWM, ADC e interrupção externa."
> "API de E/S (... geração de sinal PWM)".

Nada mais. Apenas uma API simples que gera PWM e permite alterar o duty cycle.

---

## Leitura obrigatória antes de implementar

1. `RTOS/io.c` e `RTOS/io.h` — **vazios**, serão preenchidos por você.
2. `RTOS/os_config.h` — constantes do SO. `_XTAL_FREQ` **não** está definido neste projeto; se a HAL precisar do valor para `__delay_*`, defina-o aqui (1 linha).
3. `RTOS/hw.c` — Timer0 é usado pelo scheduler tick. **Não alterar**. Timer2 (que o PWM usa) é independente.
4. `RTOS_V25.X/io.c` — referência de `pwm_init` e `pwm_set_duty`. Mesmo hardware (PIC18F46K22), mesmo módulo CCP1/Timer2.

---

## Contexto de hardware — PIC18F46K22

PWM gerado pelo módulo **CCP1** em conjunto com o **Timer2**:
- Saída: **RC2** (CCP1).
- Período: `PR2` (8 bits) + prescaler do Timer2.
- Duty de 10 bits: `CCPR1L` (8 MSBs) + `CCP1CONbits.DC1B` (2 LSBs).

### Registradores envolvidos

| Registrador | Função |
|---|---|
| `TRISCbits.TRISC2` | Pino RC2 como saída. |
| `PR2` | Período do PWM (0–255). |
| `T2CON` | Liga Timer2 e define prescaler (1:1, 1:4, 1:16). |
| `CCP1CON` | Modo do CCP1 (`0b00001100` = PWM). |
| `CCPR1L` | 8 MSBs do duty. |
| `CCP1CONbits.DC1B` | 2 LSBs do duty. |

### Fórmulas

Período:
```
PR2 = (Fosc / (4 · prescaler_T2 · f_pwm)) − 1
```

Com prescaler 1:1 e `PR2 = 0xFF` (255), a frequência do PWM é aproximadamente `Fosc / 1024`. Se `_XTAL_FREQ` = 48 MHz → ~46,9 kHz; se 16 MHz → ~15,6 kHz. Documente a escolha.

Duty (valor raw de 10 bits):
```
duty_max         = 4 · (PR2 + 1)        // 1024 quando PR2 = 0xFF
CCPR1L           = (duty >> 2)
CCP1CONbits.DC1B = (duty & 0b11)
```

### Sequência de inicialização (datasheet PIC18F46K22)

1. `TRISCbits.TRISC2 = 0`.
2. `CCP1CON = 0b00001100` (modo PWM).
3. `T2CON   = 0b00000100` (Timer2 ligado, prescaler 1:1).
4. `PR2     = 0xFF`.
5. Duty inicial em `CCPR1L` + `CCP1CONbits.DC1B`.

---

## API a implementar em `RTOS/io.c` e `RTOS/io.h`

Seguir exatamente a assinatura da referência em `RTOS_V25.X/io.c`:

```c
void pwm_init(uint8_t channel);
void pwm_set_duty(uint8_t channel, uint16_t duty);
```

- `channel` recebido para permitir expansão futura a CCP2, mas implementar **apenas `channel == 1`** (CCP1/RC2). Canais diferentes ficam como *no-op* — comentar no código.
- `duty` é valor **raw** de 0 a `4 * (PR2 + 1)`. A função deve fazer *clamp* ao máximo antes de gravar em `CCPR1L` / `CCP1CONbits.DC1B`.
- **Não** implemente `pwm_start`/`pwm_stop`. Depois de `pwm_init`, o sinal é contínuo. Para "parar", chame `pwm_set_duty(1, 0)`.

`io.h` deve declarar apenas os dois protótipos acima e os *includes* mínimos (`<xc.h>`, `<stdint.h>`).

---

## Integração com o RTOS (`RTOS/`)

- **Não** alterar `RTOS/hw.c`, `RTOS/kernel.c`, `RTOS/scheduler.c`. Timer0 (scheduler) e Timer2 (PWM) são independentes no PIC18F46K22.
- A API deve ser chamável de qualquer task sem abrir janela de corrida — ela só escreve em registradores de hardware. Se a aplicação precisar proteger contra chamadas concorrentes de múltiplas tasks (ex.: duas tasks mudando o duty do mesmo canal), use `mutex_t` em `RTOS/sync.h` **na aplicação** (`user.c`), não dentro da HAL.
- Após `pwm_init` + `pwm_set_duty`, **não** faça busy-wait em task para "manter" o PWM — o hardware já gera o sinal sozinho.

---

## Regras

- HAL **consolidada** em `RTOS/io.c`/`io.h` — **não** criar `pwm.c`/`pwm.h` separados.
- Sem abstrações não pedidas pelo t1 (nada de frequência parametrizável, múltiplos canais ativos simultaneamente, etc.). Mantenha o mínimo.
- Nenhuma função que dependa da aplicação `RTOS_V25.X` (ex.: variável global `freio_acionado`, `tempo_injecao`) pode aparecer — isso é de outro projeto.
- Comentar no código (`io.c`) o valor de `PR2`, o prescaler e a frequência resultante para o `_XTAL_FREQ` escolhido.

---

## Entrega

- `RTOS/io.c` e `RTOS/io.h` com `pwm_init(uint8_t)` e `pwm_set_duty(uint8_t, uint16_t)` funcionando.
- Definição de `_XTAL_FREQ` adicionada ao projeto (em `os_config.h` ou no topo de `io.c`), com o valor escolhido documentado.
- Cálculo explícito de `PR2`, prescaler e frequência resultante.
- Exemplo mínimo em uma task de `RTOS/user.c` chamando `pwm_init(1)` e variando o duty com `pwm_set_duty(1, valor)` — sem copiar a lógica de "injeção eletrônica" do `RTOS_V25.X`.
- Nota de 2–3 linhas explicando por que Timer2 não interfere no Timer0 do scheduler.
