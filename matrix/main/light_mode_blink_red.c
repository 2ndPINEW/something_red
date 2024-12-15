#include "light_mode_blink_red.h"
#include "back_matrix_control.h"
#include "color_palette.h"
#include "freertos/semphr.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "light_mode_off.h"
#include "light_mode_rainbow_scroll.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

static uint8_t blink_state = 0;

void reset_light_mode_blink_red() {
    blink_state = 0;
}

void light_mode_blink_red() {
    // blink_stateでon/off切替
    blink_state = !blink_state;

    if (blink_state) {
        // 全部赤
        rgb_t red = (rgb_t){.r = 255, .g = 0, .b = 0};
        back_matrix_fill_color(red);
        flush_back_and_wait();

        front_matrix_fill_color(red);
        flush_front_and_wait();
    } else {
        // 全部黒
        rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
        back_matrix_fill_color(off);
        flush_back_and_wait();
        vTaskDelay(30 / portTICK_PERIOD_MS);

        front_matrix_fill_color(off);
        flush_front_and_wait();
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}
