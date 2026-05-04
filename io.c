#include "io.h"

// PWM via CCP1 + Timer2 no PIC18F46K22
void pwm_init(void)
{
    TRISCbits.TRISC2 = 0;    // RC2 como saida (pino do CCP1)
    CCP1CON          = 0b00001100; // modo PWM
    T2CON            = 0b00000100; // Timer2 ON, prescaler 1:1
    PR2              = 0xFF;       // periodo maximo → duty_max = 4*(255+1) = 1024
    CCPR1L           = 0;
    CCP1CONbits.DC1B = 0;    // duty inicial = 0%
}

void pwm_set_duty(uint16_t duty)
{
    uint16_t duty_max = (uint16_t)((PR2 + 1) * 4); // 1024 quando PR2=0xFF
    if (duty > duty_max)
        duty = duty_max;

    CCPR1L           = (uint8_t)(duty >> 2);        // 8 MSBs
    CCP1CONbits.DC1B = (uint8_t)(duty & 0b11);      // 2 LSBs
}

// ---------------------------------------------------------------------------
// ADC via modulo ADC do PIC18F46K22
// ---------------------------------------------------------------------------

void adc_init(void)
{
    TRISAbits.RA0    = 1;      // RA0 como entrada digital
    ANSELAbits.ANSA0 = 1;      // habilita funcao analogica em AN0 (obrigatorio)

    ADCON1bits.PVCFG = 0b00;   // Vref+ = VDD
    ADCON1bits.NVCFG = 0b00;   // Vref- = VSS

    ADCON2bits.ADFM  = 1;      // justificacao a direita
    ADCON2bits.ACQT  = 0b110;  // 16 TAD de aquisicao automatica
    ADCON2bits.ADCS  = 0b100;  // Fosc/64 → T_AD = 4 µs

    ADCON0bits.CHS   = 0b00000; // fixa canal AN0 (LM35 em RA0)
    ADCON0bits.ADON  = 1;      // liga modulo ADC
}

uint16_t adc_read(void)
{
    ADCON0bits.GO  = 1;        // inicia conversao
    while (ADCON0bits.GO);     // aguarda fim da conversao (~10 µs)
    return (uint16_t)((ADRESH << 8) | ADRESL);
}

// ---------------------------------------------------------------------------
// Interrupcao externa INT0 (RB0) do PIC18F46K22
// ---------------------------------------------------------------------------

void ext_int_init(void)
{
    TRISBbits.TRISB0    = 1;  // RB0 como entrada
    ANSELBbits.ANSB0    = 0;  // desabilita AN12 em RB0 (necessario para leitura digital)
    INTCON2bits.INTEDG0 = 0;  // borda de descida (botao com pull-up)
    INTCONbits.INT0IF   = 0;  // limpa flag antes de habilitar
    INTCONbits.INT0IE   = 1;  // habilita interrupcao INT0
}
