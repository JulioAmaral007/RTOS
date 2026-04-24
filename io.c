#include "io.h"

// PWM via CCP1 + Timer2 no PIC18F46K22
// Configuracao: PR2=0xFF, prescaler 1:1
// Frequencia resultante: _XTAL_FREQ / (4 * 1 * (PR2+1)) = 16MHz / 1024 ~= 15.625 kHz
//
// Timer2 nao interfere no Timer0 do scheduler: cada modulo possui registradores
// proprios e independentes (T0CON vs T2CON, TMR0 vs TMR2, PR2 exclusivo do Timer2).
// A ISR do scheduler so testa INTCONbits.TMR0IF — o Timer2 nem gera interrupcao
// nesta configuracao, apenas alimenta o modulo CCP1 via hardware interno.

void pwm_init(uint8_t channel)
{
    if (channel != 1)
        return; // somente CCP1/RC2 implementado; outros canais sao no-op

    TRISCbits.TRISC2 = 0;    // RC2 como saida (pino do CCP1)
    CCP1CON          = 0b00001100; // modo PWM
    T2CON            = 0b00000100; // Timer2 ON, prescaler 1:1
    PR2              = 0xFF;       // periodo maximo → duty_max = 4*(255+1) = 1024
    CCPR1L           = 0;
    CCP1CONbits.DC1B = 0;    // duty inicial = 0%
}

void pwm_set_duty(uint8_t channel, uint16_t duty)
{
    if (channel != 1)
        return; // somente CCP1/RC2 implementado; outros canais sao no-op

    uint16_t duty_max = (uint16_t)((PR2 + 1) * 4); // 1024 quando PR2=0xFF
    if (duty > duty_max)
        duty = duty_max;

    CCPR1L           = (uint8_t)(duty >> 2);        // 8 MSBs
    CCP1CONbits.DC1B = (uint8_t)(duty & 0x03);      // 2 LSBs
}
