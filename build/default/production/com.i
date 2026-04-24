# 1 "com.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 295 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include/language_support.h" 1 3
# 2 "<built-in>" 2
# 1 "com.c" 2
# 1 "./com.h" 1



# 1 "./types.h" 1



# 1 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/stdint.h" 1 3



# 1 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/musl_xc8.h" 1 3
# 5 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/stdint.h" 2 3
# 26 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/stdint.h" 3
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/alltypes.h" 1 3
# 133 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned __int24 uintptr_t;
# 148 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/alltypes.h" 3
typedef __int24 intptr_t;
# 164 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/alltypes.h" 3
typedef signed char int8_t;




typedef short int16_t;




typedef __int24 int24_t;




typedef long int32_t;





typedef long long int64_t;
# 194 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/alltypes.h" 3
typedef long long intmax_t;





typedef unsigned char uint8_t;




typedef unsigned short uint16_t;




typedef __uint24 uint24_t;




typedef unsigned long uint32_t;





typedef unsigned long long uint64_t;
# 235 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/alltypes.h" 3
typedef unsigned long long uintmax_t;
# 27 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/stdint.h" 2 3

typedef int8_t int_fast8_t;

typedef int64_t int_fast64_t;


typedef int8_t int_least8_t;
typedef int16_t int_least16_t;

typedef int24_t int_least24_t;
typedef int24_t int_fast24_t;

typedef int32_t int_least32_t;

typedef int64_t int_least64_t;


typedef uint8_t uint_fast8_t;

typedef uint64_t uint_fast64_t;


typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;

typedef uint24_t uint_least24_t;
typedef uint24_t uint_fast24_t;

typedef uint32_t uint_least32_t;

typedef uint64_t uint_least64_t;
# 148 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/stdint.h" 3
# 1 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/bits/stdint.h" 1 3
typedef int16_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint16_t uint_fast16_t;
typedef uint32_t uint_fast32_t;
# 149 "C:\\Program Files\\Microchip\\xc8\\v3.10\\pic\\include\\c99/stdint.h" 2 3
# 5 "./types.h" 2
# 1 "./os_config.h" 1
# 6 "./types.h" 2

typedef void TASK;

typedef enum {READY = 0,
              WAITING,
              RUNNING,
              WAITING_SEM
             } state_t;

typedef void (*f_ptr)(void);

typedef struct hw_stack {
    uint8_t TOSL_REG;
    uint8_t TOSH_REG;
    uint8_t TOSU_REG;
} hw_stack_t;

typedef struct sw_stack {
    hw_stack_t stack[31];
    uint8_t stack_size;
} sw_stack_t;

typedef struct tcb {
    uint8_t task_id;
    state_t task_state;

    f_ptr task_ptr;
    uint8_t task_delay;
    uint8_t task_priority;

    uint8_t W_REG;
    uint8_t STATUS_REG;
    uint8_t BSR_REG;
    uint8_t PRODL_REG;
    uint8_t PRODH_REG;
    uint8_t FSR0L_REG;
    uint8_t FSR0H_REG;
    uint8_t FSR1L_REG;
    uint8_t FSR1H_REG;
    uint8_t FSR2L_REG;
    uint8_t FSR2H_REG;
    uint8_t TABLAT_REG;
    uint8_t TBLPTRL_REG;
    uint8_t TBLPTRH_REG;
    uint8_t TBLPTRU_REG;
    uint8_t PCLATH_REG;
    uint8_t PCLATU_REG;


    sw_stack_t task_stack;
} tcb_t;


typedef struct ready_queue {
    tcb_t TASKS[3 +1];
    uint8_t size;
    tcb_t *task_running;
    uint8_t pos_task_running;
} ready_queue_t;
# 5 "./com.h" 2

# 1 "./sync.h" 1







typedef struct sem {
    int contador;
    uint8_t fila[3];
    uint8_t pos_input;
    uint8_t pos_output;
} sem_t;


void sem_init(sem_t *sem, uint8_t valor);
void sem_wait(sem_t *sem);
void sem_post(sem_t *sem);
# 7 "./com.h" 2


typedef struct pipe {
    char fila_dados[4];
    uint8_t pos_input;
    uint8_t pos_output;
    sem_t s_input;
    sem_t s_output;
} pipe_t;

void pipe_init(pipe_t *p);
void pipe_read(pipe_t *p, char *dado);
void pipe_write(pipe_t *p, char dado);
# 2 "com.c" 2

void pipe_init(pipe_t *p)
{
    p->pos_input = 0;
    p->pos_output = 0;
    sem_init(&p->s_input, 4);
    sem_init(&p->s_output, 0);
}

void pipe_write(pipe_t *p, char dado)
{
    sem_wait(&p->s_input);
    p->fila_dados[p->pos_input] = dado;
    p->pos_input = (p->pos_input + 1) % 4;
    sem_post(&p->s_output);
}

void pipe_read(pipe_t *p, char *dado)
{
    sem_wait(&p->s_output);
    *dado = p->fila_dados[p->pos_output];
    p->pos_output = (p->pos_output + 1) % 4;
    sem_post(&p->s_input);
}
