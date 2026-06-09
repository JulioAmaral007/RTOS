# Prompt — Etapa 2: Escalonamento Híbrido (Prioridade + Round-Robin)

Atue como especialista em RTOS e escalonamento de tarefas para sistemas embarcados PIC18F.

Analise completamente `RTOS/scheduler.c`, `RTOS/scheduler.h`, `RTOS/hw.c`, `RTOS/types.h` e `RTOS/os_config.h` antes de qualquer modificação.

---

## Contexto do código atual

O RTOS possui dois modos de scheduler separados configurados via `#define DEFAULT_SCHEDULER` em `os_config.h`: `ROUND_ROBIN` e `PRIORITY`. Eles são implementados como algoritmos independentes.

O requisito do trabalho **não é ter dois modos separados**. O requisito é um **único algoritmo híbrido**:

> "Escalonamento baseado em prioridade. Quando duas ou mais tarefas tiverem a mesma prioridade, o algoritmo deverá ser Round-Robin."

---

## Algoritmo híbrido a implementar

### Comportamento esperado

1. O scheduler sempre seleciona a task READY de **maior prioridade**.
2. Se houver **apenas uma** task READY com a maior prioridade: ela executa sem interrupção de quantum.
3. Se houver **duas ou mais** tasks READY com a mesma maior prioridade: elas compartilham a CPU via Round-Robin com quantum definido por `QUANTUM` em `os_config.h`.
4. Tasks de prioridade mais baixa **nunca** executam enquanto houver tasks de prioridade mais alta prontas.

### Lógica do scheduler (pseudocódigo)

```
function schedule():
    highest_prio = encontrar maior prioridade entre tasks em estado READY
    candidates[] = todas as tasks READY com prioridade == highest_prio
    
    if len(candidates) == 1:
        executar candidates[0]
        resetar quantum counter
    else:
        // Round-Robin entre candidates
        next = próxima task em candidates após a task atual (circular)
        executar next
```

### Integração com Timer0 ISR (`hw.c`)

O Timer0 ISR em `hw.c` já gerencia delays e pode gerenciar o quantum:
- Manter um contador `quantum_counter` global.
- A cada tick do Timer0: decrementar `quantum_counter`.
- Quando `quantum_counter == 0`: forçar chamada ao scheduler (`os_schedule()`), resetar `quantum_counter = QUANTUM`.
- O quantum só deve ser aplicado quando o grupo de maior prioridade tiver mais de uma task.

### Campo `priority` no TCB

O campo `priority` já existe em `TCB_t` (`types.h`). Utilizá-lo diretamente — não criar campo novo.

---

## O que modificar

- `scheduler.c`: reescrever a função `os_schedule()` (ou `schedule_task()`) com o algoritmo híbrido.
- `hw.c`: adicionar controle de quantum no ISR do Timer0, ativado apenas para grupos de igual prioridade.
- `os_config.h`: manter a constante `QUANTUM` — ela continua sendo utilizada pelo algoritmo híbrido.
- `types.h`: verificar se é necessário campo adicional no TCB para controle de Round-Robin (e.g., ponteiro de round-robin circular).

---

## Regras

- Não manter os dois modos separados (`ROUND_ROBIN` e `PRIORITY`) como configuração — unificá-los no único algoritmo híbrido.
- Não quebrar o mecanismo de delays (`os_delay()`) que também usa o Timer0.
- O scheduler deve ser chamado tanto por preempção (Timer0) quanto por bloqueio voluntário (`os_yield()`, `mutex_lock()`, `sem_wait()`).

---

## Entrega

- `scheduler.c` e `scheduler.h` atualizados com o algoritmo híbrido
- `hw.c` com controle de quantum integrado
- Explicação do critério de seleção de task
- Tabela mostrando comportamento com 3 tasks: 2 de prioridade 1 e 1 de prioridade 2
