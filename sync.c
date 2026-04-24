#include "sync.h"
#include "kernel.h"
#include "scheduler.h"

// Fila de aptos
extern ready_queue_t r_queue;


void sem_init(sem_t *sem, uint8_t valor)
{
    sem->contador           = valor;
    sem->pos_input          = 0;
    sem->pos_output         = 0;
}

void sem_wait(sem_t *sem)
{
    DISABLE_ALL_INTERRUPTS();
    
    sem->contador--;
    if (sem->contador < 0) {
        sem->fila[sem->pos_input] = r_queue.pos_task_running;
        sem->pos_input = (sem->pos_input + 1) % MAX_USER_TASKS;
        // Troca de contexto
        SAVE_CONTEXT(WAITING_SEM);
        scheduler();
        RESTORE_CONTEXT();
    }
    
    ENABLE_ALL_INTERRUPTS();
}

void sem_post(sem_t *sem)
{
    DISABLE_ALL_INTERRUPTS();
    
    sem->contador++;
    if (sem->contador <= 0) {
        // Libera o processo bloqueado a mais tempo
        r_queue.TASKS[sem->fila[sem->pos_output]].task_state = READY;
        sem->pos_output = (sem->pos_output + 1) % MAX_USER_TASKS;
    }
    
    ENABLE_ALL_INTERRUPTS();    
}
