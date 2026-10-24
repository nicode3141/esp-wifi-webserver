#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
esp_err_t mount_web_storage(const char* base_path);

esp_err_t start_webserver(httpd_handle_t *server);
esp_err_t stop_webserver(httpd_handle_t *server);