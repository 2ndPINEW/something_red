#include "back_matrix_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "matrix_utils.h"
#include <stdio.h>

#define BACK_LED_GPIO 20

#define PANEL_WIDTH 16
#define PANEL_HEIGHT 16
#define PANEL_SIZE (PANEL_WIDTH * PANEL_HEIGHT)

#define BACK_TOTAL_WIDTH 32
#define BACK_TOTAL_HEIGHT 32
#define BACK_TOTAL_LEDS (BACK_TOTAL_WIDTH * BACK_TOTAL_HEIGHT)

static const char *TAG = "BACK_MATRIX_CONTROL";

static led_strip_t back_strip = {.type = LED_STRIP_WS2812,
                                 .is_rgbw = false,
                                 .length = BACK_TOTAL_LEDS,
                                 .gpio = BACK_LED_GPIO,
                                 .channel = RMT_CHANNEL_0,
                                 .brightness = 30};

static uint8_t back_led_state_off = 0;
static int led_counter = 0;

void flush_back() { led_strip_flush(&back_strip); }

void clear_back_matrix() {
    led_strip_fill(&back_strip, 0, BACK_TOTAL_LEDS,
                   (rgb_t){.r = 0, .g = 0, .b = 0});
}

void back_matrix_init() {
    ESP_ERROR_CHECK(led_strip_init(&back_strip));
    clear_back_matrix();
}

static int get_panel_offset(int x, int y) {
    // 新しいパネル順と回転
    // 左上(0), 左下(1), 右下(2), 右上(3)
    // オフセット: 0, 256, 512, 768
    if (x < BACK_TOTAL_WIDTH / 2 && y < BACK_TOTAL_HEIGHT / 2) {
        // 左上パネル
        return 0;
    } else if (x < BACK_TOTAL_WIDTH / 2 && y >= BACK_TOTAL_HEIGHT / 2) {
        // 左下パネル
        return PANEL_SIZE;
    } else if (x >= BACK_TOTAL_WIDTH / 2 && y >= BACK_TOTAL_HEIGHT / 2) {
        // 右下パネル(180度回転)
        return PANEL_SIZE * 2;
    } else {
        // 右上パネル(180度回転)
        return PANEL_SIZE * 3;
    }
}

// パネルが180度回転かどうかを判定する関数
static bool is_panel_rotated_180(int x, int y) {
    // 右下パネル, 右上パネルが180度回転
    if ((x >= BACK_TOTAL_WIDTH / 2 && y >= BACK_TOTAL_HEIGHT / 2) || // 右下
        (x >= BACK_TOTAL_WIDTH / 2 && y < BACK_TOTAL_HEIGHT / 2)) {  // 右上
        return true;
    }
    return false;
}

void back_matrix_set_pixel_color(int x, int y, rgb_t color) {
    if (x < 0 || x >= BACK_TOTAL_WIDTH || y < 0 || y >= BACK_TOTAL_HEIGHT) {
        return;
    }

    int offset = get_panel_offset(x, y);

    // パネル内のローカル座標計算
    int local_x = (x < BACK_TOTAL_WIDTH / 2) ? x : (x - PANEL_WIDTH);
    int local_y = (y < BACK_TOTAL_HEIGHT / 2) ? y : (y - PANEL_HEIGHT);

    // 回転対応
    if (is_panel_rotated_180(x, y)) {
        // 180度回転
        local_x = (PANEL_WIDTH - 1) - local_x;
        local_y = (PANEL_HEIGHT - 1) - local_y;
    }

    int panel_index = get_matrix_panel_led_index(local_x, local_y, PANEL_WIDTH);

    int final_index = offset + panel_index;
    led_strip_set_pixel(&back_strip, final_index, color);
}

void blank_back_matrix() {
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            back_matrix_set_pixel_color(x, y, (rgb_t){.r = 0, .g = 0, .b = 0});
        }
    }
}

void back_matrix_blink() {
    rgb_t on_color = {.r = 50, .g = 0, .b = 0};
    rgb_t off_color = {.r = 0, .g = 0, .b = 0};

    rgb_t color = back_led_state_off ? off_color : on_color;
    led_strip_fill(&back_strip, 0, BACK_TOTAL_LEDS, color);
    flush_back();

    back_led_state_off = !back_led_state_off;
}

#define p(x_start, y)                                                          \
    back_matrix_set_pixel_color(                                               \
        (led_counter + x_start) % BACK_TOTAL_WIDTH, y,                         \
        get_color((led_counter + x_start) % BACK_TOTAL_WIDTH, y));
#define p5(x_start, y)                                                         \
    back_matrix_set_pixel_color(                                               \
        (led_counter + x_start + 5) % BACK_TOTAL_WIDTH, y,                     \
        get_color((led_counter + x_start + 5) % BACK_TOTAL_WIDTH, y));

rgb_t get_color(int x, int y) {
    if (BACK_TOTAL_WIDTH / 3 >= x) {
        return (rgb_t){.r = 50, .g = 0, .b = 0};
    } else if (BACK_TOTAL_WIDTH / 3 * 2 >= x) {
        return (rgb_t){.r = 0, .g = 50, .b = 0};
    }
    return (rgb_t){.r = 0, .g = 0, .b = 50};
}

void back_matrix_light_sequentially() {
    // 全消灯
    led_strip_fill(&back_strip, 0, BACK_TOTAL_LEDS,
                   (rgb_t){.r = 0, .g = 0, .b = 0});

    if (led_counter >= BACK_TOTAL_LEDS) {
        led_counter = 0;
    }

    p(0, 0);
    p(1, 0);
    p(2, 0);
    p(3, 0);
    p(0, 1);
    p(3, 2);
    p(2, 2);
    p(1, 2);
    p(0, 2);
    p(3, 3);
    p(0, 4);
    p(1, 4);
    p(2, 4);
    p(3, 4);

    p5(0, 0);
    p5(1, 0);
    p5(2, 0);
    p5(3, 0);
    p5(0, 1);
    p5(3, 2);
    p5(2, 2);
    p5(1, 2);
    p5(0, 2);
    p5(3, 3);
    p5(0, 4);
    p5(1, 4);
    p5(2, 4);
    p5(3, 4);

    flush_back();

    led_counter++;
}

// palette: rgb_t配列（パレット）
// palette_count: パレット色数
// canvas: 32x32のパレットインデックス配列（0～palette_count-1の値）
void back_matrix_draw_from_palette(
    const rgb_t *palette, int palette_count,
    const uint8_t canvas[BACK_TOTAL_HEIGHT][BACK_TOTAL_WIDTH]) {
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            uint8_t index = canvas[y][x];
            rgb_t color = (index < palette_count)
                              ? palette[index]
                              : (rgb_t){.r = 0, .g = 0, .b = 0};
            back_matrix_set_pixel_color(x, y, color);
        }
    }
    flush_back();
}
