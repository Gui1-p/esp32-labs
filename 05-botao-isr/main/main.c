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
