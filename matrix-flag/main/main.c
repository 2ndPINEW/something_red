#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "light_controller.h"

static const char *TAG = "MAIN";

void app_main(void) {
    light_controller_init();

    ESP_LOGI(TAG, "light controller initted");

    light_controller_set_mode(LIGHT_MODE_INIT);
    vTaskDelay(pdMS_TO_TICKS(1 * 1000));

    light_controller_set_mode(LIGHT_MODE_BLUE_FIRE);
}
