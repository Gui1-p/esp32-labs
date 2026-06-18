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
        pdMS_TO_TICKS(20000),    // vence a cada 500 ms
        pdFALSE,                // auto-reload: repete pra sempre
        NULL,                  // id (nao usamos)
        heartbeat_cb);         // funcao chamada quando vence

    xTimerStart(timer, 0);     // arma o timer

    /*
        poderia ser usado um botão para sinalizar o inicio do timer
        e nesse botão o melhor caminho seria usar um semaphore
    */
}
