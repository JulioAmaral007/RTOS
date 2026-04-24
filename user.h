#ifndef USER_H
#define	USER_H

#include "types.h"
#include <stdint.h>

void config_user(void);

TASK acionaMotor(void);
TASK ligaLed(void);
TASK apagaLed(void);

TASK LED_1(void);
TASK LED_2(void);
TASK LED_3(void);

TASK one_shot_task(void);
extern volatile uint8_t one_shot_pending;

#endif	/* USER_H */

