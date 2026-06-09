# Primeiro Trabalho Prático — Documentação Técnica

**Disciplina:** Sistemas Operacionais Embarcados (DEC7562 — UFSC)
**Microcontrolador:** PIC18F46K22 — XC8 v3.10 — MPLAB X v6.30
**Aluno:** Júlio Cézar Pereira
**Data:** 28/04/2026

---

## Sumário

1. [Introdução](#1-introdução)
2. [Arquitetura Geral do RTOS](#2-arquitetura-geral-do-rtos)
3. [Explicação dos Requisitos do T1](#3-explicação-dos-requisitos-do-t1)
   - 3.1 [Mutex (sincronização por exclusão mútua)](#31-mutex--sincronização-por-exclusão-mútua)
   - 3.2 [Escalonamento por Prioridade + Round-Robin](#32-escalonamento-por-prioridade--round-robin)
   - 3.3 [Alocação dinâmica de estruturas do SO](#33-alocação-dinâmica-de-estruturas-do-so)
   - 3.4 [API de E/S — PWM, ADC e Interrupção Externa](#34-api-de-es--pwm-adc-e-interrupção-externa)
   - 3.5 [Restrições da aplicação embarcada](#35-restrições-da-aplicação-embarcada)
4. [Explicação da Aplicação Embarcada](#4-explicação-da-aplicação-embarcada)
5. [Componentes Escolhidos](#5-componentes-escolhidos)
6. [Mapeamento de Pinos](#6-mapeamento-de-pinos)
7. [Funcionamento no Proteus](#7-funcionamento-no-proteus)
8. [Conclusão](#8-conclusão)

---

## 1. Introdução

### 1.1 Objetivo do trabalho

O Primeiro Trabalho Prático (T1) consiste em **estender o RTOS desenvolvido em sala** com novas funcionalidades de kernel e, em seguida, **construir uma aplicação embarcada real** que exercite todos esses mecanismos de forma integrada.

O enunciado (arquivo `t1.png`) especifica quatro inclusões obrigatórias na API do RTOS:

1. **Mecanismo de sincronização baseado em variáveis mutex.**
2. **Escalonador baseado em prioridade**, com Round-Robin como critério de desempate quando duas tarefas possuem a mesma prioridade.
3. **Migração de pelo menos uma estrutura de dados do SO de alocação estática para dinâmica**, utilizando a API de memória do próprio SO.
4. **API de E/S** para manipular **PWM**, **ADC** e **interrupção externa**.

Após a implementação dessas inclusões, deve ser construída uma aplicação embarcada com as seguintes restrições:

- Mais de três tarefas;
- Comunicação entre tarefas via PIPE (fila de mensagens);
- Tarefas com prioridades iguais;
- Tarefas com prioridades diferentes;
- Sincronização por semáforos *e* variáveis mutex;
- Tarefas *one-shot* (única execução), criadas a partir de uma **interrupção externa**.

### 1.2 Aplicação escolhida — Sistema de Monitoramento de Temperatura

A aplicação implementada é um **sistema de monitoramento térmico** baseado no sensor LM35. Essa escolha foi feita porque ela exercita **simultaneamente todos os requisitos** do enunciado em um único cenário coerente:

- **ADC** lê o sensor LM35;
- **PIPE** transporta as leituras entre a task produtora e a task consumidora;
- **Mutex** protege a variável global de temperatura;
- **Semáforo** sincroniza a task de PWM com a task de display;
- **PWM** controla o brilho de um LED de forma proporcional à temperatura;
- **Interrupção externa** dispara uma **task one-shot** ao apertar um botão;
- Múltiplas prioridades coexistem (idle = 0, tarefas permanentes = 5, one-shot = 6).

A aplicação não foi escolhida pela complexidade, e sim pela **densidade de cobertura dos requisitos**: cada linha de código ativa pelo menos um item do enunciado.

---

## 2. Arquitetura Geral do RTOS

O RTOS é um **microkernel preemptivo** organizado em camadas bem isoladas. O diagrama abaixo descreve a hierarquia de dependências entre os módulos:

```
            +--------------------+
            |   user.c (tasks)   |
            +---------+----------+
                      |
       +---------+----+----+--------+--------+
       |         |         |        |        |
       v         v         v        v        v
   sync.c     com.c      io.c    mem.c    types.h
   (mutex/   (pipe       (ADC/   (heap     (TCB,
    sem)      FIFO)       PWM/    SRAM)     ready
                          INT)              queue)
       \         /         |       /
        \       /          |      /
         v     v           v     v
            kernel.c + scheduler.c + hw.c
              (TCB, syscalls, ISR, scheduler)
```

### 2.1 Componentes principais

| Arquivo | Responsabilidade |
|---|---|
| `kernel.c/h` | Syscalls (`os_create_task`, `os_delay`, `os_yield`, `os_task_exit`), macros `SAVE_CONTEXT`/`RESTORE_CONTEXT` e *idle task*. |
| `scheduler.c/h` | Três algoritmos selecionáveis: Round-Robin puro, Prioridade puro e Prioridade + RR (default). |
| `hw.c/h` | Configuração do Timer0 e ISR única (tick do scheduler + INT0 do botão). |
| `sync.c/h` | Semáforos contadores e mutex com *ownership*. |
| `com.c/h` | PIPE (FIFO circular de bytes) construído sobre dois semáforos. |
| `io.c/h` | API de E/S genérica: `pwm_*`, `adc_*`, `ext_int_init`. |
| `mem.c/h` | Heap dinâmica em SRAM (Microchip *SRAMalloc*). |
| `types.h` | Definições dos tipos `tcb_t`, `state_t`, `ready_queue_t`. |
| `os_config.h` | Constantes de configuração: `QUANTUM`, `MAX_USER_TASKS`, `DEFAULT_SCHEDULER` etc. |
| `user.c/h` | Tasks da aplicação embarcada e função `config_user`. |
| `main.c` | `SRAMInitHeap` → `os_config` → `os_create_task` × 3 → `os_start`. |

### 2.2 Estrutura do TCB (`types.h`)

Cada tarefa é representada por um *Task Control Block* que armazena:

- **Identificação:** `task_id`, `task_priority`.
- **Estado:** `task_state` (`READY`, `RUNNING`, `WAITING`, `WAITING_SEM`, `WAITING_MUTEX`).
- **Função:** ponteiro `task_ptr` para o corpo da tarefa.
- **Delay:** contador `task_delay` decrementado pelo Timer0 ISR.
- **Snapshot completo da CPU:** `W`, `STATUS`, `BSR`, `FSR0/1/2`, `PROD`, `TABLAT`, `TBLPTR`, `PCLATH`, `PCLATU`.
- **Cópia da pilha de hardware do PIC18:** até 31 níveis (endereços de retorno de 21 bits).

### 2.3 Fluxo de inicialização

```
main()
 ├─ SRAMInitHeap()        // prepara heap dinâmica de 512 B
 ├─ os_config()
 │    ├─ inicializa r_queue (size=0, idle = TASKS[0])
 │    ├─ os_create_task(1, idle, 0)
 │    └─ config_user()    // configura GPIOs, mutex, sem, pipe, ADC, PWM, INT0
 ├─ os_create_task(2, task_sensor,  5)
 ├─ os_create_task(3, task_display, 5)
 ├─ os_create_task(4, task_pwm,     5)
 └─ os_start()
      ├─ setup_hardware()           // Timer0 + GIE
      ├─ scheduler() + RESTORE_CONTEXT() // primeira escolha
      └─ ENABLE_ALL_INTERRUPTS()    // GIE = 1 — agora há preempção
```

### 2.4 Mecânica de troca de contexto

A troca é executada inteiramente em macros C (`SAVE_CONTEXT`/`RESTORE_CONTEXT`) com pequenos blocos *inline assembly* para `PUSH`/`POP`. Por que macros e não funções?

- O PIC18F não possui pilha de software acessível como em arquiteturas com SP em RAM. A **pilha de hardware** (31 níveis) precisa ser esvaziada e recarregada pelo próprio kernel.
- Funções inseririam um nível adicional de `CALL/RETURN` na pilha exatamente quando estamos manipulando-a — quebrando a invariante.
- Macros são expandidas inline no chamador, evitando essa contaminação.

O `SAVE_CONTEXT(state)`:

1. Marca a task atual com o novo estado (`WAITING`, `READY`, `WAITING_SEM`, etc.);
2. Copia todos os registradores principais para o TCB;
3. Esvazia a pilha de hardware (`while (STKPTR) { POP }`), copiando cada `TOSL/TOSH/TOSU` para `task_stack`.

O `RESTORE_CONTEXT()` faz o caminho inverso, marcando a task como `RUNNING`. Se for o primeiro despacho da task (`stack_size == 0`), empilha o endereço do `task_ptr` para que o `RETURN` final transfira o PC para o início da função da task.

---

## 3. Explicação dos Requisitos do T1

### 3.1 Mutex — Sincronização por Exclusão Mútua

#### 3.1.1 O que é

Mutex (*mutual exclusion*) é uma primitiva binária com **noção de propriedade** (*ownership*): apenas a task que executou `mutex_lock` pode chamar `mutex_unlock`. Diferente de um semáforo binário, o mutex impede que outra task libere acidentalmente um recurso que não detém.

**Por que existir um mutex separado do semáforo?** Porque os dois resolvem problemas diferentes:

| Problema | Primitiva |
|---|---|
| Proteger acesso a um recurso compartilhado (seção crítica) | **Mutex** |
| Sinalizar a ocorrência de um evento entre tasks | **Semáforo** |

Usar semáforo binário como mutex é um anti-padrão clássico: qualquer task pode chamar `sem_post` e liberar a região crítica que outra task possuía.

#### 3.1.2 Como foi implementado

Arquivo: `sync.c` / `sync.h`

```c
typedef struct {
    uint8_t locked;
    uint8_t owner_pos;             // índice em r_queue.TASKS da task dona
    uint8_t fila[MAX_USER_TASKS];  // fila circular de tasks bloqueadas
    uint8_t pos_input;
    uint8_t pos_output;
} mutex_t;
```

**`mutex_lock`** (lógica em três caminhos):

1. **Mutex livre** → `locked = 1`, `owner_pos = task atual`, retorna imediatamente.
2. **Reentrância** (a própria dona chama de novo) → não bloqueia (evita auto-deadlock).
3. **Mutex ocupado por outra** → enfileira o índice da task chamadora, marca o estado como `WAITING_MUTEX`, salva contexto, chama `scheduler()`, restaura.

**`mutex_unlock`** segue a regra de *ownership*:

- Se a chamadora **não é a dona**, retorna sem fazer nada (proteção contra unlocks indevidos).
- Se há tasks na fila → **transfere a posse** para a task bloqueada há mais tempo (FIFO) e a marca como `READY`. O mutex permanece `LOCKED`, agora com novo dono.
- Se a fila está vazia → libera o mutex (`locked = 0`, `owner_pos = INVALID_OWNER`).

Toda essa lógica está envelopada em `DISABLE_ALL_INTERRUPTS` / `ENABLE_ALL_INTERRUPTS` (manipulação de `INTCONbits.GIE`) para que o estado do mutex e da fila de aptos seja sempre consistente.

#### 3.1.3 Por que foi implementado dessa forma

- **Transferência direta de posse no unlock:** evita o "*lock convoy*" e a corrida em que outra task de prioridade maior interceptaria o mutex antes da task que estava bloqueada acordar.
- **Estado dedicado `WAITING_MUTEX`:** o scheduler diferencia tasks bloqueadas por mutex de tasks bloqueadas por delay/semáforo, facilitando depuração via simulação.
- **Fila circular do mesmo tamanho que `MAX_USER_TASKS`:** garantia teórica de que toda task pode estar bloqueada simultaneamente sem estouro.
- **Reentrância silenciosa:** evita auto-deadlock se uma task chamada de dentro da seção crítica acabar tentando entrar no mesmo mutex.

#### 3.1.4 Onde é utilizado na aplicação

A variável global `temp_global` (em `user.c`) é compartilhada entre `task_display` (escritora) e `task_pwm` (leitora). Como ambas têm prioridade 5 e são preemptáveis pelo Timer0, qualquer leitura/escrita não atômica (8 bits no PIC18 é atômico, mas a aplicação foi escrita para suportar valores maiores no futuro) corre risco de inconsistência. O mutex `m_temp` garante que nenhuma task lerá a variável **enquanto** outra está atualizando.

---

### 3.2 Escalonamento por Prioridade + Round-Robin

#### 3.2.1 O que é

O enunciado pede o algoritmo **Prioridade fixa preemptiva**: a task de maior prioridade `READY` sempre roda. Quando duas ou mais tasks empatarem na prioridade mais alta, o **Round-Robin** as alterna a cada quantum.

#### 3.2.2 Como foi implementado

Arquivo: `scheduler.c`

O RTOS oferece três algoritmos selecionáveis em `os_config.h`:

```c
#define DEFAULT_SCHEDULER   RR_PRIOR_SCHEDULER
```

- `RR_SCHEDULER` (didático puro)
- `PRIOR_SCHEDULER` (prioridade pura, sem RR)
- `RR_PRIOR_SCHEDULER` ← **escolhido para este trabalho**

A função `rr_prior_scheduler()` opera em duas passadas:

```c
// Passo 1: descobre a maior prioridade entre tasks READY
for (i = 0; i < r_queue.size; i++)
    if (r_queue.TASKS[i].task_state == READY &&
        r_queue.TASKS[i].task_priority > highest_prio)
        highest_prio = r_queue.TASKS[i].task_priority;

// Passo 2: escolhe a próxima task READY com essa prioridade,
//          em ordem circular a partir da posição atual (RR).
for (i = 1; i <= r_queue.size; i++) {
    uint8_t idx = (r_queue.pos_task_running + i) % r_queue.size;
    if (r_queue.TASKS[idx].task_state == READY &&
        r_queue.TASKS[idx].task_priority == highest_prio)
        return idx;
}
```

A varredura **começa em `pos_task_running + 1`**, garantindo que a próxima task escolhida não é a que acabou de ser preemptada (rotação justa).

**Disparo do RR** — `hw.c`:

```c
if (INTCONbits.TMR0IF) {
    INTCONbits.TMR0IF = 0;
    // ... decremento de delays ...
    rr_quantum--;
    if (rr_quantum == 0) {
        rr_quantum = QUANTUM;          // QUANTUM = 5 ticks
        SAVE_CONTEXT(READY);
        scheduler();
        RESTORE_CONTEXT();
    }
}
```

A cada *tick* do Timer0, o quantum é decrementado. Quando zera, o contexto é salvo, `scheduler()` é chamado e a próxima task é despachada.

#### 3.2.3 Por que foi implementado dessa forma

- **Duas passadas em vez de uma só:** fica explícito o critério "primeiro prioridade, depois RR". Isso facilita revisar o código durante apresentação oral.
- **Varredura circular a partir de `pos_task_running + 1`:** condição necessária para que o RR seja **justo** (sem starvation entre tasks de mesma prioridade).
- **Idle como fallback:** a task `idle` está em `TASKS[0]` com prioridade 0 e estado sempre `READY`. Se nenhuma task de usuário estiver pronta, o algoritmo retorna 0 e a CPU não fica em estado indefinido.
- **`RESTORE_CONTEXT` zera `rr_quantum`:** garante que cada nova task ganhe sempre o quantum cheio.

#### 3.2.4 Como funciona internamente

Cenário típico do projeto (3 tasks com prio 5, RR entre elas):

```
   t = 0   tick   tick   tick   tick   tick (quantum esgotado)
   |-------|-------|-------|-------|-------|
   <-- task_sensor executando -------------->  preempção via Timer0
   |
   |---->  rr_prior_scheduler() escolhe task_display
                                              <-- task_display ----->
                                                                     <-- task_pwm ...
```

Se um botão for pressionado e a `one_shot_task` for criada com prioridade 6:

```
... task_sensor (prio 5) ...
        |
        +-- ISR INT0 cria one_shot_task (prio 6)
            |
            +-- próximo SAVE_CONTEXT (no fim do tick): scheduler escolhe one_shot
                |
                +-- one_shot_task executa, chama os_task_exit, libera slot
                    |
                    +-- volta a rotação RR entre task_sensor/display/pwm
```

#### 3.2.5 Onde é utilizado na aplicação

- **Prioridade 0 →** `idle` (cobertura constante, baixíssima latência de retorno).
- **Prioridade 5 →** `task_sensor`, `task_display`, `task_pwm` (RR entre si).
- **Prioridade 6 →** `one_shot_task` (preempta as três acima quando criada).

Isso comprova **simultaneamente** os dois pontos do enunciado: tasks de prioridades diferentes (0, 5, 6) e tasks de prioridades iguais (todas com 5).

---

### 3.3 Alocação dinâmica de estruturas do SO

#### 3.3.1 O que é

O enunciado pede que **alguma estrutura do SO** seja migrada de alocação estática (declaração compilada como variável global ou em `.bss`) para alocação **dinâmica** em runtime, usando a API de memória do próprio SO (`SRAMalloc`/`SRAMfree`).

#### 3.3.2 O que foi escolhido

A estrutura escolhida foi o **buffer de dados do PIPE** (`fila_dados`).

Antes (alocação estática típica):
```c
typedef struct pipe {
    char fila_dados[PIPE_MAX_SIZE];
    ...
} pipe_t;
```

Depois (alocação dinâmica em `com.c`):
```c
typedef struct pipe {
    char    *fila_dados;   // ponteiro para buffer alocado via SRAMalloc
    uint8_t  capacity;
    uint8_t  pos_input;
    uint8_t  pos_output;
    sem_t    s_input;
    sem_t    s_output;
} pipe_t;

void pipe_init(pipe_t *p)
{
    p->fila_dados = (char *) SRAMalloc(PIPE_MAX_SIZE);
    if (p->fila_dados == NULL) return;
    p->capacity   = PIPE_MAX_SIZE;
    p->pos_input  = 0;
    p->pos_output = 0;
    sem_init(&p->s_input,  PIPE_MAX_SIZE);
    sem_init(&p->s_output, 0);
}

void pipe_destroy(pipe_t *p)
{
    SRAMfree((unsigned char *) p->fila_dados);
    p->fila_dados = NULL;
    p->capacity   = 0;
}
```

#### 3.3.3 Como `SRAMalloc` funciona internamente

`mem.c` é o alocador da Microchip (`sralloc.c`), baseado em **lista encadeada implícita**:

- Heap reservada estaticamente: `unsigned char _uDynamicHeap[MAX_HEAP_SIZE]` com `MAX_HEAP_SIZE = 0x200` (512 bytes).
- Cada bloco possui um **header de 1 byte**: bit 7 = `alloc`, bits 6:0 = `count` (tamanho do segmento, incluindo header).
- `SRAMInitHeap()` divide a heap inicial em segmentos de no máximo 127 bytes (`_MAX_SEGMENT_SIZE`).
- `SRAMalloc(n)`:
  - Percorre a lista de headers;
  - Se acha um segmento livre suficiente, o aloca;
  - Se acha um menor que o pedido, tenta **`_SRAMmerge`** com o vizinho livre.
- `SRAMfree(p)` apenas zera o bit `alloc` no header — *coalescing* só ocorre na próxima alocação via merge.

Limitações: alocação máxima por chamada de **126 bytes** e heap total de **512 bytes**. Suficiente para o PIPE de 4 bytes.

#### 3.3.4 Por que escolher o PIPE (e não, por exemplo, o TCB)?

Decisão baseada em três fatores:

1. **Tamanho do PIC18:** `MAX_HEAP_SIZE = 512 B` e `_MAX_SEGMENT_SIZE = 127 B`. Os TCBs ocupam ~118 bytes cada (snapshot completo de registradores + 31 níveis de pilha). Quatro TCBs já consumiriam quase toda a heap, deixando pouca margem.
2. **Estabilidade:** alocar TCBs dinamicamente forçaria todo `os_create_task` a pagar custo de busca/merge no heap durante a inicialização do sistema. O PIPE, por ser inicializado uma única vez em `config_user`, paga o custo apenas no boot.
3. **Demonstração do conceito:** `pipe_init` mostra **claramente** o uso de `SRAMalloc` e `pipe_destroy` mostra `SRAMfree` — o ciclo completo de vida da memória dinâmica fica documentado e auditável durante a apresentação.

#### 3.3.5 Onde é utilizada na aplicação

A chamada acontece em `config_user()`:

```c
pipe_init(&p_temp);   // SRAMalloc(PIPE_MAX_SIZE) por trás
```

Esse buffer é o canal de comunicação `task_sensor → task_display`.

---

### 3.4 API de E/S — PWM, ADC e Interrupção Externa

Toda a API está concentrada em `io.c` / `io.h`, deliberadamente separada do kernel. Isso evita acoplar o RTOS a um periférico específico (boa prática de portabilidade) e oferece uma interface enxuta:

```c
// PWM
void     pwm_init(uint8_t channel);
void     pwm_set_duty(uint8_t channel, uint16_t duty);

// ADC
void     adc_init(void);
uint16_t adc_read(uint8_t channel);

// Interrupção externa
void     ext_int_init(uint8_t int_pin, uint8_t edge);
```

#### 3.4.1 PWM — `pwm_init` / `pwm_set_duty`

**Hardware envolvido:** módulo CCP1 + Timer2 do PIC18F46K22, saída em **RC2**.

**Configuração:**

```c
TRISCbits.TRISC2 = 0;    // RC2 como saída
CCP1CON          = 0b00001100; // modo PWM
T2CON            = 0b00000100; // Timer2 ON, prescaler 1:1
PR2              = 0xFF;       // período máximo
CCPR1L           = 0;
CCP1CONbits.DC1B = 0;          // duty inicial = 0%
```

**Cálculo da frequência:**

`F_PWM = F_OSC / (4 × prescaler × (PR2+1)) = 4 MHz / (4 × 1 × 256) ≈ 3,9 kHz`

**Resolução do duty:** 10 bits → `duty_max = 4 × (PR2+1) = 1024`. O bits altos do duty vão em `CCPR1L`, os 2 bits baixos em `CCP1CONbits.DC1B`.

**Por que CCP1 / Timer2 e não outro CCP?** Porque o Timer0 já é o tick do scheduler; usar Timer2 isola completamente a base de tempo do PWM da base de tempo do escalonador. Os dois módulos têm registradores próprios (`T0CON`/`TMR0` vs `T2CON`/`TMR2`), nunca compartilham flags de interrupção, e a ISR do scheduler testa apenas `INTCONbits.TMR0IF`. Logo, o PWM funciona sem perturbar o kernel.

#### 3.4.2 ADC — `adc_init` / `adc_read`

**Hardware envolvido:** módulo ADC do PIC18F46K22, canal **AN0** em **RA0**.

**Configuração crítica:**

```c
TRISAbits.RA0    = 1;      // RA0 como entrada
ANSELAbits.ANSA0 = 1;      // habilita função analógica em AN0 (obrigatório)
ADCON1bits.PVCFG = 0b00;   // Vref+ = VDD (~5 V)
ADCON1bits.NVCFG = 0b00;   // Vref- = VSS
ADCON2bits.ADFM  = 1;      // resultado justificado à direita
ADCON2bits.ACQT  = 0b110;  // 16 TAD de aquisição automática
ADCON2bits.ADCS  = 0b010;  // Fosc/32 → T_AD = 8 µs
ADCON0bits.ADON  = 1;
```

**Tempo de aquisição:** o LM35 tem impedância de saída baixíssima (~1 kΩ), porém o capacitor de hold do ADC precisa ser carregado antes da conversão. O T_ACQ recomendado pelo datasheet do PIC18F é ~2,5 µs no mínimo. Configurar `ACQT = 16 TAD ≈ 128 µs` dá margem amplíssima e elimina a necessidade de `__delay_us` antes de cada leitura — o hardware espera sozinho.

**Leitura:**
```c
uint16_t adc_read(uint8_t channel)
{
    ADCON0bits.CHS = channel;
    ADCON0bits.GO  = 1;
    while (ADCON0bits.GO);
    return (uint16_t)((ADRESH << 8) | ADRESL);
}
```

**Conversão para Celsius (em `task_sensor`):**

`temp_C = raw × 125 / 256`

Equivale a `raw × 500 / 1024` (LM35 dá 10 mV/°C, VDD = 5 V → 1 LSB ≈ 4,88 mV → cada °C = 2,048 LSB), mas escrito com aritmética de 16 bits, evitando divisão de 32 bits que o XC8 free implementa por chamada de biblioteca cara.

#### 3.4.3 Interrupção externa — `ext_int_init`

**Hardware envolvido:** linha INT0 (= **RB0**).

**Configuração:**

```c
TRISBbits.TRISB0    = 1;    // entrada
ANSELBbits.ANSB0    = 0;    // desabilita AN12 em RB0 (necessário para leitura digital)
INTCON2bits.INTEDG0 = edge; // 0 = borda de descida (botão pull-up)
INTCONbits.INT0IF   = 0;    // limpa flag
INTCONbits.INT0IE   = 1;    // habilita INT0
```

A função **não** habilita `GIE` — esse trabalho fica para `os_start()`, via `ENABLE_ALL_INTERRUPTS()`. Habilitar GIE antes de o scheduler estar pronto causaria interrupções em estado inconsistente.

**ISR única** (`hw.c`):

```c
void __interrupt() ISR(void)
{
    if (INTCONbits.INT0IF) {
        INTCONbits.INT0IF = 0;
        if (r_queue.size < MAX_USER_TASKS + 1) {
            os_create_task(5, one_shot_task, 6);   // cria one-shot
        }
    }
    if (INTCONbits.TMR0IF) {
        // tick do scheduler ...
    }
}
```

A guarda `r_queue.size < MAX_USER_TASKS + 1` impede a criação de uma one-shot quando já há uma na fila, eliminando o risco de estouro do array `TASKS[]` em caso de ruído de chave (bouncing).

---

### 3.5 Restrições da aplicação embarcada

| Restrição do enunciado | Como foi atendida |
|---|---|
| Mais de três tarefas | 4 permanentes (idle, sensor, display, pwm) + 1 dinâmica (one-shot) = **5 tasks** |
| Comunicação via PIPE | `task_sensor` escreve, `task_display` lê — `pipe_t p_temp` |
| Algumas tasks com mesma prioridade | sensor, display, pwm → todas com prioridade 5 (RR entre elas) |
| Tasks com prioridades diferentes | idle = 0, permanentes = 5, one-shot = 6 |
| Sincronização por semáforo **e** mutex | `s_new_data` (semáforo) + `m_temp` (mutex) |
| Task one-shot criada por interrupção externa | INT0/RB0 → ISR cria `one_shot_task` (prio 6), que termina com `os_task_exit()` |

---

## 4. Explicação da Aplicação Embarcada

### 4.1 Diagrama de fluxo entre tasks

```
                                       LM35
                                        |
                                  (sinal analógico)
                                        |
                                        v
              +---------------+    AN0/RA0    +----------------+
   Timer0 →   | task_sensor   |==== ADC ====> | adc_read(0)    |
   tick (RR)  | prio = 5      |               +----------------+
              +-------+-------+
                      | pipe_write(temp)
                      v
              +---------------+         +-----------------+
              | PIPE p_temp   |  FIFO   | sem_t s_output  |
              | 4 bytes       |<------->| sem_t s_input   |
              +-------+-------+         +-----------------+
                      | pipe_read
                      v
              +---------------+
              | task_display  | --(mutex_lock m_temp)--> temp_global = t
              | prio = 5      |     LATE0/LATE1/LATE2 (faixa térmica)
              +-------+-------+     LATD0 (alarme >60 °C)
                      | sem_post(s_new_data)
                      v
              +---------------+
              | task_pwm      | --(mutex_lock m_temp)--> lê temp_global
              | prio = 5      |     pwm_set_duty(1, t * 13)
              +---------------+     RC2 brilha proporcional à temperatura

   ==== Caminho assíncrono (interrupção externa) ====

   Botão RB0 (borda descida) → INT0 ISR
                                  |
                                  +-- os_create_task(5, one_shot_task, 6)
                                          |
                                          v
                                   one_shot_task (prio 6, preempta tudo)
                                          |
                                          +-- toggle LATD2
                                          +-- os_task_exit() — libera slot
```

### 4.2 Detalhamento das tasks

#### `task_sensor` — produtor (prio 5)

```c
TASK task_sensor(void)
{
    while (1) {
        uint16_t raw  = adc_read(0);
        uint8_t  temp = (uint8_t)((raw * 125U) >> 8);
        pipe_write(&p_temp, (char)temp);
        os_delay(10);     // ~10 ticks do Timer0
    }
}
```

- Lê o LM35 a cada 10 ticks;
- Converte em °C;
- Empurra para o pipe (bloqueia se cheio, via semáforo `s_input`);
- `os_delay` cede CPU às outras tasks até o tick expirar.

#### `task_display` — consumidor + sinalizador (prio 5)

```c
TASK task_display(void)
{
    char dado;
    while (1) {
        pipe_read(&p_temp, &dado);    // bloqueia em sem_wait(s_output)
        uint8_t t = (uint8_t)dado;

        mutex_lock(&m_temp);
        temp_global = t;
        mutex_unlock(&m_temp);

        LED_LOW  = (t < 25)            ? 1 : 0;  // RE0
        LED_MID  = (t >= 25 && t < 45) ? 1 : 0;  // RE1
        LED_HIGH = (t >= 45)            ? 1 : 0; // RE2

        if (t > 60) LED_ALARM = ~LED_ALARM;     // RD0 piscando
        else        LED_ALARM = 0;

        sem_post(&s_new_data);    // acorda task_pwm
    }
}
```

- Bloqueia até haver dado no pipe;
- Atualiza `temp_global` dentro de seção crítica protegida pelo mutex;
- Indica a faixa térmica em LEDs exclusivos (RE0/RE1/RE2);
- Pisca alarme RD0 acima de 60 °C;
- Posta semáforo para liberar a `task_pwm`.

#### `task_pwm` — atuador (prio 5)

```c
TASK task_pwm(void)
{
    while (1) {
        sem_wait(&s_new_data);   // bloqueia até task_display sinalizar

        mutex_lock(&m_temp);
        uint8_t t = temp_global;
        mutex_unlock(&m_temp);

        uint16_t duty = (uint16_t)t * 13U;     // 13 ≈ 1024/79
        if (duty > 1024U) duty = 1024U;
        pwm_set_duty(1, duty);
    }
}
```

- Bloqueia em `sem_wait` até nova leitura disponível;
- Lê `temp_global` dentro do mutex;
- Calcula `duty = t × 13` (saturado em 1024) — 0 °C → 0% / 79 °C → 100%;
- Aplica em CCP1.

A multiplicação por 13 evita uma divisão real (`× 1024 / 79`) que o XC8 free resolveria por uma rotina de software lenta. Como o sensor LM35 limita-se a 0–150 °C e o circuito real raramente passa de 60 °C, o erro de aproximação é desprezível.

#### `one_shot_task` — transiente (prio 6)

```c
TASK one_shot_task(void)
{
    LED_ONESHOT = ~LED_ONESHOT;   // toggle RD2
    os_task_exit();                // remove TCB e libera slot
}
```

- Criada **apenas** pela ISR de INT0;
- Executa exatamente uma vez (toggle no LED RD2);
- Termina chamando `os_task_exit()`, que decrementa `r_queue.size` e dispara um novo escalonamento.

#### `idle` — task de sistema (prio 0)

```c
TASK idle()
{
    TRISCbits.RC0 = 0;
    while (1) PORTCbits.RC0 = ~PORTCbits.RC0;
}
```

- Sempre `READY`;
- Nunca chamada se houver outra task pronta;
- Pisca RC0 para indicar visualmente "kernel vivo" (heart-beat).

### 4.3 Mapeamento requisito ↔ código

| Requisito | Implementação | Local |
|---|---|---|
| Mutex | `mutex_lock` / `mutex_unlock` | `sync.c`; uso em `task_display`, `task_pwm` |
| Semáforo | `sem_init`, `sem_wait`, `sem_post` | `sync.c`; uso entre `task_display` e `task_pwm` |
| Scheduler prio + RR | `rr_prior_scheduler()` | `scheduler.c` |
| Alocação dinâmica | `SRAMalloc` em `pipe_init` | `com.c` + `mem.c` |
| ADC | `adc_init` / `adc_read` | `io.c`; uso em `task_sensor` |
| PWM | `pwm_init` / `pwm_set_duty` | `io.c`; uso em `task_pwm` |
| INT externa | `ext_int_init` + ISR | `io.c` + `hw.c` |
| One-shot | `os_task_exit()` + criação na ISR | `kernel.c` + `hw.c` |

---

## 5. Componentes Escolhidos

### 5.1 PIC18F46K22

**Escolha:** definida pela disciplina, mas tecnicamente justificável.

**Por que ele se encaixa no projeto:**

- **40 pinos** — sobra GPIO para LEDs, botão, sensor, PWM e debug.
- **Periféricos integrados que cobrem todos os requisitos:** ADC de 10 bits com 14 canais, 5 módulos CCP (incluindo 3 ECCP com PWM avançado), 4 timers, INT0/INT1/INT2 dedicados.
- **Memória de programa:** 64 KB Flash — sobra para o RTOS + aplicação.
- **SRAM:** 3,8 KB — espaço suficiente para os TCBs (4 × 118 ≈ 472 B), heap dinâmica (512 B), pilhas e variáveis globais.
- **Pilha de hardware:** 31 níveis — exatamente o `MAX_STACK_SIZE` salvo no TCB.
- **Compilador XC8 maduro** com suporte a `__interrupt()` e *inline asm*, indispensáveis para implementar `SAVE_CONTEXT/RESTORE_CONTEXT`.

### 5.2 LM35 (sensor de temperatura)

**Por que LM35 e não NTC, DS18B20 ou termopar:**

- **Saída linear de 10 mV/°C**, sem necessidade de tabela de calibração ou linearização. Conversão se reduz a uma multiplicação inteira.
- **Alimentado por VDD do PIC (5 V)**, dispensa fonte adicional.
- **Saída analógica direta** — encaixa na entrada AN0 sem buffer.
- **Existe modelo no Proteus** (`U2 - LM35`) — permite simulação completa.
- **Range típico de 0–100 °C** atende totalmente o cenário (alarme em 60 °C).

### 5.3 LEDs

**Função no sistema (cada um valida um requisito):**

| LED | Pino | Critério | Requisito que valida |
|---|---|---|---|
| LED_LOW | RE0 | t < 25 °C | Pipe + Mutex + ADC funcionando |
| LED_MID | RE1 | 25 ≤ t < 45 °C | Pipe + Mutex + ADC funcionando |
| LED_HIGH | RE2 | t ≥ 45 °C | Pipe + Mutex + ADC funcionando |
| LED_ALARM | RD0 | pisca quando t > 60 °C | Lógica de alarme + scheduler ativo |
| LED_ONESHOT | RD2 | toggle quando botão pressionado | INT externa + one-shot + `os_task_exit` |
| LED PWM | RC2 | brilho ∝ temperatura | PWM + sincronização por semáforo |
| LED idle | RC0 | toggle constante | Kernel vivo |

**Por que LEDs e não display LCD/Seven-segment:**

- Validação **visual instantânea** durante apresentação;
- Cada LED isola um requisito específico, evitando ambiguidade na demonstração;
- Modelo simples no Proteus (LED-RED + resistor de 330 Ω);
- Não consome pinos extras como SPI/I²C.

### 5.4 Botão (push-button)

- Conectado a **RB0/INT0** com **pull-up** externo (resistor `R1 = 10 kΩ` no esquema do Proteus);
- Borda de descida configurada (`INTEDG0 = 0`): aperto do botão = transição 1→0;
- Validação direta da **interrupção externa** (requisito 4) e da **task one-shot** (restrição da aplicação).

### 5.5 Resistor de pull-up R1 (10 kΩ)

Mantém a linha INT0 em nível alto enquanto o botão não é pressionado, garantindo borda de descida limpa quando o botão fecha o circuito para GND.

---

## 6. Mapeamento de Pinos

### 6.1 Tabela completa

| Pino | Função | Periférico | Direção | Justificativa |
|---|---|---|---|---|
| **RA0** | Entrada do LM35 | ADC AN0 | IN (ANSA0=1) | Único pino que combina canal AN0 (canal 0 default) com ANSEL — minimiza configuração. |
| **RB0** | Botão | INT0 | IN digital (ANSB0=0) | RB0 é o único pino que possui INT0 hardware no PIC18F46K22. |
| **RC0** | LED idle | GPIO | OUT | Fora dos pinos de PWM/CCP/serial; livre. |
| **RC2** | LED PWM | CCP1 (PWM) | OUT | RC2 é a saída padrão do CCP1 no PIC18F46K22. Selecionar outro CCP exigiria periféricos remapeáveis (PPS), inexistentes na K22. |
| **RD0** | LED alarme | GPIO | OUT (ANSD0=0) | Pino digital simples, próximo a RD2 no esquema. |
| **RD2** | LED feedback one-shot | GPIO | OUT (ANSD2=0) | Mesma faixa do PORTD — facilidade de roteamento no Proteus. |
| **RE0** | LED faixa baixa | GPIO | OUT (ANSE0=0) | PORTE só tem 3 pinos (RE0–RE2) — usados como vetor de faixa. |
| **RE1** | LED faixa média | GPIO | OUT (ANSE1=0) | Idem. |
| **RE2** | LED faixa alta | GPIO | OUT (ANSE2=0) | Idem. |
| **MCLR/VPP** | Reset | — | IN | Ligado a `VDD` via pull-up no esquema (configuração padrão). |

### 6.2 Por que esses pinos e não outros

- **RA0 vs RA1/RA2/...:** todos suportam ADC, mas AN0 é o canal default após reset (`CHS = 0` → AN0). Reduz configuração.
- **RB0 (INT0) é obrigatório** — INT0 só existe em RB0 no PIC18F46K22.
- **RC2 (CCP1) é obrigatório para PWM via CCP1** — sem PPS, é o único pino possível.
- **PORTE (RE0–RE2)** é frequentemente subutilizado e tem **apenas 3 pinos**, ideais para um vetor compacto de "estado térmico".
- **PORTD** sobrou para LEDs auxiliares (alarme + feedback) por ter 8 pinos disponíveis e ficar "no caminho" do roteamento Proteus.
- **`ANSEL = 0` em todos os pinos digitais:** falha clássica em PIC18F é deixar o pino em modo analógico — leitura digital sempre lê 0. Cada `TRISxbits` é acompanhado do correspondente `ANSELx`bits.

### 6.3 Conflitos verificados

- **Timer0 (scheduler) × Timer2 (PWM):** módulos independentes, sem conflito. Apenas `INTCONbits.TMR0IF` é tratado na ISR.
- **INT0 × ANSB0:** `ANSELBbits.ANSB0 = 0` desliga AN12 em RB0, restaurando o pino em modo digital — necessário para INT0 funcionar.
- **RA0 × LATA:** RA0 está em modo entrada (`TRIS=1`), o estado de `LATA` não importa.

---

## 7. Funcionamento no Proteus

### 7.1 Comportamento esperado em runtime

Após carregar `dist/default/production/RTOS.production.hex` no PIC18F46K22 do Proteus:

1. **Boot:** RC0 (LED da idle) começa a pulsar rapidamente — o kernel está vivo, o scheduler está rodando RR entre as 3 tasks de prio 5 e a idle só executa quando todas estão `WAITING`.
2. **Sensor LM35 a 25 °C (default no Proteus):**
   - `task_sensor` lê 25 °C, escreve no pipe.
   - `task_display` lê do pipe, atualiza `temp_global`, acende **LED_MID (RE1)**, mantém alarme apagado.
   - `task_pwm` ajusta duty ≈ 25 × 13 = 325 → ~32% brilho no LED RC2.
3. **Aumentando a tensão do LM35 para 50 °C:**
   - LED_MID apaga; **LED_HIGH (RE2)** acende.
   - PWM sobe para ~65% brilho (duty = 650).
4. **Aumentando para 65 °C:**
   - **LED_ALARM (RD0)** começa a piscar.
   - PWM ~85% brilho.
5. **Pressionar o botão (BUT em RB0):**
   - INT0 dispara, ISR cria `one_shot_task` (prio 6).
   - No próximo tick, scheduler escolhe a one-shot (preempta tudo).
   - **LED_ONESHOT (RD2)** alterna de estado.
   - `os_task_exit` libera o slot, scheduler volta ao RR entre as 3 tasks de prio 5.

### 7.2 Como validar cada requisito

| Requisito | Como observar no Proteus |
|---|---|
| **Mutex** | Não há manifestação visual direta — a evidência é negativa: nenhum *glitch* nas atualizações de PWM. Pode-se confirmar inserindo `mutex_unlock` antes de `mutex_lock` em uma das tasks e observando o crash/comportamento errático. |
| **Scheduler prio + RR** | RC0 (idle) pisca devagar: idle só roda enquanto todas as 3 tasks estão em delay. Se uma das prio 5 for substituída por prio 6 fixo, RC0 para de piscar (essa task absorve toda a CPU). |
| **Alocação dinâmica** | Visualizar `_uDynamicHeap` no Memory Watch da MPLAB X durante simulação: o byte de header em `&_uDynamicHeap[0]` muda de `0x7F` para `0x84` (`0x80` allocated + `0x04` count) após `pipe_init`. |
| **ADC** | Mover o slider do LM35 (potenciômetro virtual) → LEDs RE0/RE1/RE2 trocam conforme a faixa. |
| **PWM** | Observar o brilho do LED em RC2 mudando suavemente conforme a temperatura (ou ligar oscilóscopio virtual no pino). |
| **INT externa + one-shot** | Clicar o botão → LED_ONESHOT (RD2) toggla. Cada clique = um toggle. Se mantém pressionado, INT0 só dispara uma vez na borda; o `r_queue.size` impede acúmulo. |
| **PIPE** | Reduzir `PIPE_MAX_SIZE` para 1 e observar `task_sensor` bloqueando se `task_display` for atrasada (introduzindo `os_delay` longo). |
| **Semáforo** | Comentar `sem_post(&s_new_data)` em `task_display` → LED PWM (RC2) congela no último valor (task_pwm fica em `WAITING_SEM` para sempre). |

### 7.3 Limites conhecidos da simulação

- O Proteus simula o LM35 com modelo ideal — sem ruído, sem inércia térmica;
- Bouncing real do botão não é modelado — nenhuma necessidade de debounce em software para a simulação;
- O timing do PWM é fiel: 3,9 kHz observáveis em oscilóscopio virtual.

---

## 8. Conclusão

### 8.1 Avaliação dos requisitos

Todos os quatro itens do enunciado foram implementados:

1. ✅ **Mutex** com *ownership* e fila circular FIFO (`sync.c`).
2. ✅ **Scheduler prio + RR** com seleção em duas passadas (`scheduler.c::rr_prior_scheduler`).
3. ✅ **Alocação dinâmica** do buffer do PIPE via `SRAMalloc` (`com.c::pipe_init`).
4. ✅ **API de E/S** completa para PWM, ADC e interrupção externa (`io.c`).

Todas as seis restrições da aplicação foram atendidas:

- ✅ Mais de 3 tasks (5 no total: idle, sensor, display, pwm, one-shot);
- ✅ Comunicação via PIPE (sensor → display);
- ✅ Tasks com prioridades iguais (5/5/5);
- ✅ Tasks com prioridades diferentes (0, 5, 6);
- ✅ Sincronização por semáforo (`s_new_data`) **e** mutex (`m_temp`);
- ✅ Task one-shot criada por interrupção externa (INT0 → `one_shot_task`).

### 8.2 Decisões de projeto resumidas

| Decisão | Motivação |
|---|---|
| Sistema de monitoramento térmico como aplicação | Densidade de cobertura: **um único cenário** exercita todos os requisitos. |
| `RR_PRIOR_SCHEDULER` como default | Atende exatamente o que o enunciado pede ("prioridade, RR para mesma prioridade"). |
| PIPE como estrutura migrada para alocação dinâmica | Tamanho compatível com o heap de 512 B; ciclo `init/destroy` claro para apresentação. |
| Mutex separado de semáforo binário | *Ownership* impede unlock indevido — boa prática didática. |
| ISR única tratando Timer0 + INT0 | Escolha do XC8 (uma só `__interrupt()`); flags discriminam origem. |
| Conversão de temperatura em 16 bits (sem divisão) | XC8 free torna divisão de 32 bits cara — multiplicação fixa é determinística. |
| `pwm_set_duty(1, t × 13)` | Aproximação inteira `1024/79 ≈ 13`, evita 32-bit divide; erro irrelevante no range. |
| LEDs RE0/RE1/RE2 como vetor de faixa térmica | PORTE só tem 3 pinos — encaixe perfeito; valida pipe+mutex+ADC visualmente. |
| Guarda `r_queue.size < MAX+1` na ISR | Elimina risco de estouro do array `TASKS[]` por bouncing. |

### 8.3 O que se aprende com este trabalho

A implementação evidencia, na prática, três lições centrais de Sistemas Operacionais Embarcados:

1. **Concorrência exige sincronização correta:** sem `mutex_lock`/`mutex_unlock` em torno de `temp_global`, valores intermediários poderiam ser lidos durante a escrita.
2. **Escalonamento define o comportamento temporal real:** alterar `DEFAULT_SCHEDULER` muda completamente a ordem de execução observada nos LEDs, mesmo sem mudar uma única linha das tasks.
3. **O kernel é tão estável quanto sua manipulação de pilha:** `SAVE_CONTEXT`/`RESTORE_CONTEXT` precisam ser macros (não funções) e o `ENABLE/DISABLE_ALL_INTERRUPTS` precisa envelopar todas as seções críticas — uma única falha aqui torna o sistema travável de forma não-determinística.

Em conjunto, o RTOS desenvolvido exibe todas as características esperadas de um microkernel preemptivo de tempo real, e a aplicação embarcada serve como evidência funcional dessas características em hardware real e simulado.

---

**Arquivos do projeto referenciados nesta documentação:**

- `main.c`, `kernel.c/h`, `scheduler.c/h`, `hw.c/h`
- `sync.c/h`, `com.c/h`, `mem.c/h`, `io.c/h`
- `user.c/h`, `types.h`, `os_config.h`
- `t1.png` (enunciado), `circuito.png` (esquemático Proteus)
- `Proteus/*.pdsprj` (projeto de simulação)
