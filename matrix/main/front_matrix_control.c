#include "front_matrix_control.h"
#include "esp_log.h"
#include "esp_ws28xx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "matrix_utils.h"
#include <stdio.h>

#define FRONT_LED_GPIO 3

static CRGB *ws2812_buffer_front;
static uint8_t front_led_state_off = 0;

void front_matrix_init() {
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        ws28xx_init(FRONT_LED_GPIO, WS2812B, FRONT_LEDS, &ws2812_buffer_front));

    // 初期クリア
    for (int i = 0; i < FRONT_LEDS; i++) {
        ws2812_buffer_front[i] = (CRGB){.r = 0, .g = 0, .b = 0};
    }
}

/**
 * フロント側マトリクスの指定座標に色を設定する
 */
void front_matrix_set_pixel_color(int x, int y, CRGB color) {
    if (x < 0 || x >= FRONT_WIDTH || y < 0 || y >= FRONT_HEIGHT)
        return;
    int index = get_matrix_led_index(x, y, FRONT_WIDTH);
    ws2812_buffer_front[index] = color;
}

void blank_front_matrix() {
    for (int y = 0; y < FRONT_HEIGHT; y++) {
        for (int x = 0; x < FRONT_WIDTH; x++) {
            front_matrix_set_pixel_color(x, y, (CRGB){.r = 0, .g = 0, .b = 0});
        }
    }
}

void front_matrix_blink() {
    for (int y = 0; y < FRONT_HEIGHT; y++) {
        for (int x = 0; x < FRONT_WIDTH; x++) {
            if (front_led_state_off) {
                front_matrix_set_pixel_color(x, y,
                                             (CRGB){.r = 0, .g = 0, .b = 0});
            } else {
                front_matrix_set_pixel_color(x, y,
                                             (CRGB){.r = 0, .g = 50, .b = 0});
            }
        }
    }
    front_led_state_off = !front_led_state_off;
}

static int led_counter = 0; // 現在光らせているLEDのインデックス
void front_matrix_light_sequentially() {
    // 全てのLEDを消灯
    blank_front_matrix();

    // led_counterがtotal_pixelsを超えた場合には0に戻す（ループ）
    if (led_counter >= FRONT_LEDS) {
        led_counter = 0; // 最初に戻る
    }

    int x = led_counter % FRONT_WIDTH;
    int y = led_counter / FRONT_WIDTH;

    // 現在のインデックスに対応するLEDを点灯
    front_matrix_set_pixel_color(x, y, (CRGB){.r = 50, .g = 0, .b = 0});

    // 次回呼び出し時は次のLEDへ
    led_counter++;
}
