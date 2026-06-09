# Prompt — Etapa 5: API de ADC para o RTOS (PIC18F46K22)

Atue como engenheiro de firmware embarcado especialista em PIC18F.

**Projeto alvo:** `RTOS/` — é este o projeto a ser entregue no t1. Os arquivos `RTOS/io.c` e `RTOS/io.h` estão **vazios** (ou contendo apenas o PWM da etapa 4) e é onde a API de ADC deve ser adicionada.

**Referências técnicas:**
- `API_ADC.X/io.c`, `API_ADC.X/io.h`, `API_ADC.X/main.c` — projeto isolado que já lê ADC no PIC18F46K22. Base para entender o módulo ADC do chip e a conversão raw → unidade física (exemplo: temperatura a partir da leitura).
- `RTOS_V25.X/io.c` — versão já integrada ao RTOS (outra disciplina). Base para ver como a API `adc_init` + `adc_read(channel)` encaixa no RTOS.

Use **apenas** para padrão de código. **Não** porte código de aplicação (ex.: `tarefa_acelerador`, média móvel para controle de pedal, etc.) — isso não está no t1.

---

## Escopo exato pedido pelo `t1.pdf`

> "API de E/S (leitura do periférico de ADC ...)"

Apenas inicializar o ADC e ler um canal analógico. Sem filtro, sem DMA, sem callback, sem fila de amostras.

---

## Leitura obrigatória antes de implementar

1. `RTOS/io.c`, `RTOS/io.h` — destino da implementação.
2. `RTOS/os_config.h` — se `_XTAL_FREQ` ainda não foi definido na etapa 4, adicione-o (impacta a escolha de `ADCS`).
3. `API_ADC.X/io.c` — referência canônica. Note o uso de `ANSELAbits.ANSA0 = 1` (obrigatório) e `ADCON0bits.GODONE` como alias de `GO`.
4. `RTOS_V25.X/io.c` — referência de como a função ficou com `channel` parametrizável. **Atenção:** essa referência **omite** `ANSELA` — isso é bug; não reproduzir.

---

## Hardware — ADC do PIC18F46K22

### Registradores principais

| Registrador | Função |
|---|---|
| `ADCON0` | `ADON` (liga), `CHS<4:0>` (canal), `GO`/`GODONE` (início/flag). |
| `ADCON1` | `PVCFG`, `NVCFG` — referências de tensão. |
| `ADCON2` | `ADFM` (justificação), `ACQT` (tempo de aquisição), `ADCS` (clock de conversão). |
| `ADRESH:ADRESL` | Resultado de 10 bits (alias `ADRES` se `ADFM = 1`). |
| `ANSELA`, `ANSELB`… | **Analog Select** — obrigatório para o pino funcionar como entrada analógica. |
| `TRISA`, `TRISB`… | Pino como entrada. |

### Clock de conversão (`ADCS`) vs. `_XTAL_FREQ`

T_AD mínimo do PIC18F46K22 é ~1 µs. Escolher `ADCS` para garantir `T_AD ≥ 1 µs`:
- Fosc = 16 MHz → `Fosc/32` (T_AD = 2 µs) é seguro.
- Fosc = 48 MHz → `Fosc/32` dá 0,67 µs → **abaixo do mínimo**; use `Fosc/64` (~1,33 µs) ou `FRC` (clock interno do ADC, sempre seguro).

Documente a escolha no código.

---

## API a implementar em `RTOS/io.c` e `RTOS/io.h`

Seguir a mesma assinatura da referência em `RTOS_V25.X/io.c`:

```c
void     adc_init(void);
uint16_t adc_read(uint8_t channel);   // channel: 0..12 → AN0..AN12
```

### `adc_init`

1. Configurar os pinos analógicos usados pela aplicação (pelo menos AN0): `TRISAbits.RA0 = 1` e `ANSELAbits.ANSA0 = 1`. Se a aplicação usar AN1, fazer o mesmo em `TRISAbits.RA1` e `ANSELAbits.ANSA1`.
2. `ADCON1` — Vref+ = Vdd, Vref− = Vss (valores padrão para Proteus 5 V / protoboard 3,3 V). Prefira forma bit-a-bit (`PVCFG`, `NVCFG`) — mais legível que valor hexadecimal opaco.
3. `ADCON2` — `ADFM = 1` (justificação à direita), `ACQT = 0b110` (16 TAD de aquisição automática), `ADCS` conforme o `_XTAL_FREQ` escolhido (ver tabela acima).
4. `ADCON0bits.ADON = 1`.

### `adc_read(channel)`

1. `ADCON0bits.CHS = channel`.
2. Pequeno `__delay_us(...)` **somente** se `ACQT` não cobrir a aquisição (o RTOS_V25.X faz ambos — redundante; escolha um caminho e documente).
3. `ADCON0bits.GO = 1`.
4. `while (ADCON0bits.GO);` — busy-wait (~10 µs).
5. Retornar `(ADRESH << 8) | ADRESL`.

---

## Integração com o RTOS (`RTOS/`)

- `adc_read` é **síncrona** e faz busy-wait de ~10 µs por conversão. Para o `QUANTUM` do scheduler (dezenas de ms), isso é desprezível.
- **Não** chame `adc_read` de dentro de uma ISR (etapa 6). A ISR do Timer0 é curta por projeto; manter.
- Se duas tasks chamam `adc_read` simultaneamente, há corrida em `ADCON0bits.CHS`. Proteção é **responsabilidade da aplicação** (`user.c`) via `mutex_t` já pronto em `RTOS/sync.h`. **Não** colocar mutex dentro da HAL.
- `adc_init` deve ser chamada **uma única vez** em `config_user()` dentro de `RTOS/user.c`, antes de `os_start`.

---

## Regras

- HAL continua consolidada em `RTOS/io.c`/`io.h` — sem arquivos novos.
- Não implementar variantes (callback, IRQ, DMA, filtro, média móvel) — nada disso é pedido.
- Não trazer helpers de `RTOS_V25.X/user_app.c` (média móvel da `tarefa_acelerador`, pipe de acelerador, `pwm_duty[3]`, etc.) — são de outra aplicação.
- **Sempre** setar `ANSELAbits.ANSA0 = 1` (e equivalentes). Sem isto, o PIC18F46K22 lê digital e o ADC retorna 0 ou lixo.
- Registradores: preferir forma bit-a-bit (`ADCON0bits.CHS = ...`) em vez de valor hexadecimal opaco.

---

## Entrega

- `RTOS/io.c` e `RTOS/io.h` com `adc_init(void)` e `adc_read(uint8_t)` funcionando.
- Tabela bit-a-bit justificando `ADCON1`, `ADCON2` e o `ADCS` escolhido (relacionado ao `_XTAL_FREQ`).
- Exemplo de uso em uma task de `RTOS/user.c` lendo AN0 (e opcionalmente AN1), sem reproduzir a lógica da injeção eletrônica/acelerador do `RTOS_V25.X`.
- Helper de conversão raw → tensão, documentando `Vref` usado (ex.: `v = raw * VREF / 1023.0f`). Conversão para unidade física (temperatura, por exemplo) é opcional — se o exemplo usar temperatura, reutilize `ler_temperatura()` de `API_ADC.X/main.c` como referência direta, sem copiar o LED em RD5.
