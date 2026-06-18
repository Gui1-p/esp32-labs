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
/*
    Qual o tipo de dado que o QueueHandle_t pode passar?

*/

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
    /*
        qualuqer tipo pode ser passado em uma fila, claro ela tem que ser 
        criada na função de criação e informada o tipo que vai passar e quanto vai passar,
        quantos itens vai passar na fila. Na verdade, não é o tipo que informamos a fila
        e sim o tamanho do tipo que ela vai carregar, ela trabalha com bytes. 
    */
    xTaskCreate(task_produtor,   "prod", 2048, NULL, 5, NULL);
    xTaskCreate(task_consumidor, "cons", 2048, NULL, 5, NULL);
}
