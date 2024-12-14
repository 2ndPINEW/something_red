#include "back_matrix_control.h"
#include "esp_log.h"
#include "esp_ws28xx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "matrix_utils.h"

#define BACK_LEFT_LED_GPIO 1
#define BACK_RIGHT_LED_GPIO 2

#define BACK_HALF_WIDTH 16
#define BACK_HALF_HEIGHT 32
#define BACK_HALF_LEDS (BACK_HALF_WIDTH * BACK_HALF_HEIGHT)

static CRGB *ws2812_buffer_left;
static CRGB *ws2812_buffer_right;
static uint8_t back_led_state_off = 0;

void back_matrix_init() {
    // 左パネル初期化
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_init(
        BACK_LEFT_LED_GPIO, WS2812B, BACK_HALF_LEDS, &ws2812_buffer_left));

    // 右パネル初期化
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_init(
        BACK_RIGHT_LED_GPIO, WS2812B, BACK_HALF_LEDS, &ws2812_buffer_right));

    // 初期状態クリア
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            back_matrix_set_pixel_color(x, y, (CRGB){.r = 0, .g = 0, .b = 0});
        }
    }
}

void back_matrix_set_pixel_color(int x, int y, CRGB color) {
    if (x < 0 || x >= BACK_TOTAL_WIDTH || y < 0 || y >= BACK_TOTAL_HEIGHT) {
        return;
    }

    if (x < BACK_HALF_WIDTH) {
        int index = get_matrix_led_index(x, y, BACK_HALF_WIDTH);
        ws2812_buffer_left[index] = color;
    } else {
        int index = get_matrix_led_index(x - BACK_HALF_WIDTH, y, BACK_HALF_WIDTH);
        ws2812_buffer_right[index] = color;
    }
}

void back_matrix_blink() {
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            if (back_led_state_off) {
                back_matrix_set_pixel_color(x, y,
                                            (CRGB){.r = 0, .g = 0, .b = 0});
            } else {
                back_matrix_set_pixel_color(x, y,
                                            (CRGB){.r = 50, .g = 0, .b = 0});
            }
        }
    }
    back_led_state_off = !back_led_state_off;
}
