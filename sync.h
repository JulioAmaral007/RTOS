#ifndef SYNC_H
#define	SYNC_H

#include <stdint.h>
#include "os_config.h"

// Padr�o POSIX
typedef struct sem {
    int contador;
    uint8_t fila[MAX_USER_TASKS];
    uint8_t pos_input;
    uint8_t pos_output;
} sem_t;


void sem_init(sem_t *sem, uint8_t valor);
void sem_wait(sem_t *sem);
void sem_post(sem_t *sem);


// Mutex — exclusao mutua com ownership
// Apenas a task que chamou mutex_lock pode chamar mutex_unlock.

#define MUTEX_UNLOCKED  0
#define MUTEX_LOCKED    1
#define INVALID_OWNER   0xFF

typedef struct {
    uint8_t locked;
    uint8_t owner_pos;             // indice em r_queue.TASKS da task dona
    uint8_t fila[MAX_USER_TASKS];  // indices das tasks bloqueadas
    uint8_t pos_input;
    uint8_t pos_output;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif	/* SYNC_H */

