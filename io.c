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

// ---------------------------------------------------------------------------
// ADC via modulo ADC do PIC18F46K22
// _XTAL_FREQ = 16 MHz → ADCS = Fosc/32 → T_AD = 2 µs (minimo exigido: 1 µs)
// ACQT = 16 TAD → aquisicao automatica de 32 µs; sem __delay_us adicional necessario
// Vref+ = VDD (~5 V), Vref- = VSS; resultado justificado a direita (10 bits)
// ---------------------------------------------------------------------------

void adc_init(void)
{
    TRISAbits.RA0    = 1;      // RA0 como entrada digital
    ANSELAbits.ANSA0 = 1;      // habilita funcao analogica em AN0 (obrigatorio)

    ADCON1bits.PVCFG = 0b00;   // Vref+ = VDD
    ADCON1bits.NVCFG = 0b00;   // Vref- = VSS

    ADCON2bits.ADFM  = 1;      // justificacao a direita
    ADCON2bits.ACQT  = 0b110;  // 16 TAD de aquisicao automatica
    ADCON2bits.ADCS  = 0b010;  // Fosc/32 → T_AD = 2 µs @ 16 MHz

    ADCON0bits.ADON  = 1;      // liga modulo ADC
}

uint16_t adc_read(uint8_t channel)
{
    ADCON0bits.CHS = channel;  // seleciona canal analogico
    ADCON0bits.GO  = 1;        // inicia conversao
    while (ADCON0bits.GO);     // aguarda fim da conversao (~10 µs)
    return (uint16_t)((ADRESH << 8) | ADRESL);
}

// ---------------------------------------------------------------------------
// Interrupcao externa INT0 (RB0) do PIC18F46K22
// GIE nao e habilitado aqui: INT0 usa apenas GIE, que ja e habilitado por
// os_start() via ENABLE_ALL_INTERRUPTS(). Habilitar GIE antes de os_start()
// causaria interrupcoes antes do scheduler estar pronto.
// ---------------------------------------------------------------------------

void ext_int_init(uint8_t int_pin, uint8_t edge)
{
    if (int_pin == 0) {
        TRISBbits.TRISB0    = 1;    // RB0 como entrada
        ANSELBbits.ANSB0    = 0;    // desabilita AN12 em RB0 (necessario para leitura digital)
        INTCON2bits.INTEDG0 = edge; // borda: 0=descida (botao pull-up), 1=subida
        INTCONbits.INT0IF   = 0;    // limpa flag antes de habilitar
        INTCONbits.INT0IE   = 1;    // habilita interrupcao INT0
    }
    // INT1/INT2: nao usados neste projeto (no-op)
}
