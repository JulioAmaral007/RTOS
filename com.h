#ifndef COM_H
#define	COM_H

#include "types.h"
#include "os_config.h"
#include "sync.h"


typedef struct pipe {
    char fila_dados[PIPE_MAX_SIZE];
    uint8_t pos_input;
    uint8_t pos_output;
    sem_t s_input;
    sem_t s_output;    
} pipe_t;

void pipe_init(pipe_t *p);
void pipe_read(pipe_t *p, char *dado);
void pipe_write(pipe_t *p, char dado);

#endif	/* COM_H */

