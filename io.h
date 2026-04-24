#ifndef IO_H
#define IO_H

#include <xc.h>
#include <stdint.h>

// PWM (CCP1/RC2)
void pwm_init(uint8_t channel);
void pwm_set_duty(uint8_t channel, uint16_t duty);

// ADC
void     adc_init(void);
uint16_t adc_read(uint8_t channel);  // channel: 0..12 → AN0..AN12

// Interrupcao externa
void ext_int_init(uint8_t int_pin, uint8_t edge);  // edge: 0=descida, 1=subida

#endif /* IO_H */
