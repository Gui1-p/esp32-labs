#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define LED_A GPIO_NUM_5
#define LED_B GPIO_NUM_6
#define LED_C GPIO_NUM_7

typedef struct Led 
{
    int pino;
    float tempo;
}Led;


void blinkLed(void *param);

void app_main(void)
{
    static Led LedGreen = {LED_A, 5};
    static Led LedYellow = {LED_B, 2};
    static Led LedRed = {LED_C, 4};

    xTaskCreate(blinkLed, "led1", 2048, &LedGreen, 5, NULL);
    xTaskCreate(blinkLed, "led2", 2048, &LedYellow, 5, NULL);
    xTaskCreate(blinkLed, "led3", 2048, &LedRed, 5, NULL);

}

void blinkLed(void *param)
{
    Led *led = (Led*)param;


    gpio_reset_pin(led->pino);
    gpio_set_direction(led->pino, GPIO_MODE_OUTPUT);

    while(true){
    gpio_set_level(led->pino, true);
    vTaskDelay(pdMS_TO_TICKS(1000*led->tempo));
    gpio_set_level(led->pino, false);
    vTaskDelay(pdMS_TO_TICKS(100*led->tempo));

    }
}