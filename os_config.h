#ifndef OS_CONFIG_H
#define	OS_CONFIG_H

#define _XTAL_FREQ 16000000UL  // 16 MHz — clock do sistema (necessario para __delay_*)

#define RR_SCHEDULER        1
#define PRIOR_SCHEDULER     2
#define RR_PRIOR_SCHEDULER  3

#define DEFAULT_SCHEDULER   RR_PRIOR_SCHEDULER

#define MAX_STACK_SIZE      31
#define MAX_USER_TASKS      4  // +1 para comportar a one-shot criada dinamicamente
#define QUANTUM             5
#define PIPE_MAX_SIZE       4

#endif	/* OS_CONFIG_H */

