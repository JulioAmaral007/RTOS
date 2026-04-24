#include "hw.h"
#include "kernel.h"
#include "scheduler.h"
#include "os_config.h"
#include "user.h"

// Quantum do algoritmo Round-Robin
uint8_t rr_quantum = QUANTUM;


void setup_hardware(void)
{
    // Configura��o do timer
    INTCONbits.TMR0IE   = 1;
    INTCONbits.TMR0IF   = 0;
    T0CONbits.T08BIT    = 1; // 8 bits
    T0CONbits.T0CS      = 0; // Instru��o interna
    T0CONbits.PSA       = 0; // Ativa preescaler
    T0CONbits.T0PS      = 0b111; // 1:256
    T0CONbits.TMR0ON    = 1; // Ativa timer
    TMR0                = 0;
}

void __interrupt() ISR(void)
{
    // INT0 — botao em RB0 (borda de descida, pull-up)
    // Flag limpa ANTES de qualquer processamento: evita re-disparo se a borda
    // demorar a cair. one_shot_pending protege contra criacao multipla por rebote.
    if (INTCONbits.INT0IF) {
        INTCONbits.INT0IF = 0;
        if (!one_shot_pending) {
            one_shot_pending = 1;
            os_create_task(5, one_shot_task, 6); // id=5, prior=6 (maior que as tasks estaticas)
        }
    }

    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;
        
        // Verifica se tem tarefa com delay > 0
        for (int i = 0; i < r_queue.size; i++) {
            if (r_queue.TASKS[i]->task_delay > 0) {
                r_queue.TASKS[i]->task_delay--;
                if (r_queue.TASKS[i]->task_delay == 0) {
                    r_queue.TASKS[i]->task_state = READY;
                }
            }
        }
        
        // Verifica o quantum para saber se h� necessidade de 
        // mudar a tarefa que est� em execu��o.
        rr_quantum--;
        if (rr_quantum == 0) {
            rr_quantum = QUANTUM;
            SAVE_CONTEXT(READY);
            scheduler();
            RESTORE_CONTEXT();
        }
    }
}
