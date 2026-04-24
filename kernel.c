#include "kernel.h"
#include "scheduler.h"
#include "user.h"
#include "hw.h"

// Fila de aptos
ready_queue_t r_queue;

// Chamadas de sistema
void os_delay(uint8_t time)
{
    DISABLE_ALL_INTERRUPTS();
    
    SAVE_CONTEXT(WAITING);
    r_queue.TASKS[r_queue.pos_task_running].task_delay = time;
    scheduler();
    RESTORE_CONTEXT();
    
    ENABLE_ALL_INTERRUPTS();
}

void os_create_task(uint8_t id, f_ptr func, uint8_t prior)
{
    tcb_t new_task;
    
    new_task.task_id        = id;
    new_task.task_delay     = 0;
    new_task.task_priority  = prior;
    new_task.task_ptr       = func;
    new_task.task_state     = READY;
    
    new_task.BSR_REG        = 0;
    new_task.FSR0H_REG      = 0;
    new_task.FSR0L_REG      = 0;
    new_task.FSR1H_REG      = 0;
    new_task.FSR1L_REG      = 0;
    new_task.FSR2H_REG      = 0;
    new_task.FSR2L_REG      = 0;
    new_task.PCLATH_REG     = 0;
    new_task.PCLATU_REG     = 0;
    new_task.PRODH_REG      = 0;
    new_task.PRODL_REG      = 0;
    new_task.STATUS_REG     = 0;
    new_task.TABLAT_REG     = 0;
    new_task.TBLPTRH_REG    = 0;
    new_task.TBLPTRL_REG    = 0;
    new_task.TBLPTRU_REG    = 0;
    new_task.W_REG          = 0;
    new_task.task_stack.stack_size = 0;
    
    // Insere nova tarefa na fila de aptos
    r_queue.TASKS[r_queue.size++] = new_task;
}

void os_yield()
{
    DISABLE_ALL_INTERRUPTS();
    
    SAVE_CONTEXT(READY);
    scheduler();
    RESTORE_CONTEXT();            
    
    ENABLE_ALL_INTERRUPTS();
}

void os_config()
{
    r_queue.size                = 0;
    r_queue.task_running        = &r_queue.TASKS[0];
    r_queue.pos_task_running    = 0;
    
    // Criar a tarefa idle
    os_create_task(1, idle, 0);
    asm("global _idle");
    
    config_user();   
}

void os_start()
{  
    setup_hardware();
    scheduler();
    RESTORE_CONTEXT();
    ENABLE_ALL_INTERRUPTS();
}

void os_task_change_state(state_t new_state, tcb_t *task_handle)
{
    DISABLE_ALL_INTERRUPTS();
    
    if (task_handle == NULL) {
        SAVE_CONTEXT(new_state);
        scheduler();
        RESTORE_CONTEXT();
    }
    else {
        task_handle->task_state = new_state;
    }
    
    ENABLE_ALL_INTERRUPTS();
}


TASK idle()
{
    TRISCbits.RC0 = 0;
    while (1) {
        //asm("NOP");
        PORTCbits.RC0 = ~PORTCbits.RC0;
    }
}
