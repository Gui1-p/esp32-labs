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
    if (acordou) portYIELD_FROM_ISR(); //Se o "acordou" esta em HIGH ele passa a vez da task 
                                       //que estava em execução antes da ISR para a de maior
                                       //prioridade
}


// TASK 1: trata o botao, escolhe a proxima velocidade e MANDA pela fila
void task_botao(void *p) {
    while (1) {
        xSemaphoreTake(sem_botao, portMAX_DELAY);   // espera aperto
        idx = (idx + 1) % 4;
        int nova = velocidades[idx];
        printf("botao! nova velocidade: %d ms\n", nova);
        xQueueSend(fila_velocidade, &nova, 0);      // avisa a task do LED
        /*
            A task tratou de enviar uma variavel simple e não um vetor pela fila
        */
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
