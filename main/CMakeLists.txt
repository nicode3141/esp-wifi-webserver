idf_component_register(SRCS "wifi-webserver.c"
                    PRIV_REQUIRES esp_wifi esp_http_server
                    INCLUDE_DIRS ".")
