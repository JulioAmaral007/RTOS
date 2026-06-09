# Script de Apresentação — T1 RTOS PIC18F46K22
**DEC7562 — Sistemas Operacionais Embarcados — UFSC 2026/1**

---

## 1. Abertura — Proteus já aberto, simulação rodando

> **Começar já com o Proteus rodando. Apontar para o LED RC0 piscando.**

"O sistema já está rodando. Esse LED piscando aqui — RC0 — é a **task idle**.
Ela é a task de menor prioridade do sistema (prioridade 0), e sempre que nenhuma outra task está pronta para rodar, o scheduler escala ela.
O piscar é o sinal de que o kernel está vivo e o escalonador está rodando normalmente."

---

## 2. Mostrando o circuito — o que cada pino faz

> **Apontar para cada componente enquanto explica.**

"Temos aqui:
- **RA0** — entrada do LM35. É o canal AN0 do ADC, o canal padrão após o reset do micro.
- **RB0** — botão de interrupção externa. O INT0 do PIC18F46K22 só existe nesse pino — não tem outro.
- **RC2** — LED controlado por PWM. A saída do módulo CCP1 no PIC18 só sai aqui — sem PPS nesse micro.
- **RE0, RE1, RE2** — três LEDs de faixa de temperatura: baixa, média, alta.
- **RD0** — LED de alarme de sobretemperatura.
- **RD2** — LED da one-shot task, criada pela interrupção do botão."

---

## 3. O que está acontecendo agora — fluxo das tasks

> **Temperatura padrão do Proteus, ~25°C. Apontar para os LEDs acesos.**

"Com a temperatura em torno de 25°C, vejam o que está acontecendo:

- O LED **RE1** (faixa média) está aceso — a `task_display` leu do pipe e identificou a faixa.
- O LED **RC2** tem um brilho médio — a `task_pwm` ajustou o duty proporcional à temperatura.

Esse ciclo completo — sensor → pipe → display → semáforo → pwm — é o coração da aplicação.
Vou explicar cada parte desse fluxo conforme aparece no circuito."

---

## 4. Leitura do sensor — `task_sensor`, ADC

> **Apontar para o LM35 e para RA0.**

"Aqui está o LM35 conectado em RA0.

A função `adc_init` em `io.c` configura o canal AN0:
- `ANSELAbits.ANSA0 = 1` — habilita função analógica em RA0. Sem isso o pino fica digital e o ADC lê lixo.
- Aquisição automática de 16 TAD — o hardware espera o capacitor de hold carregar, sem precisar de delay no código.
- Clock do ADC = Fosc/64 → T_AD = 16 µs, acima do mínimo do datasheet.

A `task_sensor` chama `adc_read` a cada 10 ticks de scheduler, converte o valor bruto para °C:
```c
temp = (uint8_t)(((uint32_t)raw * 500UL) / 1024UL);
```
O 500 vem de: LM35 produz 10 mV/°C, Vref = 5000 mV → 5000 ÷ 10 = 500.
O cast para `uint32_t` é obrigatório — `1023 × 500` estoura `uint16_t`.

Depois de escrever no pipe, a task chama `os_delay(10)` e vai para estado `WAITING`."

---

## 5. Pipe e sincronização — `task_sensor` → `task_display`

> **Explicar o fluxo de dados sem mostrar código, só o comportamento.**

"A temperatura vai da `task_sensor` para a `task_display` através de um **pipe — FIFO circular**.

O pipe foi a estrutura escolhida para a **alocação dinâmica** do T1.
O buffer interno que antes era um array fixo na struct agora é alocado em runtime:
```c
p->fila_dados = (char *) SRAMalloc(PIPE_MAX_SIZE);
```
O alocador `SRAMalloc` usa um heap de **512 bytes em SRAM**, com header de 1 byte por bloco.

Internamente o pipe usa dois semáforos — um controla espaços livres, outro controla dados disponíveis.
Quando o pipe está vazio, `pipe_read` bloqueia a `task_display` com `sem_wait` — ela fica em `WAITING_SEM` até a `task_sensor` escrever e chamar `sem_post`."

---

## 6. Display e mutex — `task_display`

> **Apontar para os LEDs de faixa (RE0/RE1/RE2) e o LED de alarme (RD0).**

"A `task_display` lê a temperatura do pipe e:
1. Escreve em `temp_global` com mutex.
2. Acende o LED da faixa correta — RE0, RE1 ou RE2.
3. Se passar de 60°C, pisca o alarme em RD0.
4. Posta o semáforo `s_new_data` para acordar a `task_pwm`.

O **mutex** em `sync.c` tem noção de posse — só a task que fez o `lock` pode fazer o `unlock`.
A diferença do semáforo binário é exatamente essa: qualquer task poderia liberar um semáforo que não é dela.
O mutex `m_temp` protege a variável `temp_global` compartilhada entre `task_display` (escritora) e `task_pwm` (leitora)."

---

## 7. PWM — `task_pwm`, `pwm_init`, `pwm_set_duty`

> **Apontar para o LED RC2 e variar a temperatura para mostrar o brilho mudando.**

"A `task_pwm` fica bloqueada no semáforo `s_new_data`.
Quando a `task_display` posta, ela acorda, lê `temp_global` com mutex e chama `pwm_set_duty`.

O PWM usa o módulo **CCP1 + Timer2**, com saída no pino RC2.
`pwm_init` configura PR2 = 0xFF, prescaler 1:1 → frequência de ~3,9 kHz.
A resolução é de 10 bits — duty de 0 a 1024.

```c
CCPR1L           = (uint8_t)(duty >> 2);   // 8 MSBs
CCP1CONbits.DC1B = (uint8_t)(duty & 0x03); // 2 LSBs
```

**Por que Timer2 e não Timer0?**
O Timer0 é o tick do scheduler. Timer2 tem registradores próprios e não compartilha a flag de interrupção com o kernel — os dois convivem sem nenhuma interferência.

> *Variar a temperatura no Proteus e mostrar o brilho mudando em RC2.*"

---

## 8. Escalonador — como as tasks se revezam

> **Explicar sem abrir código, só o comportamento visto no Proteus.**

"O escalonador padrão é o `RR_PRIOR_SCHEDULER`, selecionável em `os_config.h`.

Ele opera em **duas passadas**:
1. Acha a maior prioridade entre as tasks `READY`.
2. Dentro dessa prioridade, escolhe a próxima em ordem circular — Round-Robin.

No projeto:
- **Prioridade 5** — `task_sensor` e `task_display`: se ambas estiverem prontas, se revezam por Round-Robin.
- **Prioridade 4** — `task_pwm`: só roda quando as de prio 5 estão bloqueadas.
- **Prioridade 0** — `idle`: roda apenas quando todas as outras estão em WAITING.
- **Prioridade 6** — `one_shot_task`: preempta todas ao ser criada.

O Timer0 dispara a cada tick e o scheduler decide quem roda a seguir."

---

## 9. Interrupção externa e one-shot — apertar o botão

> **Apertar o botão RB0 no Proteus ao vivo.**

"Vou apertar o botão agora."

> *Apertar o botão. Apontar para o LED RD2 toglar.*

"A ISR de INT0 foi ativada. Dentro dela:
```c
if (INTCONbits.INT0IF) {
    INTCONbits.INT0IF = 0;
    if (r_queue.size < MAX_USER_TASKS + 1)
        os_create_task(5, one_shot_task, 6);
}
```
A guarda `r_queue.size < MAX_USER_TASKS + 1` impede criar uma segunda one-shot se o botão tremer.

A `one_shot_task` é criada com **prioridade 6** — maior que todas. No próximo tick do Timer0, o scheduler a vê como a task de maior prioridade, ela preempta tudo, togla o LED RD2, e termina com `os_task_exit()`.
O scheduler então volta ao Round-Robin normal entre as tasks de prioridade 5.

Pontos críticos da `ext_int_init`:
- `ANSELBbits.ANSB0 = 0` — RB0 compartilha com AN12; sem isso o pino fica analógico e a interrupção nunca dispara.
- A função **não toca em GIE** — quem habilita as interrupções globais é o `os_start()`, depois que o kernel está pronto."

---

## 10. Perguntas frequentes

**"Por que macros para salvar/restaurar contexto e não funções?"**
> "O PIC18 tem pilha de hardware de 31 níveis. Uma chamada de função adicionaria um nível na pilha exatamente quando estamos esvaziando ela. Macros são expandidas inline e não contaminam a pilha."

**"Por que semáforo E mutex? Não é redundante?"**
> "Não. O semáforo `s_new_data` sincroniza eventos — task_display avisa task_pwm que há dado novo. O mutex `m_temp` protege acesso exclusivo à variável compartilhada. São problemas diferentes: sinalização vs. exclusão mútua."

**"Por que o buffer do PIPE para alocação dinâmica e não o TCB?"**
> "Cada TCB ocupa ~118 bytes. Quatro TCBs = ~472 bytes, quase toda a heap de 512. Sobrava margem zero. O buffer do pipe tem 4 bytes — seguro, e o ciclo `pipe_init`/`pipe_destroy` demonstra `malloc`/`free` com clareza."

**"Como o debounce do botão é tratado?"**
> "Na simulação não é necessário. Em hardware real, a guarda na ISR impede criar múltiplas one-shots por bouncing — só uma task por vez."

---

*Demonstrar no Proteus: idle piscando → temperatura baixa → temperatura alta → alarme → apertar botão. Deixar o Proteus aberto durante todas as explicações.*
