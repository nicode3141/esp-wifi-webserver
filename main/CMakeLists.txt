idf_component_register(SRCS "wifi-webserver.c" "credentials.c"
                    PRIV_REQUIRES esp_wifi vfs fatfs esp_http_server nvs_flash spiffs
                    INCLUDE_DIRS ".")
