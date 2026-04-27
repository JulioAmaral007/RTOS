#ifndef OS_CONFIG_H
#define	OS_CONFIG_H

#define _XTAL_FREQ 4000000UL   // 4 MHz — clock do sistema (necessario para __delay_*)

#define RR_SCHEDULER        1
#define PRIOR_SCHEDULER     2
#define RR_PRIOR_SCHEDULER  3

#define DEFAULT_SCHEDULER   RR_PRIOR_SCHEDULER

#define MAX_STACK_SIZE      31   // 5 TCBs estaticos x ~118 bytes = ~590 bytes em SRAM de dados
#define MAX_USER_TASKS      4    // idle + 3 user tasks + 1 one-shot = 5 slots
#define QUANTUM             5
#define PIPE_MAX_SIZE       4

#endif	/* OS_CONFIG_H */

