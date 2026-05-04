#ifndef IO_H
#define IO_H

#include <xc.h>
#include <stdint.h>

// PWM (CCP1/RC2)
void pwm_init(void);
void pwm_set_duty(uint16_t duty);

// ADC
void     adc_init(void);
uint16_t adc_read(void);

// Interrupcao externa
void ext_int_init(void);

#endif /* IO_H */
