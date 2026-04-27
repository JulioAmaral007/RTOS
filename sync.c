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

// ---------------------------------------------------------------------------
// Mutex
// ---------------------------------------------------------------------------

void mutex_init(mutex_t *m)
{
    m->locked     = MUTEX_UNLOCKED;
    m->owner_pos  = INVALID_OWNER;
    m->pos_input  = 0;
    m->pos_output = 0;
}

void mutex_lock(mutex_t *m)
{
    DISABLE_ALL_INTERRUPTS();

    if (m->locked == MUTEX_UNLOCKED) {
        // Mutex livre: adquirir imediatamente
        m->locked    = MUTEX_LOCKED;
        m->owner_pos = r_queue.pos_task_running;
    } else if (m->owner_pos == r_queue.pos_task_running) {
        // A task ja e a dona — evita auto-deadlock, retorna sem bloquear
    } else {
        // Mutex ocupado por outra task: bloquear o chamador
        m->fila[m->pos_input] = r_queue.pos_task_running;
        m->pos_input = (m->pos_input + 1) % MAX_USER_TASKS;
        SAVE_CONTEXT(WAITING_MUTEX);
        scheduler();
        RESTORE_CONTEXT();
        // Quando executar novamente, ownership ja foi transferido por mutex_unlock
    }

    ENABLE_ALL_INTERRUPTS();
}

void mutex_unlock(mutex_t *m)
{
    DISABLE_ALL_INTERRUPTS();

    // Apenas o dono pode desbloquear
    if (m->owner_pos != r_queue.pos_task_running) {
        ENABLE_ALL_INTERRUPTS();
        return;
    }

    if (m->pos_input != m->pos_output) {
        // Transferir ownership para a task bloqueada ha mais tempo
        m->owner_pos  = m->fila[m->pos_output];
        m->pos_output = (m->pos_output + 1) % MAX_USER_TASKS;
        r_queue.TASKS[m->owner_pos].task_state = READY;
        // mutex permanece LOCKED com o novo dono
    } else {
        // Sem tasks esperando: liberar o mutex
        m->locked    = MUTEX_UNLOCKED;
        m->owner_pos = INVALID_OWNER;
    }

    ENABLE_ALL_INTERRUPTS();
}
