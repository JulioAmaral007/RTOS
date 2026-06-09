# Prompt — Etapa 6: Interrupção Externa + Task One-Shot

Atue como especialista em sistemas embarcados, interrupções do PIC18F e RTOS preemptivo.

**Projeto alvo:** `RTOS/` — único projeto entregue no t1.
**Referências técnicas:**
- `RTOS_V25.X/io.c` — função `ext_int_init(int_pin, edge)` pronta (modelo do `io.c` final).
- `RTOS_V25.X/timer.c` — ISR com *hook* `interrupt_user()` chamado antes do tratamento do Timer0 (padrão de separar ISR de sistema vs. ISR de aplicação).

**Importante:** **não** porte nenhuma lógica de aplicação do `RTOS_V25.X` (ex.: `tarefa_controle_estabilidade`, `freio_acionado`, `tempo_injecao`). O t1 pede **tasks one-shot genéricas**, não um sistema automotivo.

---

## Requisito central (`t1.pdf`)

> "Tarefas *one-shot*, ou seja, tarefas que tenham uma única execução. A criação de uma tarefa one-shot **deverá acontecer quando uma interrupção externa for gerada**."

Duas peças:
1. Interrupção externa — detectar um evento (botão) via INT0.
2. Task one-shot — criada dentro da ISR, executa uma única vez, se encerra.

---

## Leitura obrigatória antes de implementar

1. `RTOS/hw.c` — hoje o ISR do projeto é **único** (`__interrupt() ISR(void)`) e trata apenas Timer0. **Não existe hook de usuário.** Você precisará tratar INT0 **dentro deste mesmo ISR** (há um só vetor sendo usado).
2. `RTOS/hw.h` — declaração do ISR.
3. `RTOS/kernel.c` — `os_create_task(id, func, prior)` aloca TCB dinamicamente (`SRAMalloc`). **`os_task_exit()` já está implementado** (linha ~107): remove a task da fila, faz `SRAMfree` do TCB e chama o scheduler. Use-o — **não** o substitua.
4. `RTOS/kernel.h` — macros `SAVE_CONTEXT`, `RESTORE_CONTEXT`, `DISABLE_ALL_INTERRUPTS`, `ENABLE_ALL_INTERRUPTS`.
5. `RTOS/types.h` — estados atuais: `READY, WAITING, RUNNING, WAITING_SEM, WAITING_MUTEX`. **Não é necessário adicionar `TERMINATED`** — `os_task_exit` já remove a task da `TASKS[]` e libera memória.
6. `RTOS_V25.X/io.c` — referência de `ext_int_init`. **Não** copie o `interrupt_user()` com lógica automotiva do `user_app.c` — esse callback é do outro projeto.

---

## Parte 1 — Interrupção Externa no PIC18F46K22

### Pinos disponíveis
- **INT0** — RB0 (sempre alta prioridade).
- INT1 — RB1, INT2 — RB2 (não necessários).

### Função `ext_int_init` em `RTOS/io.c`

Usar o mesmo padrão da referência `RTOS_V25.X/io.c`:

```c
void ext_int_init(uint8_t int_pin, uint8_t edge);
```

- `int_pin == 0`: configurar `TRISBbits.TRISB0 = 1`, `INTCON2bits.INTEDG0 = edge`, limpar `INT0IF`, setar `INT0IE`.
- Habilitar `INTCONbits.PEIE = 1` e `INTCONbits.GIE = 1` ao final (ou deixar isso para `os_start`; documente a escolha).
- `edge`: 0 = borda de descida (botão com pull-up), 1 = borda de subida.

Se a aplicação do t1 usar apenas INT0, trate somente esse caso. Os demais pinos podem ficar como *no-op* com comentário.

---

## Parte 2 — Tratamento do INT0 no ISR do `RTOS/hw.c`

### Estrutura atual (já presente)

```c
void __interrupt() ISR(void) {
    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;
        // decrementa task_delay, quantum, scheduler...
    }
}
```

### Extensão a implementar

Adicionar, **antes ou depois** do bloco do Timer0 (mas no mesmo ISR, pois só há um vetor em uso), um segundo `if` que trate `INT0IF`:

```c
if (INTCONbits.INT0IF) {
    INTCONbits.INT0IF = 0;          // limpar flag PRIMEIRO
    if (!one_shot_pending) {        // evita re-entrada por rebote
        one_shot_pending = 1;
        os_create_task(<id>, one_shot_task, <priority>);
    }
}
```

Pontos obrigatórios:
- Limpar `INT0IF` **antes** de qualquer processamento.
- **Não** fazer `__delay_ms` dentro do ISR — trate o *debounce* na própria one-shot (os primeiros 10–20 ms da task são um `os_delay` antes de reler `PORTBbits.RB0`).
- `one_shot_pending` é `volatile uint8_t` global (definida em `hw.c` ou `user.c`, extern no `.h` correspondente). Protege contra criação múltipla caso o botão gere várias bordas antes da one-shot terminar.
- **Não** crie um segundo vetor (`__interrupt(low_priority)`) — o projeto usa um só.

### Alternativa de organização (opcional)

Se preferir separar responsabilidades, adicione em `hw.c` um *hook* chamado `interrupt_user()` implementado em `user.c`:

```c
// hw.c
void __interrupt() ISR(void) {
    interrupt_user();   // trata INT0 e demais flags de aplicação
    if (INTCONbits.TMR0IF) { ... }  // Timer0 permanece no ISR do sistema
}
```

Esse é exatamente o padrão de `RTOS_V25.X/timer.c`. Adote se a separação facilitar a organização do seu código.

---

## Parte 3 — Task one-shot em `RTOS/user.c`

Use **`os_task_exit()`** como última instrução — **já existe** em `RTOS/kernel.c`. Não invente `remove_task` nem loop `while(1){yield();}`.

### Exemplo mínimo (para o t1)

```c
// Flag global (em user.c ou hw.c)
volatile uint8_t one_shot_pending = 0;

TASK one_shot_task(void) {
    os_delay(2);            // debounce: ~2 ticks após o disparo
    if (PORTBbits.RB0 == 0) {
        // evento válido: executar a ação única
        LATDbits.LATD2 = ~LATDbits.LATD2;   // feedback visual
        // ... qualquer operação curta (ler ADC, postar semáforo, etc.)
    }
    one_shot_pending = 0;   // libera a flag ANTES de sair
    os_task_exit();         // remove da fila, SRAMfree do TCB, scheduler
    // nunca retorna daqui
}
```

### Criação via ISR

Já descrito na Parte 2 — dentro do `ISR(void)` em `hw.c`, ao capturar `INT0IF`, chamar `os_create_task(<id_livre>, one_shot_task, <prioridade>)`. Escolha:
- `id_livre` diferente de 1 (idle) e dos ids das tasks estáticas (2, 3, 4 são usadas em `main.c`).
- `prioridade` **≥** a das outras tasks, se o one-shot deve interromper o trabalho normal (recomendado: igual à maior prioridade para disputar via RR, ou maior se for um alarme).

---

## Parte 4 — Ciclo de vida do TCB na one-shot

1. Botão pressionado → INT0IF sobe.
2. ISR em `hw.c` limpa `INT0IF`, marca `one_shot_pending = 1`, chama `os_create_task(..., one_shot_task, ...)`.
3. `os_create_task` aloca TCB via `SRAMalloc`, adiciona em `r_queue.TASKS[r_queue.size++]` com estado `READY`.
4. Próximo tick / próxima chamada do scheduler → `rr_prior_scheduler()` escolhe a one-shot (pela prioridade).
5. `one_shot_task` executa a ação única.
6. `one_shot_task` limpa `one_shot_pending` e chama `os_task_exit()`.
7. `os_task_exit` remove o slot em `TASKS[]`, faz `SRAMfree(TCB)`, chama o scheduler e retorna a CPU para a próxima task.

---

## Regras

- **Não** adicionar estado `TERMINATED` no enum — o projeto resolve com `os_task_exit` + `SRAMfree`. Se usado em algum caminho, documente por quê.
- **Não** substituir `os_task_exit` por `remove_task` + `while(1){yield();}`. Essa é abordagem do `RTOS_V25.X`; aqui há algo melhor já pronto.
- **Não** chamar funções longas (delays em ms, `adc_read` com `ACQT` longo, pipes bloqueantes) dentro do ISR.
- Flag `one_shot_pending` é escrita no ISR (sem proteção necessária, pois GIE está em 0) e lida na task — na task, proteja leitura/escrita com `DISABLE_ALL_INTERRUPTS`/`ENABLE_ALL_INTERRUPTS` se houver mais de um ponto de acesso.
- O ISR deve continuar curto; mover trabalho para a task.

---

## Entrega

- `RTOS/io.c`/`io.h` com `ext_int_init(uint8_t int_pin, uint8_t edge)` implementada para INT0.
- `RTOS/hw.c` modificado para tratar `INT0IF` dentro do ISR único existente, criando a task one-shot via `os_create_task` — com comentário explicando por que limpar flag antes e por que `one_shot_pending` é necessário.
- `RTOS/user.c` com uma task `one_shot_task` que usa `os_task_exit()` como última instrução. A ação da task deve ser simples e observável (ex.: toggle de LED, leitura pontual de ADC, post de semáforo).
- Variável global `volatile uint8_t one_shot_pending` declarada e usada como descrito.
- Fluxo documentado em 6 passos (botão → ISR → create → scheduler → run → exit) no relatório da etapa.
- Verificar e documentar: não é necessário incluir `TERMINATED` no enum.
