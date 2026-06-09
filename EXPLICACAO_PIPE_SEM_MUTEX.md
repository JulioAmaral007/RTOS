# Explicação — Requisitos T1 aplicados no projeto

---

## 1. Compostas por várias tarefas (mais de três)

O projeto tem quatro tarefas de usuário mais a idle:

- **task_sensor** — lê o ADC, converte pra °C e escreve no pipe
- **task_display** — lê do pipe, atualiza `temp_global` com mutex, acende o LED da faixa correta e posta o semáforo pra acordar a task_pwm
- **task_pwm** — espera o semáforo, lê `temp_global` com mutex e ajusta o duty cycle do PWM
- **one_shot_task** — criada dinamicamente pelo ISR do INT0 ao pressionar o botão; executa uma única vez, togla o LED em RD2 e encerra
- **idle** — fallback do scheduler, roda quando todas as outras estão bloqueadas

---

## 2. Tarefas que se comunicam via PIPE

```
task_sensor ──[pipe]──► task_display
```

`task_sensor` chama `pipe_write(pipe, temp)`. Internamente o pipe faz `sem_wait(s_input)` — se não houver espaço livre, bloqueia. Quando há espaço, escreve no buffer circular e chama `sem_post(s_output)` pra sinalizar dado disponível.

`task_display` chama `pipe_read(pipe, &temp)`. Internamente faz `sem_wait(s_output)` — se o buffer estiver vazio, vai pra `WAITING_SEM` até `task_sensor` postar. Ao ler, devolve o espaço com `sem_post(s_input)`.

---

## 3. Tarefas com prioridades iguais

`task_sensor` e `task_display` têm ambas prioridade **5**. Quando as duas estão READY ao mesmo tempo o scheduler aplica Round-Robin entre elas — cada uma roda um quantum (5 ticks) e cede o processador pra outra, sem que nenhuma fique presa.

---

## 4. Tarefas sincronizadas por semáforos e variáveis mutex

**Semáforo `s_new_data`** — inicializado em 0. `task_display` posta após processar a temperatura; `task_pwm` bloqueia nele no início de cada iteração. Garante que `task_pwm` só rode quando há dado novo.

**Mutex `m_temp`** — protege a variável global `temp_global` compartilhada entre `task_display` (escrita) e `task_pwm` (leitura). Sem ele, o Timer0 poderia preemptar `task_display` no meio da escrita e `task_pwm` leria um valor corrompido.

---

## 5. Tarefa one-shot via interrupção externa

Ao pressionar o botão o ISR do INT0 chama `os_create_task(one_shot_task, prioridade=6)`. No próximo tick o scheduler identifica ela como a maior prioridade READY e a preempta imediatamente. Ela executa, togla o LED em RD2, chama `os_task_exit` e é removida da fila — não volta a rodar.

---

## 6. Escalonamento por prioridade com Round-Robin

O `rr_prior_scheduler` opera em duas passadas a cada tick do Timer0:

1. Varre todas as tasks e acha a maior prioridade entre as READY
2. A partir da task atual, avança em ordem circular até achar a próxima READY com essa prioridade — isso é o Round-Robin

Prioridades: idle = 0, task_pwm = 4, task_sensor = task_display = 5, one_shot_task = 6 (quando existe).

---

## 7. Alocação dinâmica

O pipe aloca seu buffer interno com `SRAMalloc` em `pipe_init`, trocando a alocação estática por dinâmica usando a API de heap do próprio SO (`mem.c`). O mesmo vale para a `one_shot_task`, que é criada e destruída sob demanda — o TCB é alocado em `os_create_task` e liberado em `os_task_exit`.

---

## 8. API de E/S — PWM, ADC e interrupção externa

**ADC** — `adc_config()` configura AN0/RA0; `adc_read()` retorna o valor bruto de 10 bits. A conversão pra °C usa `temp = (raw * 500) / 1024` (cast pra `uint32_t` obrigatório para evitar overflow).

**PWM** — `pwm_config()` inicializa o CCP1 em modo PWM; `pwm_set_duty(duty)` recebe o valor calculado como `duty = temp * 13` (aproximação inteira de `1024/79`) e ajusta o brilho do LED em RC2.

**Interrupção externa** — `ext_int_config()` habilita INT0 na borda de descida em RB0. O ISR é o gatilho para criação da `one_shot_task`.
