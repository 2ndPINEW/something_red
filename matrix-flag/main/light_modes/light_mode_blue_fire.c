#include "light_mode_blue_fire.h"
#include "../led_matrix_control.h"
#include "../color_palette.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include "../matrix_utils.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define FIRE_WIDTH LED_TOTAL_WIDTH
#define FIRE_HEIGHT LED_TOTAL_HEIGHT

static const rgb_t *blue_fire_pal = NULL;

// キャンバスと熱分布データ
static uint8_t canvas[FIRE_HEIGHT][FIRE_WIDTH];
static uint8_t heat[FIRE_HEIGHT][FIRE_WIDTH];

static int frame_counter = 0;

// HSV→RGBは以前と同様
static rgb_t hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float hh = fmodf(h / 60.0f, 6);
    float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    float r = 0, g = 0, b = 0;
    if (0 <= hh && hh < 1) {
        r = c;
        g = x;
        b = 0;
    } else if (1 <= hh && hh < 2) {
        r = x;
        g = c;
        b = 0;
    } else if (2 <= hh && hh < 3) {
        r = 0;
        g = c;
        b = x;
    } else if (3 <= hh && hh < 4) {
        r = 0;
        g = x;
        b = c;
    } else if (4 <= hh && hh < 5) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }
    float m = v - c;
    r += m;
    g += m;
    b += m;
    rgb_t color = {.r = (uint8_t)(r * 255),
                   .g = (uint8_t)(g * 255),
                   .b = (uint8_t)(b * 255)};
    return color;
}

// 青い炎用パレット生成
// 下側は黒～青～水色～白のグラデーション
// 0 = 黒(冷), 中間=青～水色, 最後=白(熱)
static rgb_t blue_fire_palette[RAINBOW_PALETTE_SIZE];
static void generate_blue_fire_palette() {
    for (int i = 0; i < RAINBOW_PALETTE_SIZE; i++) {
        float v = (float)i / (RAINBOW_PALETTE_SIZE - 1); // 0.0～1.0
        // 青い炎にするには、H=180(シアン)～240度(青)へ
        float hue = 180.0f + 60.0f * (1.0f - v); // 上が水色(180)、下が青(240)
        float s = 1.0f;
        float val = 0.5f + 0.5f * v; // 下は暗め(0.5)、上は明るい(1.0)
        blue_fire_palette[i] = hsv_to_rgb(hue, s, val);
    }
    blue_fire_pal = blue_fire_palette;
}

void reset_light_mode_blue_fire() {
    generate_blue_fire_palette();
    frame_counter = 0;

    for (int y = 0; y < FIRE_HEIGHT; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            heat[y][x] = 0;
        }
    }

    led_matrix_fill_color((rgb_t){.r = 0, .g = 0, .b = 0});
    flush_led_and_wait();
}

// 炎拡散ロジック(揺らぎ, 上で減少, 横方向オフセット)
static void update_heat() {
    for (int x = 0; x < FIRE_WIDTH; x++) {
        for (int y = 0; y < FIRE_HEIGHT - 1; y++) {
            // 下の行から熱をもらうが、横に揺らぎを出すため、x±1からもとる
            int xoff = x + (rand() % 3 - 1); // -1,0,1
            if (xoff < 0)
                xoff = 0;
            if (xoff >= FIRE_WIDTH)
                xoff = FIRE_WIDTH - 1;
            int avg = heat[y + 1][xoff];

            // 上へ行くほど減衰を大きく
            // yが小さいほど上。例えば上半分で追加減衰
            if (y < FIRE_HEIGHT / 2) {
                avg = avg - (rand() % 5);
            } else {
                avg = avg - (rand() % 3);
            }

            if (avg < 0)
                avg = 0;
            heat[y][x] = (uint8_t)avg;
        }
    }

    // 最下行に火種
    for (int x = 0; x < FIRE_WIDTH; x++) {
        int spark = (rand() % 160);
        heat[FIRE_HEIGHT - 1][x] = (uint8_t)spark;
    }
}

static void draw_heat_to_canvas() {
    for (int y = 0; y < FIRE_HEIGHT; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            int idx = (heat[y][x] * (RAINBOW_PALETTE_SIZE - 1)) / 160;
            if (idx > RAINBOW_PALETTE_SIZE - 1)
                idx = RAINBOW_PALETTE_SIZE - 1;
            canvas[y][x] = (uint8_t)idx;
        }
    }
}

void light_mode_blue_fire() {
    frame_counter++;
    
    // 熱分布の更新
    update_heat();
    
    // キャンバスに描画
    draw_heat_to_canvas();
    
    // LEDマトリックスに描画
    led_matrix_draw_from_palette(blue_fire_pal, RAINBOW_PALETTE_SIZE, canvas);
    flush_led_and_wait();
    
    // 少し待機
    vTaskDelay(pdMS_TO_TICKS(15));
}
