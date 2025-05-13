#include "light_mode_off.h"
#include "back_matrix_control.h"
#include "color_palette.h"
#include "freertos/semphr.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "light_mode_rainbow_scroll.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

void reset_light_mode_off() {}

void light_mode_off() {
    // 全部消灯
    rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
    back_matrix_fill_color(off);
    flush_back_and_wait();
    vTaskDelay(10 / portTICK_PERIOD_MS);

    front_matrix_fill_color(off);
    tape_fill(TAPE1, off);
    tape_fill(TAPE2, off);
    tape_fill(TAPE3, off);
    tape_fill(TAPE4, off);
    flush_front_and_wait();
    vTaskDelay(10 / portTICK_PERIOD_MS);
}
