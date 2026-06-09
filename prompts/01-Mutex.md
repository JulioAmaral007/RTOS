# Prompt — Etapa 1: Implementação de Mutex

Atue como engenheiro de sistemas embarcados especialista em RTOS para PIC18F.

Analise completamente os arquivos `RTOS/sync.c`, `RTOS/sync.h` e `RTOS/types.h` antes de qualquer implementação.

---

## Contexto do código atual

O RTOS já possui semáforos binários implementados em `sync.c/h` com as funções `sem_init()`, `sem_wait()` e `sem_post()`, e o estado `WAITING_SEM` no TCB definido em `types.h`. O mutex deve seguir o mesmo padrão estrutural, mas com semântica diferente.

---

## Diferença obrigatória entre mutex e semáforo

O semáforo existente é um mecanismo de sinalização. O mutex é um mecanismo de exclusão mútua com **ownership**:
- Apenas a task que executou `mutex_lock()` pode executar `mutex_unlock()`.
- Se outra task tentar dar `mutex_unlock()` em um mutex que não é dela, a operação deve ser ignorada ou gerar erro.
- Isso previne race conditions que o semáforo binário não previne.

---

## O que implementar

### 1. Novo estado no TCB (`types.h`)
Adicionar o estado `WAITING_MUTEX` no enum de estados de task, ao lado de `WAITING_SEM`.

### 2. Struct do mutex (`sync.h`)
```c
typedef struct {
    uint8_t locked;          // 0 = livre, 1 = travado
    uint8_t owner_id;        // ID da task dona do mutex
    TCB_t *waiting_queue;    // fila de tasks bloqueadas
} mutex_t;
``` 

### 3. API do mutex (`sync.h` e `sync.c`)
Implementar as seguintes funções:

```c
void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
```

**`mutex_init`**: inicializa `locked = 0`, `owner_id = INVALID_TASK_ID`, `waiting_queue = NULL`.

**`mutex_lock`**:
- Se mutex livre: marcar como travado, registrar `owner_id` com ID da task atual, retornar.
- Se mutex travado: colocar task atual em `WAITING_MUTEX`, adicionar à `waiting_queue`, chamar scheduler.

**`mutex_unlock`**:
- Verificar se a task chamadora é a `owner_id`. Se não for, retornar sem fazer nada.
- Se há tasks na `waiting_queue`: transferir ownership para a primeira task da fila, colocá-la em `READY`.
- Se não há tasks esperando: liberar o mutex (`locked = 0`, `owner_id = INVALID_TASK_ID`).

### 4. Integração com o scheduler
Quando `mutex_lock()` bloqueia a task atual:
- Alterar o estado da task para `WAITING_MUTEX` no TCB.
- Chamar o scheduler para ceder a CPU imediatamente.
- O retorno da função só ocorre quando a task for acordada pelo `mutex_unlock()`.

---

## Regras de implementação

- Seguir exatamente o padrão de `sem_wait()` e `sem_post()` para estrutura de código.
- Não reescrever o scheduler.
- Desabilitar interrupções (INTCON) ao acessar campos do mutex para garantir atomicidade, restaurar ao sair.
- Não criar gerenciador de filas novo — reutilizar o padrão da fila de semáforos.

---

## Entrega

- Modificações em `types.h` (novo estado)
- Implementação em `sync.h` e `sync.c`
- Explicação do comportamento de ownership
- Demonstração com um exemplo de duas tasks competindo pelo mesmo mutex
