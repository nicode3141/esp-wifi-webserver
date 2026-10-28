#include "esp_log.h"
#include "unity.h"

static const char *TAG = "test";

void app_main(void)
{
    UNITY_BEGIN();
    unity_run_menu();
    UNITY_END();
}