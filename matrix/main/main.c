#include "back_matrix_control.h"
#include "front_matrix_control.h"
#include "matrix_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "MAIN";

void app_main(void) {
    led_strip_install();
    back_matrix_init();
    front_matrix_init();

    while (1) {
        ESP_LOGI(TAG, "Blinking back matrix...");
        back_matrix_light_sequentially();
        flush_back();
        front_matrix_light_sequentially();
        tape_light_sequentially(TAPE1);
        tape_light_sequentially(TAPE2);
        tape_light_sequentially(TAPE3);
        tape_light_sequentially(TAPE4);
        flush_front();
        // vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
