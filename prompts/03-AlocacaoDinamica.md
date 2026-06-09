# Prompt — Etapa 3: Alocação Dinâmica de Estrutura do RTOS

Atue como engenheiro de sistemas embarcados especializado em gerenciamento de memória para RTOS em PIC18F.

Analise `RTOS/kernel.c`, `RTOS/types.h`, `RTOS/mem.c`, `RTOS/mem.h` e `RTOS/os_config.h` antes de qualquer modificação.

---

## Contexto do código atual

O RTOS declara o array de TCBs estaticamente em `kernel.c`:
```c
TCB_t tasks[MAX_USER_TASKS];
```
Este array ocupa memória SRAM permanentemente, independente de quantas tasks estão ativas.

O RTOS já possui um alocador dinâmico de SRAM em `mem.c/mem.h` com as funções:
- `SRAMInitHeap()` — inicializa o heap (deve ser chamada uma única vez, antes de qualquer alocação)
- `SRAMalloc(size)` — aloca `size` bytes; retorna ponteiro ou `NULL` em falha
- `SRAMfree(ptr)` — libera memória previamente alocada

---

## O que converter

**Alvo principal: o array de TCBs em `kernel.c`.**

Converter de:
```c
TCB_t tasks[MAX_USER_TASKS];   // alocação estática
```

Para alocação dinâmica por demanda: cada TCB é alocado quando `os_create_task()` é chamado e liberado quando a task termina (essencial para tasks one-shot da Etapa 6).

---

## Restrições críticas de memória

- Heap total disponível: **512 bytes**.
- Tamanho máximo de um único segmento: **126 bytes**.
- `sizeof(TCB_t)` deve ser verificado. Se exceder 126 bytes, o TCB precisa ser dividido (e.g., stack armazenado separadamente) ou o campo `MAX_STACK_SIZE` precisa ser ajustado.
- Com `MAX_STACK_SIZE = 31` entradas de `uint16_t` (2 bytes cada) = 62 bytes só para stack. Calcular o tamanho total do TCB antes de alocar.

---

## O que implementar

### 1. Inicialização do heap (`main.c`)
Chamar `SRAMInitHeap()` antes de `os_config()` e `os_start()`.

### 2. Alocação de TCB em `os_create_task()` (`kernel.c`)
```c
TCB_t *new_task = (TCB_t *) SRAMalloc(sizeof(TCB_t));
if (new_task == NULL) {
    // falha de alocação: não criar a task, retornar código de erro
    return ERROR_NO_MEMORY;
}
```
Substituir os acessos ao array estático pelo ponteiro alocado dinamicamente.

### 3. Manter array de ponteiros
Substituir `TCB_t tasks[MAX_USER_TASKS]` por `TCB_t *tasks[MAX_USER_TASKS]`. Os ponteiros são estáticos (pequenos), os TCBs são dinâmicos.

### 4. Liberação do TCB
Implementar (ou preparar para a Etapa 6) uma função `os_task_exit()` que:
- Marca a task como terminada.
- Chama `SRAMfree(tcb_ptr)` para liberar o TCB.
- Chama o scheduler para ceder a CPU.

---

## Tratamento de falha obrigatório

Quando `SRAMalloc()` retornar `NULL`:
- `os_create_task()` deve retornar um código de erro (não travar o sistema).
- Não tentar usar o ponteiro `NULL`.

---

## Regras

- Não criar um novo gerenciador de memória — usar exclusivamente `mem.c/mem.h`.
- `SRAMInitHeap()` deve ser chamado **exatamente uma vez** antes de qualquer `SRAMalloc()`.
- Não modificar a struct `TCB_t` desnecessariamente — apenas a forma de alocação muda.
- O resto do RTOS que acessa tasks por índice ou ponteiro deve ser atualizado para usar o array de ponteiros.

---

## Entrega

- `kernel.c` modificado com alocação dinâmica de TCBs
- `main.c` com chamada a `SRAMInitHeap()`
- Análise do tamanho do `TCB_t` vs. limite do heap
- Explicação do ciclo de vida do TCB (alocação → uso → liberação)
- Código da função `os_task_exit()` (mesmo que seja usada plenamente apenas na Etapa 6)
