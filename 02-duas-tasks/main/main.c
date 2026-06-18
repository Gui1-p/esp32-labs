// Lab 02 — Duas coisas ao mesmo tempo
// Uma task pisca o LED; outra imprime um batimento no serial, em ritmos
// diferentes. RTOS: concorrencia.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

#define LED 2

// TASK 1 — pisca o LED a cada 500 ms
/*
    Oque é o void *param?
    É possivel usar esse parametro na task se eu quero usar a mesma função 
    para tasks diferentes, mas semelhantes, que usem um parametro que seja diferente
    uma da outra.
    A forma de passar esse parâmetro é pelo ARG da função de criação de task.
    É preciso fazer o casting para transformar o void * no tipo desejado 
*/
void task_led(void *param)
{
    gpio_reset_pin(LED); //limpa qualquer função atrelada ao pino anteriormente
    gpio_set_direction(LED, GPIO_MODE_OUTPUT); //seta o modo do pino (input ou output)
    while (1) {
        gpio_set_level(LED, 1); //seta a saida do pino
        vTaskDelay(pdMS_TO_TICKS(500));//delay na task
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
