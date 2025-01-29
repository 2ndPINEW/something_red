#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "light_controller.h"

static const char *TAG = "MAIN";

void app_main(void) {
    light_controller_init();

    while (1) {
        ESP_LOGI(TAG, "Switching mode to BLINK");
        light_controller_set_mode(LIGHT_MODE_POP);
        vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));

        light_controller_set_mode(LIGHT_MODE_PARTY);
        vTaskDelay(pdMS_TO_TICKS(3 * 60 * 1000));
    }
}
