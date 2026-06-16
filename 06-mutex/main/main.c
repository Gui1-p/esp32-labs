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
