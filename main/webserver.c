#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "sdkconfig.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

const char *TAG = "webserver";
static const char *SPIFFS_BASE_PATH = "/spiffs";

#define PART_LABEL "storage"
/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/**
 * Used to mount a base path to the virual filesystem.
 * @param base_path the path the files should be accessible after
 * @returns esp_err_t ESP_OK if successful
 */
esp_err_t mount_web_storage(const char* base_path){
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = base_path,
        .partition_label = PART_LABEL,
        .max_files = 5,   // This sets the maximum number of files that can be open at the same time
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            return ret;
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            return ret;
        }
        return ret;
    }

    // Total/used represent bytes of the fs
    size_t total = 0;
    size_t used = 0;

    ret = esp_spiffs_info(PART_LABEL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        esp_vfs_spiffs_unregister(PART_LABEL);
        return ret;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

// Accepts a http request of a filepath
// Sends the corresponding file back to the client
static esp_err_t file_handler(httpd_req_t *req){
    const char *uri = req->uri;
    const char *filename = NULL;
    const char *content_type = "text/plain";

    // Retrieve lenght until ?
    size_t uri_length = strcspn(uri, "?");

    if(uri_length == 1 && uri[0] == '/'){
        // Requests HTML
        filename = "index.html";
        content_type = "text/html; charset=utf-8";
    } else if(uri_length == strlen("/index.html") && strncmp(uri, "/index.html", uri_length) == 0){
        // Requests HTML
        filename = "index.html";
        content_type = "text/html; charset=utf-8";
    } else if(uri_length == strlen("/styles.css") && strncmp(uri, "/styles.css", uri_length) == 0){
        // Requests CSS
        filename = "styles.css";
        content_type = "text/css";
    } else if(uri_length == strlen("/script.js") && strncmp(uri, "/script.js", uri_length) == 0){
        // Requests JS
        filename = "script.js";
        content_type = "application/javascript; charset=utf-8";
    } else if(uri_length == strlen("/favicon.svg") && strncmp(uri, "/favicon.svg", uri_length) == 0){
        // Requests FAVICON
        filename = "favicon.svg";
        content_type = "image/svg+xml";
    } else if(uri_length == strlen("/assets/chip.gif") && strncmp(uri, "/assets/chip.gif", uri_length) == 0){
        // Requests Chip GIF
        filename = "assets/chip.gif";
        content_type = "image/gif";
    } else{
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_OK;
    }

    // Request filepath
    // Example: SPIFF_DIR/index.html
    char path[64];
    // Length of fromatted fullpath, here: SPIFF_DIR/FILENAME
    int path_length = snprintf(path, sizeof(path), "%s/%s", SPIFFS_BASE_PATH, filename);
    if(path_length < 0 || (size_t) path_length >= sizeof(path)){
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Path too long");
        return ESP_FAIL;
    }

    // The requested file opened as "read-only,binary"
    FILE *file = fopen(path, "rb");
    if(file == NULL){
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_OK;
    }

    httpd_resp_set_type(req, content_type);

    char buff[1024];
    size_t bytes_read;
    esp_err_t res = ESP_OK;

    // Each iteration a section of the requested file is read and sent as a chunk to the client
    while((bytes_read = fread(buff, 1, sizeof(buff), file)) > 0){
        res = httpd_resp_send_chunk(req, buff, bytes_read);

        if(res != ESP_OK){
            ESP_LOGW(TAG, "Failed to send current file chunk");
            break;
        }
    }

    if(ferror(file)){
        ESP_LOGE(TAG, "Failed to read requested file: %s", path);
        res = ESP_FAIL;
    }

    fclose(file);

    if(res == ESP_OK){
        res = httpd_resp_send_chunk(req, NULL, 0);
    }

    return res;
}

esp_err_t start_webserver(httpd_handle_t *server){
    if(server == NULL){
        return ESP_ERR_INVALID_ARG;
    }

    *server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    esp_err_t res = mount_web_storage(SPIFFS_BASE_PATH);
    if(res != ESP_OK){
        ESP_LOGE(TAG, "Failed to mount spiffs storage: %s", esp_err_to_name(res));
        return res;
    }

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
     res = httpd_start(server, &config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        esp_vfs_spiffs_unregister(PART_LABEL);
        return res;
    }

    httpd_uri_t file_download = {
        .uri       = "/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = file_handler,
        .user_ctx  = NULL    // Pass server data as context
    };

    httpd_register_uri_handler(*server, &file_download);

    return ESP_OK;
}