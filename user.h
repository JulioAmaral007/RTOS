#ifndef USER_H
#define USER_H

#include "types.h"
#include <stdint.h>

void config_user(void);

TASK task_sensor(void);
TASK task_display(void);
TASK task_pwm(void);
TASK one_shot_task(void);

#endif /* USER_H */
