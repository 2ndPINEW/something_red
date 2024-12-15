#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "light_controller.h"

static const char *TAG = "MAIN";

void app_main(void) {
    light_controller_init();

    // 初期モードはlight_controller_init内で設定済み

    // 好きなタイミングでモード変更が可能
    // ここでは5秒おきにモードを切り替えてみる例
    while(1) {
        // vTaskDelay(pdMS_TO_TICKS(5000));
        // ESP_LOGI(TAG, "Switching mode to BLINK");
        // light_controller_set_mode(LIGHT_MODE_BLINK);

        // vTaskDelay(pdMS_TO_TICKS(5000));
        // ESP_LOGI(TAG, "Switching mode to OFF");
        // light_controller_set_mode(LIGHT_MODE_OFF);

        // vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Switching mode to RAINBOW_SCROLL");
        light_controller_set_mode(LIGHT_MODE_RAINBOW_SCROLL);
    }
}
