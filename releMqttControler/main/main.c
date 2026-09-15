/*
 * Servidor HTTP local no ESP32.
 *
 * Conecta no Wi-Fi como estacao (STA), sobe um servidor HTTP e serve
 * uma pagina com um botao que liga/desliga um LED.
 *
 * Rotas:
 *   GET  /        -> pagina HTML
 *   GET  /state   -> {"led":true|false}   (estado atual)
 *   POST /toggle  -> inverte e devolve {"led":...}
 */

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "nvs_flash.h"          //oque faz?
#include "esp_netif.h"          //oque faz?
#include "esp_event.h"          //pra que preciso?
#include "esp_wifi.h"           //como faz?
#include "esp_log.h"            //pra que preciso?
#include "esp_http_server.h"    //como faz?
#include "driver/gpio.h"        //manipulação de registrador pra usar os GPIO

static const char *TAG = "app";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_events;
static int  s_retry_count = 0;
static bool s_led_on      = false;

/* ------------------------------------------------------------------ */
/*  LED                                                                */
/* ------------------------------------------------------------------ */

static void led_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << CONFIG_LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    gpio_set_level(CONFIG_LED_GPIO, 0);
}

            //tenh que ver sobre esse gpio_config_t e porque uso esp log aqui tambem, 
            /*
                devo pegar e apagar oque não me parece essencial pra ver oque acontece
                principalmente esses esp log
            
            */

static void led_set(bool on)
{
    s_led_on = on;
    gpio_set_level(CONFIG_LED_GPIO, on ? 1 : 0);
    ESP_LOGI(TAG, "LED -> %s", on ? "ON" : "OFF");
}

/* ------------------------------------------------------------------ */
/*  Wi-Fi                                                              */
/* ------------------------------------------------------------------ */

/* Handler unico para os dois grupos de evento.
 * Regra: aqui dentro so se faz coisa rapida. Nada de loop, nada de delay. */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        /* radio ligado, agora sim tenta associar */
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < CONFIG_WIFI_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGW(TAG, "desconectado, tentativa %d/%d",
                     s_retry_count, CONFIG_WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "falhou apos %d tentativas", CONFIG_WIFI_MAX_RETRY);
            xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
        }

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) data;
        ESP_LOGI(TAG, "IP recebido: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

            //ate aqui são basicamente so funções de biblioteca, de diferente tem o xEventGroupSetBits, parar um tempo pra ver oque é

/* Retorna true se conectou, false se desistiu. Bloqueia ate decidir. */
static bool wifi_init_sta(void)
{
    s_wifi_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());                 /* pilha TCP/IP  */
    ESP_ERROR_CHECK(esp_event_loop_create_default());  /* loop eventos  */
    esp_netif_create_default_wifi_sta();               /* interface STA */

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* registrar ANTES do start, senao perde o evento STA_START */
    esp_event_handler_instance_t h_wifi, h_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &h_wifi));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &h_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            /* se sua rede for aberta ou WPA antigo, baixe esse nivel */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());   /* retorna na hora, nao conecta */

    ESP_LOGI(TAG, "conectando em '%s'...", CONFIG_WIFI_SSID);

    /* aqui o codigo linear espera o handler resolver */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_events,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    return (bits & WIFI_CONNECTED_BIT) != 0;
}

            //apenas funções de bibliotecas tambem até aqui, so isso de diferente e do freeRTOS xEventGroupWaitBits, vale a penas ver oque é


/* ------------------------------------------------------------------ */
/*  Pagina HTML                                                        */
/* ------------------------------------------------------------------ */

/* Aspas simples no HTML de proposito: assim o literal em C fica limpo. */
static const char PAGINA[] =
"<!DOCTYPE html><html lang='pt-BR'><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>ESP32</title><style>"
"body{background:#111;color:#eee;font-family:system-ui,sans-serif;margin:0;"
"height:100vh;display:flex;flex-direction:column;align-items:center;"
"justify-content:center;gap:24px}"
"h1{font-size:1rem;font-weight:400;color:#777;letter-spacing:.1em}"
"button{width:190px;height:190px;border-radius:50%;border:2px solid #444;"
"background:#1c1c1c;color:#eee;font-size:1.1rem;letter-spacing:.08em;"
"cursor:pointer;transition:all .15s}"
"button:active{transform:scale(.96)}"
"button.on{background:#1f6f4a;border-color:#2fa36b;box-shadow:0 0 30px #1f6f4a}"
"p{color:#555;font-size:.85rem;margin:0}"
"</style></head><body>"
"<h1>CONTROLE LOCAL</h1>"
"<button id='b' onclick='toggle()'>...</button>"
"<p id='s'>carregando</p>"
"<script>"
"function render(on){"
"var b=document.getElementById('b');"
"b.textContent=on?'LIGADO':'DESLIGADO';"
"b.className=on?'on':'';"
"document.getElementById('s').textContent='GPIO em nivel '+(on?'alto':'baixo');}"
"function toggle(){fetch('/toggle',{method:'POST'})"
".then(function(r){return r.json();}).then(function(j){render(j.led);});}"
"fetch('/state').then(function(r){return r.json();})"
".then(function(j){render(j.led);});"
"</script></body></html>";

            //html simples com um pouco de css direto, bem ruim de visualizar, mas sei bem html 

/* ------------------------------------------------------------------ */
/*  Handlers HTTP                                                      */
/* ------------------------------------------------------------------ */

static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, PAGINA, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t responde_estado(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, s_led_on ? "{\"led\":true}" : "{\"led\":false}");
}

static esp_err_t state_get(httpd_req_t *req)
{
    return responde_estado(req);
}

static esp_err_t toggle_post(httpd_req_t *req)
{
    led_set(!s_led_on);
    return responde_estado(req);
}

static httpd_handle_t servidor_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;   /* fecha conexao velha se lotar */

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "nao consegui subir o servidor");
        return NULL;
    }

    httpd_uri_t r_root   = { .uri = "/",       .method = HTTP_GET,
                             .handler = root_get,    .user_ctx = NULL };
    httpd_uri_t r_state  = { .uri = "/state",  .method = HTTP_GET,
                             .handler = state_get,   .user_ctx = NULL };
    httpd_uri_t r_toggle = { .uri = "/toggle", .method = HTTP_POST,
                             .handler = toggle_post, .user_ctx = NULL };

    httpd_register_uri_handler(server, &r_root);
    httpd_register_uri_handler(server, &r_state);
    httpd_register_uri_handler(server, &r_toggle);

    ESP_LOGI(TAG, "servidor no ar na porta %d", config.server_port);
    return server;
}

            //embora seja biblioteca tambem essa é interessante de entender, esses esp LOG são realmente necessários, oque exatamente isso faz de essencial pra funcionar

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

void app_main(void)
{
    /* 1. NVS: o driver Wi-Fi guarda calibracao de RF aqui */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    led_init();

    /* 2. Wi-Fi. So segue adiante se tiver IP. */
    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "sem rede, nada a fazer");
        return;
    }

    /* 3. Servidor. Depois disso app_main pode sair:
     *    o httpd roda na propria task dele. */
    servidor_start();
}
