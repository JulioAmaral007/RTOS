#include "user.h"
#include <xc.h>
#include "kernel.h"
#include "sync.h"
#include "com.h"

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

TASK LED_2()
{
    while (1) {
        
        //sem_post(&controle_leitura);
        PORTCbits.RC7 = ~PORTCbits.RC7;
        //sem_post(&s);
        //os_delay(5);
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
