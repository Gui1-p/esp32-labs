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
