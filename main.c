/*
 RTOS - Turma 2026/1
 */

#include "kernel.h"
#include "user.h"

int main()
{
    os_config();
    
    //os_create_task(2, acionaMotor, 5);
    //os_create_task(3, ligaLed, 5);
    //os_create_task(4, apagaLed, 5);
    
    os_create_task(2, LED_1, 5);
    os_create_task(3, LED_2, 5);
    os_create_task(4, LED_3, 5);
   
    os_start();
    
    while (1) {
        
    }
    
    return 0;
}
