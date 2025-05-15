#include "light_mode_off.h"
#include "../led_matrix_control.h"
#include "../color_palette.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

void reset_light_mode_off() {}

void light_mode_off() {
    // 全部消灯
    rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
    led_matrix_fill_color(off);
    flush_led_and_wait();
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
