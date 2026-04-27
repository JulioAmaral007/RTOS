#include "user.h"
#include <xc.h>
#include "kernel.h"
#include "sync.h"
#include "com.h"
#include "io.h"

// Recurso compartilhado: temperatura atual em graus Celsius
static volatile uint8_t temp_global = 0;

// Mutex: protege acesso a temp_global entre task_display e task_pwm
static mutex_t m_temp;

// Semaforo: task_display posta para acordar task_pwm apos nova leitura
static sem_t s_new_data;

// Pipe: canal de comunicacao task_sensor → task_display
static pipe_t p_temp;

// Aliases de hardware para os LEDs do circuito
#define LED_LOW     LATEbits.LATE0   // RE0: temperatura < 25 graus C
#define LED_MID     LATEbits.LATE1   // RE1: temperatura 25-44 graus C
#define LED_HIGH    LATEbits.LATE2   // RE2: temperatura >= 45 graus C
#define LED_ALARM   LATDbits.LATD0   // RD0: pisca quando temperatura > 60 graus C
#define LED_ONESHOT LATDbits.LATD2   // RD2: feedback visual da task one-shot


void config_user(void)
{
    // LEDs de faixa de temperatura (PORTE - pinos analogicos → digital + saida)
    TRISEbits.RE0    = 0;
    ANSELEbits.ANSE0 = 0;
    TRISEbits.RE1    = 0;
    ANSELEbits.ANSE1 = 0;
    TRISEbits.RE2    = 0;
    ANSELEbits.ANSE2 = 0;

    // LED de alarme e LED de feedback da one-shot (PORTD)
    TRISDbits.RD0    = 0;
    ANSELDbits.ANSD0 = 0;
    TRISDbits.RD2    = 0;
    ANSELDbits.ANSD2 = 0;

    // Estado inicial: todos LEDs apagados
    LATE        = 0x00;
    LED_ALARM   = 0;
    LED_ONESHOT = 0;

    // Declara simbolos visiveis ao linker (necessario para ponteiros de funcao
    // armazenados em TCBs alocados em runtime com SRAMalloc)
    asm("global _task_sensor, _task_display, _task_pwm, _one_shot_task");

    // Inicializa primitivas RTOS
    mutex_init(&m_temp);
    sem_init(&s_new_data, 0);
    pipe_init(&p_temp);

    // Inicializa perifericos de hardware
    pwm_init(1);           // CCP1/RC2: LED com brilho proporcional a temperatura
    adc_init();            // AN0/RA0: entrada do sensor LM35
    ext_int_init(0, 0);    // INT0/RB0: borda de descida, botao com pull-up
}


// task_sensor  — prioridade 5
// Le o LM35 via ADC a cada 10 ticks e envia a temperatura em graus C
// pelo pipe para task_display.
// temp_C = raw * 125 / 256  (equivale a raw * 500 / 1024, so em aritmetica 16-bit)
// LM35: 10 mV/grau C, VDD = 5 V, ADC 10-bit → 1 LSB ≈ 4,88 mV
TASK task_sensor(void)
{
    while (1) {
        uint16_t raw  = adc_read(0);
        uint8_t  temp = (uint8_t)((raw * 125U) >> 8);
        pipe_write(&p_temp, (char)temp);
        os_delay(10);
    }
}


// task_display  — prioridade 5  (Round-Robin com task_sensor e task_pwm)
// Consome temperatura do pipe, atualiza temp_global com mutex e:
//   - Acende o LED de faixa correspondente (RE0/RE1/RE2)
//   - Pisca LED_ALARM (RD0) se temperatura > 60 graus C (verificacao inline)
//   - Posta semaforo para despertar task_pwm
TASK task_display(void)
{
    char dado;
    while (1) {
        pipe_read(&p_temp, &dado);   // bloqueia ate dado disponivel no pipe
        uint8_t t = (uint8_t)dado;

        mutex_lock(&m_temp);
        temp_global = t;
        mutex_unlock(&m_temp);

        // Indicacao por faixa de temperatura — somente 3 LEDs, exclusivos
        LED_LOW  = (t < 25)            ? 1 : 0;  // RE0: frio
        LED_MID  = (t >= 25 && t < 45) ? 1 : 0;  // RE1: medio
        LED_HIGH = (t >= 45)            ? 1 : 0;  // RE2: quente

        // Alarme de sobretemperatura: pisca RD0 enquanto t > 60°C
        if (t > 60) LED_ALARM = ~LED_ALARM;
        else        LED_ALARM = 0;

        sem_post(&s_new_data);   // notifica task_pwm
    }
}


// task_pwm  — prioridade 5  (Round-Robin com task_sensor e task_display)
// Aguarda semaforo, le temp_global com mutex e ajusta o duty cycle do
// LED em RC2/CCP1 de forma proporcional a temperatura:
//   0 grau C  → duty = 0     (0% brilho)
//   79 graus C → duty = 1024  (100% brilho)
// t * 13 evita divisao de 32 bits: 13 ≈ 1024/79, resultado em uint16_t
TASK task_pwm(void)
{
    while (1) {
        sem_wait(&s_new_data);      // bloqueia ate task_display sinalizar

        mutex_lock(&m_temp);
        uint8_t t = temp_global;
        mutex_unlock(&m_temp);

        uint16_t duty = (uint16_t)t * 13U;
        if (duty > 1024U) duty = 1024U;
        pwm_set_duty(1, duty);
    }
}


// one_shot_task  — prioridade 6 (mais alta; preempta todas as demais)
// Criada pela ISR ao detectar borda de descida em INT0/RB0 (botao com pull-up).
// Executa exatamente uma vez: debounce, feedback visual em RD2, encerra-se.
TASK one_shot_task(void)
{
    os_delay(2);                // aguarda ~2 ticks de Timer0 (debounce)
    LED_ONESHOT = ~LED_ONESHOT; // toggle: INT0 confirmou a borda, r_queue.size garante unicidade
    os_task_exit();             // remove TCB, libera slot, aciona scheduler
}
