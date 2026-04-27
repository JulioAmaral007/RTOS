#ifndef TYPES_H
#define	TYPES_H

#include <stdint.h>
#include "os_config.h"

typedef void TASK;

typedef enum {
    READY        = 0,  // apta a ser escalonada, aguardando o scheduler
    WAITING,           // bloqueada por os_delay(), aguardando contador zerar no ISR
    RUNNING,           // executando agora (apenas uma task por vez)
    WAITING_SEM,       // bloqueada em sem_wait(), aguardando sem_post()
    WAITING_MUTEX      // bloqueada em mutex_lock(), aguardando mutex_unlock()
} state_t;

typedef void (*f_ptr)(void);

// Uma entrada da pilha de hardware do PIC18 (endereço de retorno de 21 bits)
typedef struct hw_stack {
    uint8_t TOSL_REG;   // Top Of Stack Low   — byte menos significativo
    uint8_t TOSH_REG;   // Top Of Stack High  — byte do meio
    uint8_t TOSU_REG;   // Top Of Stack Upper — bits 20:16
} hw_stack_t;

// Cópia completa da pilha de hardware da task (até 31 níveis)
typedef struct sw_stack {
    hw_stack_t stack[MAX_STACK_SIZE];  // endereços de retorno salvos
    uint8_t stack_size;                // quantas entradas estão ocupadas
} sw_stack_t;

typedef struct tcb {
    uint8_t task_id;        // identificador único (índice na ready_queue)
    state_t task_state;     // estado atual da task
    f_ptr   task_ptr;       // ponteiro para a função da task
    uint8_t task_delay;     // ticks restantes de os_delay(), decrementado pelo ISR
    uint8_t task_priority;  // prioridade: maior número = mais prioritária

    // Registradores salvos no SAVE_CONTEXT e restaurados no RESTORE_CONTEXT
    uint8_t W_REG;       // acumulador principal, usado em quase toda operação ALU
    uint8_t STATUS_REG;  // flags: C, DC, Z, OV, N
    uint8_t BSR_REG;     // Bank Select Register — banco de RAM ativo (0-15)
    uint8_t PRODL_REG;   // byte baixo do resultado de MULWF
    uint8_t PRODH_REG;   // byte alto do resultado de MULWF
    uint8_t FSR0L_REG;   // FSR0 Low  — ponteiro de acesso indireto à RAM (INDF0)
    uint8_t FSR0H_REG;   // FSR0 High
    uint8_t FSR1L_REG;   // FSR1 Low  — usado pelo XC8 para passagem de parâmetros
    uint8_t FSR1H_REG;   // FSR1 High
    uint8_t FSR2L_REG;   // FSR2 Low  — frame pointer do XC8 (variáveis locais)
    uint8_t FSR2H_REG;   // FSR2 High
    uint8_t TABLAT_REG;  // dado lido pela instrução TBLRD (leitura de Flash)
    uint8_t TBLPTRL_REG; // endereço na Flash acessado por TBLRD — Low
    uint8_t TBLPTRH_REG; // endereço na Flash acessado por TBLRD — High
    uint8_t TBLPTRU_REG; // endereço na Flash acessado por TBLRD — Upper (até 2MB)
    uint8_t PCLATH_REG;  // PC Latch High  — necessário para GOTOs/CALLs longos
    uint8_t PCLATU_REG;  // PC Latch Upper — endereçamento de Flash > 64KB

    // Cópia da pilha de hardware (endereços de retorno de todas as chamadas ativas)
    sw_stack_t task_stack;
} tcb_t;

// Fila de aptos: contém todos os TCBs, incluindo a idle task (slot 0)
typedef struct ready_queue {
    tcb_t   TASKS[MAX_USER_TASKS+1]; // +1 reservado para a idle task (prio=0, sempre READY)
    uint8_t size;                    // número de tasks criadas (incluindo idle)
    tcb_t  *task_running;            // ponteiro direto para o TCB em execução
    uint8_t pos_task_running;        // índice dessa task em TASKS[]
} ready_queue_t;

#endif	/* TYPES_H */
