#ifndef IO_H
#define IO_H

#include <xc.h>
#include <stdint.h>

void pwm_init(uint8_t channel);
void pwm_set_duty(uint8_t channel, uint16_t duty);

#endif /* IO_H */
