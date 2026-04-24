#include "user.h"
#include <xc.h>
#include "kernel.h"
#include "sync.h"
#include "com.h"
#include "io.h"

sem_t s;
pipe_t p;
volatile uint8_t one_shot_pending = 0;

void config_user()
{
    TRISCbits.RC6       = 0;
    TRISCbits.RC7       = 0;
    TRISDbits.RD0       = 0;
    TRISDbits.RD2       = 0;    // RD2: LED de feedback da one-shot
    ANSELDbits.ANSD0    = 0;
    ANSELDbits.ANSD2    = 0;
    ANSELCbits.ANSC6    = 0;
    ANSELCbits.ANSC7    = 0;
    
    asm("global _LED_1, _LED_2, _LED_3");
    
    sem_init(&s, 0);
    pipe_init(&p);

    pwm_init(1);           // inicializa CCP1/RC2; sinal continuo gerado por hardware
    adc_init();            // inicializa modulo ADC; deve ser chamado uma unica vez
    ext_int_init(0, 0);   // INT0/RB0, borda de descida (botao com pull-up)
}

TASK acionaMotor()
{
    while (1) {
        
    }
}

TASK ligaLed()
{
    while (1) {
        
    }    
}

TASK apagaLed()
{
    while (1) {
        
    }    
}

// Le AN0 e envia 'L' (liga) se tensao >= 2.5 V, 'D' (desliga) caso contrario.
// LED_3 consome o pipe e aciona RD0 conforme o comando recebido.
TASK LED_1()
{
    while (1) {
        uint16_t raw = adc_read(0);          // le canal AN0
        char cmd = (raw >= 512) ? 'L' : 'D'; // threshold: ~2.5 V (512/1023 * 5V)
        PORTCbits.RC6 = ~PORTCbits.RC6;
        pipe_write(&p, cmd);
        os_delay(5);
    }
}

// Rampa de duty cycle: 0% -> 100% -> 0% em passos de 1/8 do maximo
TASK LED_2()
{
    uint16_t duty = 0;
    uint8_t increasing = 1;
    while (1) {
        pwm_set_duty(1, duty);
        os_delay(20);
        if (increasing) {
            duty += 128;
            if (duty >= 1024) { duty = 1024; increasing = 0; }
        } else {
            if (duty < 128) { duty = 0; increasing = 1; }
            else duty -= 128;
        }
    }
}

TASK LED_3()
{
    char dado;
    while (1) {
        pipe_read(&p, &dado);
        if (dado == 'L')
            PORTDbits.RD0 = 1;
        else if (dado == 'D')
            PORTDbits.RD0 = 0;
        //os_delay(1);
        //os_task_change_state(WAITING, NULL);
    }
}

// Task one-shot criada pela ISR ao detectar INT0 (botao em RB0 com pull-up).
// Executa uma unica vez: faz debounce, valida o evento e encerra via os_task_exit().
TASK one_shot_task(void)
{
    os_delay(2);                            // debounce: aguarda ~2 ticks do scheduler
    if (PORTBbits.RB0 == 0) {              // valida: botao ainda pressionado?
        LATDbits.LATD2 = ~LATDbits.LATD2; // feedback visual em RD2
    }
    one_shot_pending = 0;  // libera flag antes de sair (permite nova criacao)
    os_task_exit();        // remove TCB da fila, SRAMfree, chama scheduler
}
