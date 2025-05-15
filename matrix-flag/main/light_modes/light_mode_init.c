#include "light_mode_init.h"
#include "../led_matrix_control.h"
#include "../color_palette.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

void reset_light_mode_init() {}

void light_mode_init() {
    // 全部消灯
    rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
    led_matrix_fill_color(off);
    flush_led_and_wait();
    vTaskDelay(500 / portTICK_PERIOD_MS);

    static const rgb_t init_pal[2] = {{.r = 0, .g = 0, .b = 0},
                                      {.r = 255, .g = 255, .b = 255}};

    static uint8_t init_led_canvas[LED_TOTAL_HEIGHT][LED_TOTAL_WIDTH];

    for (int y = 0; y < LED_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < LED_TOTAL_WIDTH; x++) {
            init_led_canvas[y][x] = 0;
        }
    }
    init_led_canvas[LED_TOTAL_HEIGHT - 1][LED_TOTAL_WIDTH - 1] = 1;

    led_matrix_draw_from_palette(init_pal, 2, init_led_canvas);
    flush_led_and_wait();
    vTaskDelay(pdMS_TO_TICKS(500));
}
