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
