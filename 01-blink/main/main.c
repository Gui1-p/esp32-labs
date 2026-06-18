// Lab 01 — Primeiro pisca
// Acende e apaga o LED onboard (GPIO2).
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED 2   // GPIO2 — LED onboard 

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

/*
    Não há a criação de nenhuma task aqui, apenas é aplicado um delay em uma task que ja existe, o main.
    A ideia do vTaskDelay é que nesse delay a task de folga para que outras também sejam executadas, mas
    neste caso so existe a task main, então nenhuma outra será executada mesmo que a task não esteja rodando.
*/

}
