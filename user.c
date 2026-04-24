#include "user.h"
#include <xc.h>
#include "kernel.h"
#include "sync.h"
#include "com.h"
#include "io.h"

sem_t s;
pipe_t p;

void config_user()
{
    TRISCbits.RC6       = 0;
    TRISCbits.RC7       = 0;
    TRISDbits.RD0       = 0;
    ANSELDbits.ANSD0    = 0;
    ANSELCbits.ANSC6    = 0;
    ANSELCbits.ANSC7    = 0;
    
    asm("global _LED_1, _LED_2, _LED_3");
    
    sem_init(&s, 0);
    pipe_init(&p);

    pwm_init(1); // inicializa CCP1/RC2; sinal continuo gerado por hardware
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

TASK LED_1()
{
    //char *acionamento = SRAMAlloc(6);
    char acionamento[] = {'L', 'L', 'D', 'L', 'D', 'D'};
    uint8_t pos = 0;
    while (1) {        
        PORTCbits.RC6 = ~PORTCbits.RC6;
        pipe_write(&p, acionamento[pos]);
        pos = (pos + 1) % 6;
        //os_delay(5);
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
