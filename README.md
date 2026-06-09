# PIC18-RTOS — Kernel de Tempo Real para PIC18F46K22

Implementação completa de um RTOS preemptivo construído do zero para o microcontrolador PIC18F46K22, com escalonamento por prioridade e Round-Robin, semáforos, mutex, pipes, alocação dinâmica e API de hardware (PWM, ADC e interrupção externa).

---

## 🚀 Visão Geral

Este projeto implementa um kernel de sistema operacional de tempo real (RTOS) desenvolvido inteiramente em C com mínimo de assembly para o PIC18F46K22. O sistema gerencia múltiplas tarefas concorrentes de forma preemptiva, sincroniza o acesso a recursos compartilhados e abstrai periféricos de hardware, tudo em um microcontrolador com apenas 3,8 KB de SRAM.

**Público-alvo:** Desenvolvedores embarcados, estudantes de sistemas operacionais e engenheiros interessados na implementação interna de um RTOS em arquitetura de 8 bits.

**Problema resolvido:** Coordenar tarefas de leitura de sensor, controle de atuador e resposta a eventos externos de forma determinística e sem bloqueios manuais de polling, em hardware extremamente restrito.

**Aplicação de demonstração:** Sistema de monitoramento de temperatura com sensor LM35, controle de brilho por PWM, indicação por LEDs e resposta a botão por interrupção externa.

---

## 🛠️ Tecnologias Utilizadas

- **Linguagem:** C (C99), com assembly inline para save/restore de contexto
- **Microcontrolador:** PIC18F46K22 — 64 KB Flash, 3,8 KB SRAM, pilha de hardware de 31 níveis
- **Compilador:** XC8 v3.10 (Microchip)
- **IDE:** MPLAB X IDE v6.30
- **Simulação:** Proteus VSM (arquivos `.pdsprj` incluídos)
- **Build System:** Makefiles gerados pelo MPLAB X (`nbproject/`)
- **Periféricos utilizados:** Timer0 (tick do kernel), Timer2 + CCP1 (PWM), ADC AN0, INT0 (interrupção externa)

---

## 🎯 Principais Funcionalidades

### Kernel

- Escalonamento preemptivo com três algoritmos selecionáveis em tempo de compilação: **Round-Robin puro**, **Prioridade pura** e **Prioridade + Round-Robin** (padrão)
- Troca de contexto completa: salva e restaura 18 registradores do PIC18 e copia integralmente a pilha de hardware de 31 níveis para a TCB
- Syscalls: `os_create_task`, `os_delay`, `os_yield`, `os_task_exit`, `os_task_change_state`
- Tarefa idle integrada (prioridade 0) garante que o sistema nunca trave
- Suporte a **tarefas one-shot** criadas dinamicamente pela ISR de interrupção externa

### Sincronização

- **Semáforos** POSIX-compatíveis com contador podendo ser negativo, fila FIFO de tarefas bloqueadas
- **Mutex** com controle de propriedade, prevenção de auto-deadlock (reentrância) e transferência de propriedade no unlock

### Comunicação entre Tarefas

- **Pipe** (FIFO circular) com buffer alocado dinamicamente no heap; sincronização interna por semáforos de produtor-consumidor

### Gerenciamento de Memória

- Alocador dinâmico de heap SRAM de 512 bytes (Microchip SRAMalloc)
- Suporte a alocação (`SRAMalloc`), liberação (`SRAMfree`) e coalescência de blocos adjacentes livres

### API de Hardware (I/O)

- **PWM** via CCP1 + Timer2: 10 bits de resolução, ~3,9 kHz (RC2)
- **ADC** de 10 bits via AN0/RA0, com LM35 → conversão direta para °C
- **Interrupção externa** INT0/RB0 por borda de descida, integrada à ISR do kernel

### Aplicação de Demonstração (Monitoramento de Temperatura)

- `task_sensor` — lê ADC a cada 10 ticks (~640 ms) e escreve na pipe
- `task_display` — consome pipe, atualiza LEDs por faixa (< 25°C / 25–44°C / ≥ 45°C) e sinaliza alarme acima de 60°C
- `task_pwm` — ajusta o duty do PWM linearmente com a temperatura (0°C = 0%, 79°C = 100%)
- `one_shot_task` — criada ao pressionar o botão (INT0), togla LED e termina sozinha

---

## 🏗️ Arquitetura

```
┌─────────────────────────────────────────────────────────┐
│                   Aplicação do Usuário                   │
│   task_sensor   task_display   task_pwm   one_shot_task  │
├─────────────────────────────────────────────────────────┤
│          Primitivas de Sincronização e IPC               │
│       semáforo · mutex · pipe (circular FIFO)            │
├──────────────────────┬──────────────────────────────────┤
│      Kernel / OS     │     Gerenciamento de Memória      │
│  scheduler · syscalls│  SRAMalloc / SRAMfree (512 B)     │
├──────────────────────┴──────────────────────────────────┤
│                  Camada de Hardware                       │
│   Timer0 ISR (tick) · INT0 ISR · PWM · ADC              │
├─────────────────────────────────────────────────────────┤
│          PIC18F46K22 — XC8 v3.10 — 4 MHz                │
└─────────────────────────────────────────────────────────┘
```

**Fluxo principal:**

1. `SRAMInitHeap()` inicializa o heap antes de qualquer alocação.
2. `os_config()` cria a tarefa idle e configura recursos do usuário (GPIO, PWM, ADC, interrupção).
3. Três tarefas de aplicação são registradas com `os_create_task`.
4. `os_start()` configura o Timer0, habilita interrupções e despacha a primeira tarefa.
5. O Timer0 gera um tick periódico que decrementa contadores de delay e aciona preempção quando o quantum Round-Robin expira.
6. A ISR unificada trata tanto o Timer0 quanto o INT0; flags distinguem a origem.

---

## 📂 Estrutura do Projeto

```
RTOS/
├── main.c              — Sequência de inicialização do sistema
├── kernel.c / kernel.h — Syscalls do OS e macros de save/restore de contexto
├── scheduler.c / .h    — Algoritmos Round-Robin, Prioridade e RR+Prioridade
├── hw.c / hw.h         — Configuração do Timer0 e ISR unificada
├── sync.c / sync.h     — Semáforos e mutex
├── com.c / com.h       — Pipe com buffer dinâmico
├── mem.c / mem.h       — Alocador de heap SRAM (Microchip)
├── io.c / io.h         — API de hardware: PWM, ADC, interrupção externa
├── user.c / user.h     — Tarefas da aplicação de monitoramento de temperatura
├── types.h             — TCB, estados de tarefa, fila de tarefas, tipos base
├── os_config.h         — Constantes de configuração do kernel
├── Proteus/            — Esquema e arquivos de simulação (.pdsprj)
├── prompts/            — Enunciados das atividades do laboratório
└── nbproject/          — Arquivos de projeto MPLAB X e Makefiles gerados
```

| Arquivo | Responsabilidade |
|---|---|
| `types.h` | Define `tcb_t`, `state_t`, `hw_stack_t`, `ready_queue_t` e todos os tipos do sistema |
| `os_config.h` | Constantes tunáveis: `MAX_USER_TASKS`, `QUANTUM`, `PIPE_MAX_SIZE`, `DEFAULT_SCHEDULER` |
| `kernel.c` | Implementa syscalls e macros assembly de troca de contexto |
| `scheduler.c` | Três algoritmos de escalonamento; selecionável por `#define` |
| `hw.c` | Configura Timer0, ISR do tick e da interrupção externa INT0 |
| `sync.c` | Semáforos e mutex com filas de bloqueio e controle de propriedade |
| `com.c` | Pipe produtor-consumidor com sincronização interna por semáforos |
| `mem.c` | Heap implícita com lista encadeada, alocação, liberação e coalescência |
| `io.c` | HAL para PWM (CCP1/Timer2), ADC (AN0) e INT0 |
| `user.c` | Três tarefas de aplicação + configuração de GPIO e primitivas |

---

## ⚙️ Como Executar

### Pré-requisitos

- **MPLAB X IDE** v6.30 ou superior — [download](https://www.microchip.com/mplab/mplab-x-ide)
- **XC8 Compiler** v3.10 — [download](https://www.microchip.com/xc-compilers)
- **Proteus VSM** (para simulação) ou hardware real PIC18F46K22
- Programador compatível (PICkit 3/4, MPLAB Snap) caso use hardware físico

### Compilação via MPLAB X IDE

1. Abra o MPLAB X IDE.
2. File → Open Project → selecione a pasta `RTOS/`.
3. Selecione a configuração `default` (produção).
4. Build → Build Project (ou `F11`).

O `.hex` gerado estará em `dist/default/production/`.

### Compilação via linha de comando

```bash
# No diretório RTOS/
make build     # compila a configuração padrão
make clean     # remove objetos intermediários
make clobber   # remove todos os artefatos de build
```

### Simulação no Proteus

1. Abra `Proteus/FinalCircuit.pdsprj`.
2. Carregue o `.hex` gerado no componente PIC18F46K22.
3. Inicie a simulação (Play).
4. Observe os LEDs (RE0, RE1, RE2, RD0, RD2, RC2) e use o potenciômetro para simular o LM35.
5. Pressione o botão em RB0 para disparar a tarefa one-shot.

### Gravação em Hardware Real

```bash
# Com IPE (MPLAB IPE) ou via IDE:
# Target → PIC18F46K22
# Hex File → dist/default/production/RTOS.production.hex
# Program
```

---

## ⚙️ Configuração do Kernel

Todas as constantes tunáveis estão em `os_config.h`:

| Constante | Padrão | Significado |
|---|---|---|
| `DEFAULT_SCHEDULER` | `RR_PRIOR_SCHEDULER` | Algoritmo: `RR_SCHEDULER`, `PRIOR_SCHEDULER` ou `RR_PRIOR_SCHEDULER` |
| `MAX_USER_TASKS` | `4` | Número máximo de tarefas de usuário simultâneas |
| `MAX_STACK_SIZE` | `31` | Profundidade máxima da pilha de hardware salva por tarefa |
| `QUANTUM` | `5` | Ticks do Timer0 por fatia de tempo Round-Robin |
| `PIPE_MAX_SIZE` | `4` | Capacidade em bytes do buffer da pipe |
| `_XTAL_FREQ` | `4000000UL` | Frequência do oscilador do sistema |

---

## 🗺️ Mapeamento de Pinos

| Pino | Função | Modo |
|---|---|---|
| RA0 | Sensor LM35 (temperatura) | Entrada analógica AN0 |
| RB0 | Botão (one-shot) | Interrupção INT0, borda de descida |
| RC0 | LED heartbeat (idle) | Saída digital |
| RC2 | LED de brilho variável | Saída PWM (CCP1) |
| RD0 | LED de alarme (> 60°C) | Saída digital |
| RD2 | LED de feedback one-shot | Saída digital |
| RE0 | LED faixa baixa (< 25°C) | Saída digital |
| RE1 | LED faixa média (25–44°C) | Saída digital |
| RE2 | LED faixa alta (≥ 45°C) | Saída digital |

---

## 🔒 Segurança e Robustez

- **Seções críticas** protegidas por desabilitar GIE antes de acessar estruturas do kernel (semáforos, mutex, contexto)
- **Mutex com propriedade:** desbloquear de tarefa não-proprietária é ignorado silenciosamente
- **Proteção contra self-deadlock:** se o dono tenta adquirir o mesmo mutex novamente, retorna imediatamente
- **Guarda na ISR:** one-shot tasks só são criadas se houver slot disponível (`r_queue.size < MAX_USER_TASKS + 1`), evitando overflow da fila por bouncing de botão
- **Heap limitado:** `SRAMalloc` retorna `NULL` para alocações inválidas ou quando o heap está esgotado; `pipe_init` depende de heap disponível

---

## 📈 Melhorias Futuras

- Suporte a múltiplos núcleos (dual-core via PIC32MK ou dsPIC33)
- Alarmes de software com resolução de microssegundos
- Mutex com herança de prioridade para eliminar inversão de prioridade
- Watchdog integrado ao kernel para detecção de deadlock
- Porta para PIC18F47Q10 com suporte a DMA
- Interface de monitoramento via UART (debug shell)
- Expansão do heap com banco de memória externo (SPI SRAM)

---

## 📚 Contexto Acadêmico

Projeto desenvolvido para a disciplina **Sistemas Operacionais Embarcados** da **UFSC** (2026/1). O trabalho cobre seis atividades práticas sequenciais:

1. Implementação de mutex com controle de propriedade
2. Escalonadores Round-Robin e por prioridade
3. Alocação dinâmica de memória em heap SRAM
4. API de controle PWM via CCP1 + Timer2
5. API de leitura ADC com conversão para temperatura
6. Interrupção externa e tarefas one-shot dinâmicas

Todas as atividades foram integradas em uma única aplicação de monitoramento de temperatura com sensor LM35.

---

## 👨‍💻 Desenvolvedor

**Desenvolvido por:**

- **Júlio Cézar** — [GitHub](https://github.com/jczrp)

---

> Projeto acadêmico — UFSC · Sistemas Operacionais Embarcados · 2026/1
