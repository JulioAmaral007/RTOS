/*
 * Sistema de Monitoramento de Temperatura — RTOS PIC18F46K22
 * Disciplina: Sistemas Operacionais Embarcados — DEC7562 — UFSC 2026/1
 *
 * Tasks permanentes (prioridade 5, Round-Robin entre si):
 *   task_sensor  — le LM35 via ADC, envia temperatura ao pipe
 *   task_display — consome pipe, atualiza temp_global (mutex), acende LEDs,
 *                  pisca alarme (>60°C), posta semaforo para task_pwm
 *   task_pwm     — aguarda semaforo, le temp_global (mutex), ajusta PWM
 *
 * Task transiente (prioridade 6, criada por ISR):
 *   one_shot_task — debounce + feedback RD2 + os_task_exit()
 *
 * Task de sistema (prioridade 0):
 *   idle          — criada por os_config(), togglea RC0
 *
 * Total: 4 ta+ 3 user) + 1 one-shot dinamica = 5 tasks
 */

#include "kernel.h"
#include "user.h"
#include "mem.h"

int main(void)
{
    SRAMInitHeap();
    os_config();   // cria idle (prio=0) e chama config_user()

    // Tres tasks de mesma prioridade → Round-Robin entre si
    os_create_task(2, task_sensor,  5);
    os_create_task(3, task_display, 5);
    os_create_task(4, task_pwm,     5);

    // one_shot_task (prio=6) e criada dinamicamente pela ISR de INT0/RB0

    os_start();

    while (1) {}
    return 0;
}
