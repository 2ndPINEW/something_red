#include "back_matrix_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "matrix_utils.h"
#include "led_strip.h"
#include <stdio.h>

#define BACK_LED_GPIO 9

#define PANEL_WIDTH 16
#define PANEL_HEIGHT 16
#define PANEL_SIZE (PANEL_WIDTH * PANEL_HEIGHT)

#define BACK_TOTAL_WIDTH 32
#define BACK_TOTAL_HEIGHT 32
#define BACK_TOTAL_LEDS (BACK_TOTAL_WIDTH * BACK_TOTAL_HEIGHT)

static led_strip_t back_strip = {
    .type = LED_STRIP_WS2812,
    .is_rgbw = false,
    .length = BACK_TOTAL_LEDS,
    .gpio = BACK_LED_GPIO,
    .channel = RMT_CHANNEL_0,
    .brightness = 50
};

static uint8_t back_led_state_off = 0;
static int led_counter = 0;

void flush_back() {
    led_strip_flush(&back_strip);
}

void clear_back_matrix() {
    led_strip_fill(&back_strip, 0, BACK_TOTAL_LEDS, (rgb_t){.r=0,.g=0,.b=0});
}

void back_matrix_init() {
    ESP_ERROR_CHECK(led_strip_init(&back_strip));
    clear_back_matrix();
}

// パネルオフセットを計算する関数
// x,y全体座標から、どのパネルか判別してoffsetを返す
static int get_panel_offset(int x, int y) {
    // 左上パネル
    if (x < BACK_TOTAL_WIDTH/2 && y < BACK_TOTAL_HEIGHT/2) {
        return 0;
    }
    // 左下パネル
    if (x < BACK_TOTAL_WIDTH/2 && y >= BACK_TOTAL_HEIGHT/2) {
        return PANEL_SIZE;
    }
    // 右上パネル
    if (x >= BACK_TOTAL_WIDTH/2 && y < BACK_TOTAL_HEIGHT/2) {
        return PANEL_SIZE * 2;
    }
    // 右下パネル
    return PANEL_SIZE * 3;
}

void back_matrix_set_pixel_color(int x, int y, rgb_t color) {
    if (x < 0 || x >= BACK_TOTAL_WIDTH || y < 0 || y >= BACK_TOTAL_HEIGHT) {
        return;
    }

    int offset = get_panel_offset(x, y);

    // パネル内のローカル座標計算
    int local_x, local_y;
    if (x < BACK_TOTAL_WIDTH/2) {
        local_x = x;
    } else {
        local_x = x - PANEL_WIDTH; // 16引く
    }

    if (y < BACK_TOTAL_HEIGHT/2) {
        local_y = y;
    } else {
        local_y = y - PANEL_HEIGHT; // 16引く
    }

    // パネル内でのインデックス取得(16x16の範囲で)
    int panel_index = get_matrix_panel_led_index(local_x, local_y, PANEL_WIDTH);

    int final_index = offset + panel_index;
    led_strip_set_pixel(&back_strip, final_index, color);
}

void blank_back_matrix() {
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            back_matrix_set_pixel_color(x, y, (rgb_t){.r=0, .g=0, .b=0});
        }
    }
}

void back_matrix_blink() {
    rgb_t on_color = {.r=50, .g=0, .b=0};
    rgb_t off_color = {.r=0, .g=0, .b=0};

    rgb_t color = back_led_state_off ? off_color : on_color;
    led_strip_fill(&back_strip, 0, BACK_TOTAL_LEDS, color);

    back_led_state_off = !back_led_state_off;
}

void back_matrix_light_sequentially() {
    led_strip_fill(&back_strip, 0, BACK_TOTAL_LEDS, (rgb_t){.r=0,.g=0,.b=0});

    if (led_counter >= BACK_TOTAL_LEDS) {
        led_counter = 0;
    }

    int x = led_counter % BACK_TOTAL_WIDTH;
    int y = led_counter / BACK_TOTAL_WIDTH;

    back_matrix_set_pixel_color(x, y, (rgb_t){.r=50, .g=0, .b=0});

    led_counter++;
}
