#ifndef USER_H
#define	USER_H

#include "types.h"

void config_user(void);

TASK acionaMotor(void);
TASK ligaLed(void);
TASK apagaLed(void);


TASK LED_1(void);
TASK LED_2(void);
TASK LED_3(void);


#endif	/* USER_H */

