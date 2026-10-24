#include <string.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "credentials.h"
#include "webserver.h"

#define ESP_WIFI_CHANNEL 1
#define MAX_STA_CONN 2

static char* TAG = "webserver";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data){
    printf("Event nr: %ld\n", (long) event_id);
}

void init_nvs(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
    ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_ERROR_CHECK(ret);
}

void init_soft_ap(softap_credentials_t *credentials){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    esp_wifi_init(&config);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(credentials->ssid),
            .channel = ESP_WIFI_CHANNEL,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            }
        }
    };

    strcpy((char *)wifi_config.ap.ssid, credentials->ssid);
    strcpy((char *)wifi_config.ap.password, credentials->password);


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             credentials->ssid, credentials->password, ESP_WIFI_CHANNEL);
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

httpd_handle_t start_webserver_old() {
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
    init_nvs();
    
    softap_credentials_t ap_credentials = {0};
    esp_err_t ret = credentials_load(&ap_credentials);

    if(ret == ESP_ERR_NVS_NOT_FOUND){
        ret = set_default_credentials(&ap_credentials);
    } else if(ret != ESP_OK){
        ESP_LOGE(TAG, "Failed to load wifi credentials %s", esp_err_to_name(ret));
        return;
    }

    ESP_ERROR_CHECK(ret);
    init_soft_ap(&ap_credentials);

    httpd_handle_t server = NULL;
    esp_err_t res = start_webserver(&server);
    if(res != ESP_OK){
        ESP_LOGE(TAG, "Failed to start webserver: %s", esp_err_to_name(res));
        return;
    }
}