#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "light_controller.h"

static const char *TAG = "MAIN";

void app_main(void) {
    light_controller_init();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Switching mode to BLINK");
        light_controller_set_mode(LIGHT_MODE_BLINK_RED);

        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Switching mode to OFF");
        light_controller_set_mode(LIGHT_MODE_OFF);

        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Switching mode to RAINBOW_SCROLL");
        light_controller_set_mode(LIGHT_MODE_RAINBOW_SCROLL);

        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Switching mode to INIT");
        light_controller_set_mode(LIGHT_MODE_INIT);
    }
}
