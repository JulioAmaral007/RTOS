#include "scheduler.h"
#include "types.h"
#include "kernel.h"
#include "os_config.h"

// Fila de aptos
extern ready_queue_t r_queue;


void scheduler()
{
  #if DEFAULT_SCHEDULER == RR_SCHEDULER
    r_queue.pos_task_running = RR_scheduler();
  #elif DEFAULT_SCHEDULER == PRIOR_SCHEDULER
    r_queue.pos_task_running = priority_scheduler();
  #elif DEFAULT_SCHEDULER == RR_PRIOR_SCHEDULER
    r_queue.pos_task_running = rr_prior_scheduler();
  #endif
  r_queue.task_running = r_queue.TASKS[r_queue.pos_task_running];
}

uint8_t RR_scheduler()
{
    uint8_t prox = r_queue.pos_task_running, tentativas = 0;

    do {
        prox = (prox+1) % r_queue.size;
        tentativas++;
        if (tentativas >= (MAX_USER_TASKS+1)) return 0;
    } while (r_queue.TASKS[prox]->task_state != READY ||
             r_queue.TASKS[prox]->task_ptr == idle);

    return prox;
}

uint8_t priority_scheduler(void)
{
    uint8_t prox = r_queue.pos_task_running;

    while (r_queue.TASKS[prox]->task_state != READY)
        prox = (prox + 1) % r_queue.size;

    uint8_t current_task = r_queue.TASKS[prox]->task_priority;

    for (uint8_t i = 1; i < r_queue.size; i++) {
        if (r_queue.TASKS[i]->task_state == READY &&
            r_queue.TASKS[i]->task_priority > current_task) {
            prox = i;
            current_task = r_queue.TASKS[i]->task_priority;
        }
    }

    return prox;
}

// Prioridade com Round-Robin para tarefas de mesma prioridade.
// Passo 1: encontra a maior prioridade entre tasks READY.
// Passo 2: seleciona a proxima task READY com essa prioridade em ordem
//          circular a partir da posicao atual, garantindo a rotacao.
uint8_t rr_prior_scheduler(void)
{
    uint8_t i;
    uint8_t highest_prio = 0;

    for (i = 0; i < r_queue.size; i++) {
        if (r_queue.TASKS[i]->task_state == READY &&
            r_queue.TASKS[i]->task_priority > highest_prio) {
            highest_prio = r_queue.TASKS[i]->task_priority;
        }
    }

    for (i = 1; i <= r_queue.size; i++) {
        uint8_t idx = (r_queue.pos_task_running + i) % r_queue.size;
        if (r_queue.TASKS[idx]->task_state == READY &&
            r_queue.TASKS[idx]->task_priority == highest_prio) {
            return idx;
        }
    }

    return 0; // fallback: idle (prioridade 0, sempre READY)
}