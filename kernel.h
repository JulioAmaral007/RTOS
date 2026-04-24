#ifndef KERNEL_H
#define	KERNEL_H

#include "types.h"
#include <xc.h>

// Fila de aptos
extern ready_queue_t r_queue;

// Quantum do algoritmo Round-Robin
extern uint8_t rr_quantum;


// Chamadas de sistema
void os_delay(uint8_t time);
void os_create_task(uint8_t id, f_ptr func, uint8_t prior);
void os_yield(void);
void os_config(void);
void os_start(void);
void os_task_change_state(state_t new_state, tcb_t *task_handle);

TASK idle();

#define DISABLE_ALL_INTERRUPTS() INTCONbits.GIE = 0;
#define ENABLE_ALL_INTERRUPTS() INTCONbits.GIE = 1;

#define SAVE_CONTEXT(state) \
do { \
    if (r_queue.task_running->task_state == RUNNING) { \
        r_queue.task_running->task_state            = state; \
        r_queue.task_running->BSR_REG               = BSR; \
        r_queue.task_running->FSR0H_REG             = FSR0H; \
        r_queue.task_running->FSR0L_REG             = FSR0L; \
        r_queue.task_running->FSR1H_REG             = FSR1H; \
        r_queue.task_running->FSR1L_REG             = FSR1L; \
        r_queue.task_running->FSR2H_REG             = FSR2H; \
        r_queue.task_running->FSR2L_REG             = FSR2L; \
        r_queue.task_running->PCLATH_REG            = PCLATH; \
        r_queue.task_running->PCLATU_REG            = PCLATU; \
        r_queue.task_running->PRODH_REG             = PRODH; \
        r_queue.task_running->PRODL_REG             = PRODL; \
        r_queue.task_running->TABLAT_REG            = TABLAT; \
        r_queue.task_running->TBLPTRH_REG           = TBLPTRH; \
        r_queue.task_running->TBLPTRL_REG           = TBLPTRL; \
        r_queue.task_running->TBLPTRU_REG           = TBLPTRU; \
        r_queue.task_running->W_REG                 = WREG; \
        r_queue.task_running->STATUS_REG            = STATUS; \
        r_queue.task_running->task_stack.stack_size = 0; \
        while (STKPTR) { \
            r_queue.task_running->task_stack.stack[r_queue.task_running->task_stack.stack_size].TOSL_REG   = TOSL; \
            r_queue.task_running->task_stack.stack[r_queue.task_running->task_stack.stack_size].TOSH_REG   = TOSH; \
            r_queue.task_running->task_stack.stack[r_queue.task_running->task_stack.stack_size].TOSU_REG   = TOSU; \
            r_queue.task_running->task_stack.stack_size += 1; \
            asm("POP"); \
        } \
    } \
} while (0); \


#define RESTORE_CONTEXT() \
do { \
    if (r_queue.task_running->task_state == READY) { \
        r_queue.task_running->task_state    = RUNNING; \
        BSR                                 = r_queue.task_running->BSR_REG; \
        FSR0H                               = r_queue.task_running->FSR0H_REG; \
        FSR0L                               = r_queue.task_running->FSR0L_REG; \
        FSR1H                               = r_queue.task_running->FSR1H_REG; \
        FSR1L                               = r_queue.task_running->FSR1L_REG; \
        FSR2H                               = r_queue.task_running->FSR2H_REG; \
        FSR2L                               = r_queue.task_running->FSR2L_REG; \
        PCLATH                              = r_queue.task_running->PCLATH_REG; \
        PCLATU                              = r_queue.task_running->PCLATU_REG; \
        PRODH                               = r_queue.task_running->PRODH_REG; \
        PRODL                               = r_queue.task_running->PRODL_REG; \
        TABLAT                              = r_queue.task_running->TABLAT_REG; \
        TBLPTRH                             = r_queue.task_running->TBLPTRH_REG; \
        TBLPTRL                             = r_queue.task_running->TBLPTRL_REG; \
        TBLPTRU                             = r_queue.task_running->TBLPTRU_REG; \
        STATUS                              = r_queue.task_running->STATUS_REG; \
        WREG                                = r_queue.task_running->W_REG; \
        STKPTR                              = 0; \
        if (r_queue.task_running->task_stack.stack_size > 0) { \
            do { \
                asm("PUSH"); \
                r_queue.task_running->task_stack.stack_size -= 1; \
                TOSL = r_queue.task_running->task_stack.stack[r_queue.task_running->task_stack.stack_size].TOSL_REG; \
                TOSH = r_queue.task_running->task_stack.stack[r_queue.task_running->task_stack.stack_size].TOSH_REG; \
                TOSU = r_queue.task_running->task_stack.stack[r_queue.task_running->task_stack.stack_size].TOSU_REG; \
            } while (r_queue.task_running->task_stack.stack_size); \
        } \
        else { \
            asm("PUSH"); \
            TOSL = (uint8_t)((uint24_t)r_queue.task_running->task_ptr & 0xFF); \
            TOSH = (uint8_t)(((uint24_t)r_queue.task_running->task_ptr >> 8) & 0xFF); \
            TOSU = (uint8_t)(((uint24_t)r_queue.task_running->task_ptr >> 16) & 0xFF); \
        } \
        rr_quantum = QUANTUM; \
    } \
} while (0); \

#endif	/* KERNEL_H */

