#include "com.h"

void pipe_init(pipe_t *p)
{
    p->pos_input    = 0;
    p->pos_output   = 0;
    sem_init(&p->s_input, PIPE_MAX_SIZE);
    sem_init(&p->s_output, 0);
}

void pipe_write(pipe_t *p, char dado)
{
    sem_wait(&p->s_input);
    p->fila_dados[p->pos_input] = dado;
    p->pos_input = (p->pos_input + 1) % PIPE_MAX_SIZE;
    sem_post(&p->s_output);
}

void pipe_read(pipe_t *p, char *dado)
{
    sem_wait(&p->s_output);
    *dado = p->fila_dados[p->pos_output];
    p->pos_output = (p->pos_output + 1) % PIPE_MAX_SIZE;
    sem_post(&p->s_input);
}
