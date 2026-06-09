# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Embedded systems coursework for "Sistemas Operacionais Embarcados" targeting the **PIC18F46K22** microcontroller. Contains two sub-projects:

- **RTOS/** — A preemptive multitasking RTOS kernel built from scratch
- **API_ADC.X/** — Standalone ADC firmware demonstrating temperature-based LED control

## Build System

Both projects use **MPLAB X IDE** (v6.30) with the **XC8 v3.10** compiler. Builds are driven by generated Makefiles under `nbproject/`.

```bash
# From within a project directory (RTOS/ or API_ADC.X/)
make build    # compile default configuration
make clean    # remove object files
make clobber  # remove all build artifacts
```

Output lands in `dist/default/production/` (`.hex`, `.elf`, `.map`, `.lst`).

There is no automated test runner — verification is done via **Proteus simulation** (`.pdsprj` files) or physical hardware.

## RTOS Architecture

The RTOS is a preemptive microkernel with these layers:

### Kernel (`kernel.c/h`)
Entry point for the OS. Exposes `os_config()`, `os_start()`, and syscalls `os_create_task()`, `os_delay()`, `os_yield()`. Contains assembly-level context save/restore macros that preserve all PIC18F registers (W, STATUS, BSR, FSR0–2, PROD, PCLATH/U, TABLAT, TBL pointers).

### Scheduler (`scheduler.c/h`)
Two algorithms selectable at compile time via `os_config.h`:
- **Round-Robin** — time-sliced with a configurable quantum
- **Priority-based** — highest-priority READY task always runs

The scheduler maintains a task list and an idle task (always READY, lowest priority).

### Task Control Block (`types.h`)
The `TCB_t` struct holds task state (`READY`, `RUNNING`, `WAITING`, `WAITING_SEM`), priority, delay counter, function pointer, saved register snapshot, and a 31-entry hardware stack copy.

### Hardware / Timer ISR (`hw.c/h`)
Timer0 fires the scheduler tick. The ISR decrements task delay counters and triggers preemption when a Round-Robin quantum expires.

### Synchronization (`sync.c/h`)
Binary semaphores: `sem_init()`, `sem_wait()`, `sem_post()`. Waiting tasks are queued in the semaphore's blocked list and unblocked on post.

### Inter-Task Communication (`com.c/h`)
Pipe (circular FIFO): `pipe_init()`, `pipe_read()`, `pipe_write()`. Internally uses semaphores for flow control. Max size set by `PIPE_MAX_SIZE` in `os_config.h`.

### Memory Management (`mem.c/h`)
Simple SRAM heap allocator (Microchip-provided): `SRAMalloc()`, `SRAMfree()`, `SRAMInitHeap()`. Max heap 512 bytes; max single allocation 126 bytes.

### User Tasks (`user.c/h`)
Three demo tasks wired up in `main.c`:
- **LED_1** — produces a fixed data pattern into a pipe
- **LED_2** — toggles RC7
- **LED_3** — consumes pipe data and drives RD0

### Key Tunables (`os_config.h`)
| Constant | Default | Meaning |
|---|---|---|
| `MAX_STACK_SIZE` | 31 | Hardware stack entries per task |
| `MAX_USER_TASKS` | 3 | Maximum concurrent tasks |
| `QUANTUM` | 5 | Timer ticks per RR slot |
| `PIPE_MAX_SIZE` | 4 | Pipe buffer size (bytes) |
| `DEFAULT_SCHEDULER` | — | `ROUND_ROBIN` or `PRIORITY` |

## API_ADC.X Architecture

Thin two-file firmware:
- `io.c/h` — ADC HAL: `adc_config()` (configures AN0/RA0), `adc_on()`, `adc_read()` (returns 10-bit value)
- `main.c` — reads ADC, converts to °C, toggles LED on RD5 when temperature > 60°C

## Course Assignment Prompts

`prompts/` contains the lab assignments (in Portuguese) that define what to implement:
- `01-Mutex.md` through `08-SimulacaoProteus.md` — sequential lab tasks covering mutex, scheduling, dynamic allocation, PWM, ADC, external interrupts, embedded application design, and Proteus simulation.
