#include "credentials.h"
#include <string.h>
#include "nvs.h"

#define NVS_NAMESPACE "softap_cfg"
#define SSID_KEY "ssid"
#define PW_KEY "password"

static esp_err_t validate_credentials(const softap_credentials_t *credentials){
    if(credentials == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    size_t ssid_len = strnlen(credentials->ssid, sizeof(credentials->ssid));
    size_t pw_len = strnlen(credentials->password, sizeof(credentials->password));

    if(ssid_len == 0 || ssid_len > MAX_LEN_SOFTAP_SSID){
        return ESP_ERR_INVALID_ARG;
    }

    if(pw_len < 8 || pw_len > MAX_LEN_SOFTAP_PW){
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t credentials_load(softap_credentials_t *credentials){
    if(credentials == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    memset(credentials, 0, sizeof(*credentials)); // Reset credentials

    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    if(ret != ESP_OK) return ret;

    size_t ssid_buff_size = sizeof(credentials->ssid);
    size_t pw_buff_size = sizeof(credentials->password);


    ret = nvs_get_str(handle, SSID_KEY, credentials->ssid, &ssid_buff_size);

    if(ret == ESP_OK){
        ret = nvs_get_str(handle, PW_KEY, credentials->password, &pw_buff_size);
    }

    nvs_close(handle);

    if(ret == ESP_OK){
        ret = validate_credentials(credentials);
    }

    if(ret != ESP_OK){
        // Reset partition if failed
        memset(credentials, 0, sizeof(*credentials));
    }

    return ret;
}

esp_err_t credentials_save(const softap_credentials_t *credentials){
    if(credentials == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = validate_credentials(credentials);
    if(ret != ESP_OK) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

    if(ret != ESP_OK){
        return ret;
    }
    
    ret = nvs_set_str(handle, SSID_KEY, credentials->ssid);

    if(ret == ESP_OK){
        ret = nvs_set_str(handle, PW_KEY, credentials->password);
    }

    if(ret == ESP_OK){
        ret = nvs_commit(handle);
    }

    nvs_close(handle);

    return ret;
}

esp_err_t set_default_credentials(softap_credentials_t *credentials){
    if(credentials == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    *credentials = (softap_credentials_t){
        .ssid = "dmx-tool-AP",
        .password = "ultrasecure"
    };

    return credentials_save(credentials);
}