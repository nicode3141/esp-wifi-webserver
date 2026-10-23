#pragma once

#include "esp_err.h"

#define MAX_LEN_SOFTAP_SSID 32
#define MAX_LEN_SOFTAP_PW 63

typedef struct {
    char ssid[MAX_LEN_SOFTAP_SSID+1];
    char password[MAX_LEN_SOFTAP_PW+1];
} softap_credentials_t;

esp_err_t credentials_load(softap_credentials_t *credentials);

esp_err_t credentials_save(const softap_credentials_t *credentials);

esp_err_t set_default_credentials(softap_credentials_t *credentials);