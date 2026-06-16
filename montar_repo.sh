#!/usr/bin/env bash
#
# montar_repo.sh — cria a estrutura do repositório esp32-labs com os 8 labs
# de FreeRTOS, cada um como um projeto ESP-IDF completo e independente.
#
# Uso: rode este script DE DENTRO da pasta do seu repositório (ex.: ~/esp32-labs)
#   cd ~/esp32-labs
#   bash montar_repo.sh
#
set -e

echo ">> Criando estrutura do repositório esp32-labs..."

# --- helper: cria o esqueleto de um projeto ESP-IDF ---
scaffold () {
  local dir="$1" proj="$2"
  mkdir -p "$dir/main"
  cat > "$dir/CMakeLists.txt" << TOPCMAKE
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
project($proj)
TOPCMAKE
  cat > "$dir/main/CMakeLists.txt" << 'MAINCMAKE'
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver)
MAINCMAKE
  cat > "$dir/sdkconfig.defaults" << 'DEFCFG'
CONFIG_IDF_TARGET="esp32"
DEFCFG
}

# ============================== LAB 01 ==============================
scaffold "01-blink" "blink"
cat > "01-blink/main/main.c" << 'CCODE'
// Lab 01 — Primeiro pisca
// Acende e apaga o LED onboard (GPIO2). O "ola mundo" do embarcado.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED 2   // GPIO2 — LED onboard na maioria das placas

void app_main(void)
{
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(LED, 1);              // acende
        vTaskDelay(pdMS_TO_TICKS(500));      // espera 500 ms
        gpio_set_level(LED, 0);              // apaga
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
CCODE

# ============================== LAB 02 ==============================
scaffold "02-duas-tasks" "duas_tasks"
cat > "02-duas-tasks/main/main.c" << 'CCODE'
// Lab 02 — Duas coisas ao mesmo tempo
// Uma task pisca o LED; outra imprime um batimento no serial, em ritmos
// diferentes. O momento "aha" do RTOS: concorrencia.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2

// TASK 1 — pisca o LED a cada 500 ms
void task_led(void *param)
{
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(LED, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// TASK 2 — imprime um batimento a cada 200 ms
void task_heartbeat(void *param)
{
    int n = 0;
    while (1) {
        printf("batimento %d (eu rodo enquanto o LED pisca!)\n", n++);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    //          funcao          nome          stack  arg  prio handle
    xTaskCreate(task_led,       "led",       2048,  NULL, 5,  NULL);
    xTaskCreate(task_heartbeat, "heartbeat", 2048,  NULL, 5,  NULL);
}
CCODE

# ============================== LAB 03 ==============================
scaffold "03-prioridade" "prioridade"
cat > "03-prioridade/main/main.c" << 'CCODE'
// Lab 03 — Prioridade e preempcao
// Uma task de prioridade ALTA e uma BAIXA disputam a CPU.
//
// EXPERIMENTO: comente o vTaskDelay dentro de task_gulosa e regrave.
// O LED para de piscar -> starvation: a task de alta prioridade que
// nunca bloqueia monopoliza a CPU e mata de fome a de baixa.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2

// Prioridade BAIXA (2): so quer piscar o LED
void task_led(void *p) {
    gpio_reset_pin(LED); gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(LED, 1); vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(LED, 0); vTaskDelay(pdMS_TO_TICKS(300));
    }
}

// Prioridade ALTA (10): trabalha sem parar
void task_gulosa(void *p) {
    volatile long x = 0;
    while (1) {
        x++;
        vTaskDelay(pdMS_TO_TICKS(10));  // devolve a CPU; sem isto = starvation
    }
}

void app_main(void) {
    xTaskCreate(task_led,    "led",    2048, NULL, 2,  NULL); // baixa
    xTaskCreate(task_gulosa, "gulosa", 2048, NULL, 10, NULL); // alta
}
CCODE

# ============================== LAB 04 ==============================
scaffold "04-fila" "fila"
cat > "04-fila/main/main.c" << 'CCODE'
// Lab 04 — Filas (queues)
// Uma task gera numeros (1..4); outra recebe e pisca o LED essa
// quantidade de vezes. O jeito certo de passar dados entre tasks.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2

QueueHandle_t fila;   // o "cano" entre as duas tasks

// PRODUTOR — manda um numero (1..4) a cada 2 s
void task_produtor(void *p) {
    int n = 1;
    while (1) {
        printf("produtor: mandando %d\n", n);
        xQueueSend(fila, &n, portMAX_DELAY);  // empurra n na fila
        n = (n % 4) + 1;                      // 1,2,3,4,1,2...
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// CONSUMIDOR — espera um numero e pisca o LED essas vezes
void task_consumidor(void *p) {
    gpio_reset_pin(LED); gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    int recebido;
    while (1) {
        // BLOQUEIA aqui ate chegar algo na fila (sem gastar CPU):
        if (xQueueReceive(fila, &recebido, portMAX_DELAY) == pdTRUE) {
            printf("consumidor: recebi %d, piscando...\n", recebido);
            for (int i = 0; i < recebido; i++) {
                gpio_set_level(LED, 1); vTaskDelay(pdMS_TO_TICKS(150));
                gpio_set_level(LED, 0); vTaskDelay(pdMS_TO_TICKS(150));
            }
        }
    }
}

void app_main(void) {
    fila = xQueueCreate(5, sizeof(int));  // ate 5 inteiros guardados
    xTaskCreate(task_produtor,   "prod", 2048, NULL, 5, NULL);
    xTaskCreate(task_consumidor, "cons", 2048, NULL, 5, NULL);
}
CCODE

# ============================== LAB 05 ==============================
scaffold "05-botao-isr" "botao_isr"
cat > "05-botao-isr/main/main.c" << 'CCODE'
// Lab 05 — Botao + interrupcao (ISR) + semaforo
// Apertar o botao (GPIO4) alterna o LED, via interrupcao de hardware.
// Padrao "ISR sinaliza, task trabalha".
//
// Fiacao: botao entre GPIO4 e GND (pull-up interno ligado).
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2
#define BOTAO 4

SemaphoreHandle_t sem_botao;

// A ISR: roda quando o botao e apertado. Curtissima — so sinaliza.
void IRAM_ATTR botao_isr(void *arg) {
    BaseType_t acordou = pdFALSE;
    xSemaphoreGiveFromISR(sem_botao, &acordou);  // "o botao foi apertado!"
    if (acordou) portYIELD_FROM_ISR();           // troca pra task ja, se preciso
}

// A task: faz o trabalho de verdade (alternar o LED)
void task_botao(void *p) {
    int estado = 0;
    while (1) {
        // dorme ate a ISR sinalizar:
        if (xSemaphoreTake(sem_botao, portMAX_DELAY) == pdTRUE) {
            estado = !estado;
            gpio_set_level(LED, estado);
            printf("botao! LED agora: %s\n", estado ? "ON" : "OFF");
        }
    }
}

void app_main(void) {
    // LED como saida
    gpio_reset_pin(LED); gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    // Botao como entrada, pull-up interno, interrupcao na borda de descida
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOTAO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .intr_type = GPIO_INTR_NEGEDGE,   // dispara quando vai de 1->0 (apertou)
    };
    gpio_config(&cfg);

    sem_botao = xSemaphoreCreateBinary();
    xTaskCreate(task_botao, "botao", 2048, NULL, 5, NULL);

    // liga o servico de ISR e registra a nossa ISR no pino do botao
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOTAO, botao_isr, NULL);
}
CCODE

# ============================== LAB 06 ==============================
scaffold "06-mutex" "mutex"
cat > "06-mutex/main/main.c" << 'CCODE'
// Lab 06 — Mutex
// Duas tasks imprimem uma linha em pedacos. O mutex impede que elas
// se atropelem e baguncem o serial.
//
// EXPERIMENTO: comente o take/give e regrave para ver a "salada" — as
// linhas das duas tasks se misturam sem a protecao do mutex.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>

SemaphoreHandle_t mutex_serial;

void imprime_linha(const char *quem) {
    xSemaphoreTake(mutex_serial, portMAX_DELAY);   // pega o "direito de falar"
    printf("[%s] ", quem);          vTaskDelay(pdMS_TO_TICKS(5));
    printf("parte1 ");              vTaskDelay(pdMS_TO_TICKS(5));
    printf("parte2 ");              vTaskDelay(pdMS_TO_TICKS(5));
    printf("FIM\n");
    xSemaphoreGive(mutex_serial);                  // solta pra proxima task
}

void task_A(void *p) { while(1){ imprime_linha("AAAA"); vTaskDelay(pdMS_TO_TICKS(50)); } }
void task_B(void *p) { while(1){ imprime_linha("bbbb"); vTaskDelay(pdMS_TO_TICKS(50)); } }

void app_main(void) {
    mutex_serial = xSemaphoreCreateMutex();
    xTaskCreate(task_A, "A", 2048, NULL, 5, NULL);
    xTaskCreate(task_B, "B", 2048, NULL, 5, NULL);
}
CCODE

# ============================== LAB 07 ==============================
scaffold "07-timer" "timer"
cat > "07-timer/main/main.c" << 'CCODE'
// Lab 07 — Software timer
// Pisca o LED via timer, sem dedicar uma task inteira.
//
// EXPERIMENTO: troque pdTRUE por pdFALSE (one-shot): o LED muda de
// estado uma vez so e para. E assim que se faz um timeout.
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2

// callback: roda toda vez que o timer vence
void heartbeat_cb(TimerHandle_t t) {
    static int estado = 0;
    estado = !estado;
    gpio_set_level(LED, estado);
    printf("tic (timer) -> LED %s\n", estado ? "ON" : "OFF");
}

void app_main(void) {
    gpio_reset_pin(LED); gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    TimerHandle_t timer = xTimerCreate(
        "heartbeat",           // nome (debug)
        pdMS_TO_TICKS(500),    // vence a cada 500 ms
        pdTRUE,                // auto-reload: repete pra sempre
        NULL,                  // id (nao usamos)
        heartbeat_cb);         // funcao chamada quando vence

    xTimerStart(timer, 0);     // arma o timer
}
CCODE

# ============================== LAB 08 ==============================
scaffold "08-projeto-integrado" "projeto_integrado"
cat > "08-projeto-integrado/main/main.c" << 'CCODE'
// Lab 08 — Mini-projeto integrado
// O botao muda a velocidade do pisca (via fila), enquanto um timer toca
// um heartbeat independente. ISR + semaforo + fila + tasks + timer juntos.
//
// Fiacao: botao entre GPIO4 e GND; LED no GPIO2.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2
#define BOTAO 4

SemaphoreHandle_t sem_botao;
QueueHandle_t fila_velocidade;

// velocidades de pisca em ms (a cada aperto, passa pra proxima)
int velocidades[] = { 500, 250, 100, 1000 };
int idx = 0;

// ISR do botao: sinaliza
void IRAM_ATTR botao_isr(void *arg) {
    BaseType_t acordou = pdFALSE;
    xSemaphoreGiveFromISR(sem_botao, &acordou);
    if (acordou) portYIELD_FROM_ISR();
}

// TASK 1: trata o botao, escolhe a proxima velocidade e MANDA pela fila
void task_botao(void *p) {
    while (1) {
        xSemaphoreTake(sem_botao, portMAX_DELAY);   // espera aperto
        idx = (idx + 1) % 4;
        int nova = velocidades[idx];
        printf("botao! nova velocidade: %d ms\n", nova);
        xQueueSend(fila_velocidade, &nova, 0);      // avisa a task do LED
    }
}

// TASK 2: pisca o LED; troca de velocidade quando chega algo na fila
void task_led(void *p) {
    gpio_reset_pin(LED); gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    int periodo = 500;
    while (1) {
        // pega nova velocidade SE houver (timeout 0 = nao bloqueia):
        int nova;
        if (xQueueReceive(fila_velocidade, &nova, 0) == pdTRUE) periodo = nova;

        gpio_set_level(LED, 1); vTaskDelay(pdMS_TO_TICKS(periodo));
        gpio_set_level(LED, 0); vTaskDelay(pdMS_TO_TICKS(periodo));
    }
}

// TIMER: heartbeat no serial, independente de tudo
void heartbeat_cb(TimerHandle_t t) {
    printf("   ...sistema vivo...\n");
}

void app_main(void) {
    // botao com pull-up + interrupcao
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOTAO), .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1, .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);

    sem_botao = xSemaphoreCreateBinary();
    fila_velocidade = xQueueCreate(4, sizeof(int));

    xTaskCreate(task_botao, "botao", 2048, NULL, 6, NULL);  // alta: reage rapido
    xTaskCreate(task_led,   "led",   2048, NULL, 4, NULL);  // media

    TimerHandle_t hb = xTimerCreate("hb", pdMS_TO_TICKS(2000), pdTRUE, NULL, heartbeat_cb);
    xTimerStart(hb, 0);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOTAO, botao_isr, NULL);
}
CCODE

# ============================== docs + meta ==============================
mkdir -p docs
cat > docs/.gitkeep << 'GK'
GK

# .gitignore (essencial: build/ e sdkconfig com senha de WiFi nunca vao pro git)
cat > .gitignore << 'GITIGNORE'
# ESP-IDF — artefatos gerados (nao versionar)
build/
sdkconfig
sdkconfig.old
managed_components/
dependencies.lock

# Python
*.pyc
__pycache__/

# Editor / SO
.vscode/
*.swp
.DS_Store
GITIGNORE

# README
cat > README.md << 'README'
# esp32-labs

Meus experimentos aprendendo **ESP32** com **ESP-IDF** e **FreeRTOS**.
Cada pasta numerada e um projeto independente — faca na ordem.

## Labs de FreeRTOS

| # | Pasta | O que ensina |
|---|-------|--------------|
| 01 | `01-blink` | GPIO e o primeiro pisca-LED |
| 02 | `02-duas-tasks` | duas tasks concorrentes em ritmos diferentes |
| 03 | `03-prioridade` | prioridade e preempcao (causar e consertar starvation) |
| 04 | `04-fila` | passar dados entre tasks com filas (queues) |
| 05 | `05-botao-isr` | botao via interrupcao (ISR) + semaforo |
| 06 | `06-mutex` | proteger recurso compartilhado com mutex |
| 07 | `07-timer` | agendar com software timer |
| 08 | `08-projeto-integrado` | tudo junto: ISR + semaforo + fila + tasks + timer |

Varios labs tem um **EXPERIMENTO** comentado no topo do `main.c` — vale testar.

## Hardware

- ESP32 DevKit + cabo USB
- LED no GPIO2 (onboard na maioria das placas, ou externo via resistor 330 Ohm)
- Botao entre GPIO4 e GND (labs 05 e 08)

## Como rodar um lab

```bash
. $IDF_PATH/export.sh          # carrega o ESP-IDF
cd 01-blink                    # entra na pasta do lab
idf.py set-target esp32        # uma vez por pasta
idf.py -p /dev/ttyACM0 flash monitor
```

Sair do monitor: `Ctrl+]`.

## Documentacao

Guias em HTML (abrir no navegador) ficam em `docs/`.

## Adicionar um lab novo

1. Crie uma pasta nova (ex.: `09-meu-sensor/`) com a mesma estrutura.
2. `git add . && git commit -m "lab 09: meu sensor" && git push`

Projeto novo = pasta nova. Nada e sobrescrito.
README

echo ""
echo ">> Pronto! Estrutura criada:"
echo ""
ls -d [0-9]* 2>/dev/null | sed 's/^/   /'
echo "   docs/"
echo "   README.md"
echo "   .gitignore"
echo ""
echo ">> Proximo passo: git add . && git commit -m \"labs 1 a 8\" && git push"
