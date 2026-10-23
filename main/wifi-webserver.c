#include <string.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi_netif.h"
#include "esp_http_server.h"
#include "sdkconfig.h"

#define ESP_WIFI_SSID "dmx-tool-AP"
#define ESP_WIFI_PASSWORD "ultrasecure"
#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 2

static char* TAG = "webserver";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data){
    printf("Event nr: %ld!\n", event_id);
}

void init_soft_ap(){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&config);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .password = ESP_WIFI_PASSWORD,
            .channel = ESP_WIFI_CHANNEL,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            }
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ESP_WIFI_SSID, ESP_WIFI_PASSWORD, ESP_WIFI_CHANNEL);
}

static esp_err_t hello_get_handler(httpd_req_t * req){
    const char* response_str = "<h1>Hello World</h1>";
    httpd_resp_send(req, response_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t hello_world = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = hello_get_handler,
    .user_ctx = NULL
};

httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Server started successfully, registering URI handlers...");

        httpd_register_uri_handler(server, &hello_world);
        return server;
    }

    ESP_LOGE(TAG, "Failed to start server");
    return NULL;
}

void app_main(void){
    init_soft_ap();

    httpd_handle_t server = start_webserver();

}
