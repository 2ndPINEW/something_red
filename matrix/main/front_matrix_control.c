#include "front_matrix_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "matrix_utils.h"
#include <stdio.h>

#define FRONT_LED_GPIO 10

static led_strip_t front_strip = {.type = LED_STRIP_WS2812,
                                  .is_rgbw = false,
                                  .length = TOTAL_LEDS,
                                  .gpio = FRONT_LED_GPIO,
                                  .channel = RMT_CHANNEL_1,
                                  .brightness = 10};

// フロント用
static uint8_t front_led_state_off = 0;
static int led_counter = 0;

// テープ用カウンタ（TAPE1～TAPE4分）
static int tape_counters[TAPE_COUNT] = {0};

void flush_front() { led_strip_flush(&front_strip); }

void clear_front_matrix() {
    led_strip_fill(&front_strip, 0, TOTAL_LEDS,
                   (rgb_t){.r = 0, .g = 0, .b = 0});
}

void front_matrix_init() {
    ESP_ERROR_CHECK(led_strip_init(&front_strip));
    clear_front_matrix();
}

void front_matrix_set_pixel_color(int x, int y, rgb_t color) {
    if (x < 0 || x >= FRONT_WIDTH || y < 0 || y >= FRONT_HEIGHT) {
        return;
    }
    int index = get_matrix_panel_led_index(x, y, FRONT_WIDTH);
    led_strip_set_pixel(&front_strip, index, color);
}

void front_matrix_blink() {
    rgb_t off_color = {.r = 0, .g = 0, .b = 0};
    rgb_t on_color = {.r = 255, .g = 0, .b = 0};

    rgb_t color = front_led_state_off ? off_color : on_color;
    led_strip_fill(&front_strip, 0, FRONT_LEDS, color);
    flush_front();

    front_led_state_off = !front_led_state_off;
}

void front_matrix_light_sequentially() {
    // 全消灯（フロント部分）
    led_strip_fill(&front_strip, 0, FRONT_LEDS,
                   (rgb_t){.r = 50, .g = 0, .b = 0});

    if (led_counter >= FRONT_LEDS) {
        led_counter = 0;
    }

    int x = led_counter % FRONT_WIDTH;
    int y = led_counter / FRONT_WIDTH;

    front_matrix_set_pixel_color(x, y, (rgb_t){.r = 50, .g = 50, .b = 0});
    flush_front();

    led_counter++;
}

// テープ制御用内部関数
static int tape_segment_start(tape_segment_t tape) {
    switch (tape) {
    case TAPE1:
        return TAPE1_START;
    case TAPE2:
        return TAPE2_START;
    case TAPE3:
        return TAPE3_START;
    case TAPE4:
        return TAPE4_START;
    default:
        return 0;
    }
}

static bool tape_is_forward(tape_segment_t tape) {
    // TAPE1: forward, TAPE2: reverse, TAPE3: forward, TAPE4: reverse
    if (tape == TAPE1 || tape == TAPE3)
        return true;
    return false;
}

void tape_set_pixel_color(tape_segment_t tape, int i, rgb_t color) {
    if (i < 0 || i >= TAPE_LEDS)
        return;

    int start = tape_segment_start(tape);
    int idx;
    if (tape_is_forward(tape)) {
        // forward
        idx = start + i;
    } else {
        // reverse
        idx = start + (TAPE_LEDS - 1 - i);
    }
    led_strip_set_pixel(&front_strip, idx, color);
}

void tape_fill(tape_segment_t tape, rgb_t color) {
    for (int i = 0; i < TAPE_LEDS; i++) {
        tape_set_pixel_color(tape, i, color);
    }
}

// テープを手前から奥に順番に光らせる関数
// forwardの場合はそのまま0→29、reverseの場合は29→0の順で1ピクセルだけ点灯するアニメーション
void tape_light_sequentially(tape_segment_t tape) {
    // 全消灯（このテープセグメントのみ消灯）
    for (int i = 0; i < TAPE_LEDS; i++) {
        tape_set_pixel_color(tape, i, (rgb_t){.r = 0, .g = 0, .b = 0});
    }

    if (tape_counters[tape] >= TAPE_LEDS) {
        tape_counters[tape] = 0;
    }

    // 現在のインデックスを点灯
    tape_set_pixel_color(tape, tape_counters[tape],
                         (rgb_t){.r = 50, .g = 0, .b = 0});

    tape_counters[tape]++;
}

// フロントマトリクス用：パレットインデックスを持つcanvasを受け取り描画
// palette: rgb_t配列(パレット)
// palette_count: パレットの色数
// canvas: FRONT_HEIGHT x FRONT_WIDTH
// の2次元配列、各要素は0～palette_count-1のパレットインデックス
void front_matrix_draw_from_palette(
    const rgb_t *palette, int palette_count,
    const uint8_t canvas[FRONT_HEIGHT][FRONT_WIDTH]) {
    for (int y = 0; y < FRONT_HEIGHT; y++) {
        for (int x = 0; x < FRONT_WIDTH; x++) {
            uint8_t index = canvas[y][x];
            rgb_t color = (index < palette_count)
                              ? palette[index]
                              : (rgb_t){.r = 0, .g = 0, .b = 0};
            front_matrix_set_pixel_color(x, y, color);
        }
    }
}

// テープライト用：パレットインデックスを持つ1次元配列を受け取り描画
// palette: rgb_t配列(パレット)
// palette_count: パレットの色数
// tape_canvas: TAPE_LEDS長の配列、各要素はパレットインデックス
// tape: 対象テープセグメント(TAPE1～TAPE4)
void tape_draw_from_palette(const rgb_t *palette, int palette_count,
                            const uint8_t tape_canvas[TAPE_LEDS],
                            tape_segment_t tape) {
    for (int i = 0; i < TAPE_LEDS; i++) {
        uint8_t index = tape_canvas[i];
        rgb_t color = (index < palette_count) ? palette[index]
                                              : (rgb_t){.r = 0, .g = 0, .b = 0};
        tape_set_pixel_color(tape, i, color);
    }
}

// 必要に応じてテープライト4本分をまとめて描画する関数も用意可能
// tape_canvases: TAPE_COUNT ×
// TAPE_LEDSの2次元配列(ここでは1次元配列の配列と仮定)
void tape_draw_all_from_palette(
    const rgb_t *palette, int palette_count,
    const uint8_t tape_canvases[TAPE_COUNT][TAPE_LEDS]) {
    for (int t = 0; t < TAPE_COUNT; t++) {
        tape_segment_t seg = (tape_segment_t)t;
        for (int i = 0; i < TAPE_LEDS; i++) {
            uint8_t index = tape_canvases[t][i];
            rgb_t color = (index < palette_count)
                              ? palette[index]
                              : (rgb_t){.r = 0, .g = 0, .b = 0};
            tape_set_pixel_color(seg, i, color);
        }
    }
    flush_front();
}
