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
  #endif  
  r_queue.task_running     = &r_queue.TASKS[r_queue.pos_task_running];
}

uint8_t RR_scheduler()
{
    uint8_t prox = r_queue.pos_task_running, tentativas = 0;
    
    do {
        prox = (prox+1) % r_queue.size;
        tentativas++;
        if (tentativas >= (MAX_USER_TASKS+1)) return 0;
    } while (r_queue.TASKS[prox].task_state != READY ||
             r_queue.TASKS[prox].task_ptr == idle);
    
    return prox;
}

uint8_t priority_scheduler(void)
{
    uint8_t prox = r_queue.pos_task_running;
    
    while (r_queue.TASKS[prox].task_state != READY)
        prox = (prox + 1) % r_queue.size;
    
    uint8_t current_task = r_queue.TASKS[prox].task_priority;
    
    for(uint8_t i = 1; i < r_queue.size;i++){
        if(r_queue.TASKS[i].task_state == READY && 
           r_queue.TASKS[i].task_priority > current_task){
            prox = i;
            current_task = r_queue.TASKS[i].task_priority;
        }
    }
    
    return prox;    
}