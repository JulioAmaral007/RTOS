#include "com.h"
#include "mem.h"
#include <stddef.h>

void pipe_init(pipe_t *p)
{
    p->fila_dados = (char *) SRAMalloc(PIPE_MAX_SIZE);
    if (p->fila_dados == NULL) return;
    p->capacity   = PIPE_MAX_SIZE;
    p->pos_input  = 0;
    p->pos_output = 0;
    sem_init(&p->s_input,  PIPE_MAX_SIZE);
    sem_init(&p->s_output, 0);
}

void pipe_destroy(pipe_t *p)
{
    SRAMfree((unsigned char *) p->fila_dados);
    p->fila_dados = NULL;
    p->capacity   = 0;
}

void pipe_write(pipe_t *p, char dado)
{
    sem_wait(&p->s_input);
    p->fila_dados[p->pos_input] = dado;
    p->pos_input = (p->pos_input + 1) % p->capacity;
    sem_post(&p->s_output);
}

void pipe_read(pipe_t *p, char *dado)
{
    sem_wait(&p->s_output);
    *dado = p->fila_dados[p->pos_output];
    p->pos_output = (p->pos_output + 1) % p->capacity;
    sem_post(&p->s_input);
}
