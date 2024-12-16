#include "light_mode_fire.h"
#include "back_matrix_control.h"
#include "color_palette.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "matrix_utils.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define FIRE_WIDTH FRONT_WIDTH
#define FIRE_HEIGHT FRONT_HEIGHT

#define BACK_FIRE_WIDTH BACK_TOTAL_WIDTH
#define BACK_FIRE_HEIGHT BACK_TOTAL_HEIGHT

static const rgb_t *fire_pal = NULL;

// front用
static uint8_t front_canvas[FIRE_HEIGHT][FIRE_WIDTH];
static uint8_t heat_front[FIRE_HEIGHT][FIRE_WIDTH];

// back用
static uint8_t back_canvas[BACK_FIRE_HEIGHT][BACK_FIRE_WIDTH];
static uint8_t heat_back[BACK_FIRE_HEIGHT][BACK_FIRE_WIDTH];

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

// 炎用パレット生成
// 下側は黒～赤～オレンジ～黄～白のグラデーション
// 0 = 黒(冷), 中間=赤～オレンジ, 最後=白(熱)
static rgb_t fire_palette[RAINBOW_PALETTE_SIZE];
static void generate_fire_palette() {
    for (int i = 0; i < RAINBOW_PALETTE_SIZE; i++) {
        float v = (float)i / (RAINBOW_PALETTE_SIZE - 1); // 0.0～1.0
        // 炎っぽくするには、H=0(赤)付近～60度(黄)へ、V=
        // max(v,0.5)で結構明るめ、S=1
        // Hを0→60度へ線形変化させるとオレンジ～黄色が多くなるため、
        // 赤みを増したいならHを小さい値に固定したりしてもよい
        float hue = 0 + 20.0f * v;
        float s = 1.0f;
        float val = 0.5f + 0.5f * v; // 下は暗め(0.5)、上は明るい(1.0)
        fire_palette[i] = hsv_to_rgb(hue, s, val);
    }
    fire_pal = fire_palette;
}

void reset_light_mode_fire() {
    generate_fire_palette();
    frame_counter = 0;

    for (int y = 0; y < FIRE_HEIGHT; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            heat_front[y][x] = 0;
        }
    }

    for (int y = 0; y < BACK_FIRE_HEIGHT; y++) {
        for (int x = 0; x < BACK_FIRE_WIDTH; x++) {
            heat_back[y][x] = 0;
        }
    }

    front_matrix_fill_color((rgb_t){.r = 0, .g = 0, .b = 0});
    flush_front_and_wait();
    back_matrix_fill_color((rgb_t){.r = 0, .g = 0, .b = 0});
    flush_back_and_wait();
}

// 炎拡散ロジック(揺らぎ, 上で減少, 横方向オフセット)
static void update_heat(uint8_t (*heat)[FIRE_WIDTH], int width, int height) {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height - 1; y++) {
            // 下の行から熱をもらうが、横に揺らぎを出すため、x±1からもとる
            int xoff = x + (rand() % 3 - 1); // -1,0,1
            if (xoff < 0)
                xoff = 0;
            if (xoff >= width)
                xoff = width - 1;
            int avg = heat[y + 1][xoff];

            // 上へ行くほど減衰を大きく
            // yが小さいほど上。例えば上半分で追加減衰
            if (y < height / 2) {
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
    for (int x = 0; x < width; x++) {
        int spark = (rand() % 160);
        heat[height - 1][x] = (uint8_t)spark;
    }
}

// back用heat更新(同じロジックだが別サイズ対応)
static void update_heat_back(uint8_t (*heat)[BACK_FIRE_WIDTH], int width,
                             int height) {
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height - 1; y++) {
            int xoff = x + (rand() % 3 - 1);
            if (xoff < 0)
                xoff = 0;
            if (xoff >= width)
                xoff = width - 1;

            int avg = heat[y + 1][xoff];
            if (y < height / 2) {
                avg = avg - (rand() % 5);
            } else {
                avg = avg - (rand() % 3);
            }
            if (avg < 0)
                avg = 0;
            heat[y][x] = (uint8_t)avg;
        }
    }

    for (int x = 0; x < width; x++) {
        int spark = (rand() % 160);
        heat[height - 1][x] = (uint8_t)spark;
    }
}

static void draw_heat_to_canvas(uint8_t (*heat)[FIRE_WIDTH],
                                uint8_t (*canvas)[FIRE_WIDTH], int width,
                                int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (heat[y][x] * (RAINBOW_PALETTE_SIZE - 1)) / 160;
            if (idx > RAINBOW_PALETTE_SIZE - 1)
                idx = RAINBOW_PALETTE_SIZE - 1;
            canvas[y][x] = (uint8_t)idx;
        }
    }
}

static void draw_heat_to_canvas_back(uint8_t (*heat)[BACK_FIRE_WIDTH],
                                     uint8_t (*canvas)[BACK_FIRE_WIDTH],
                                     int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (heat[y][x] * (RAINBOW_PALETTE_SIZE - 1)) / 160;
            if (idx > RAINBOW_PALETTE_SIZE - 1)
                idx = RAINBOW_PALETTE_SIZE - 1;
            canvas[y][x] = (uint8_t)idx;
        }
    }
}

void update_front_fire() {
    // front更新
    update_heat(heat_front, FIRE_WIDTH, FIRE_HEIGHT);
    draw_heat_to_canvas(heat_front, front_canvas, FIRE_WIDTH, FIRE_HEIGHT);

    // === ここから角取り（マスク）処理 ===
    int cx = FRONT_WIDTH / 2;
    int cy = FRONT_HEIGHT / 2;
    int r = (FRONT_WIDTH < FRONT_HEIGHT) ? FRONT_WIDTH/2 : FRONT_HEIGHT/2; 
    // rはパネルの半分程度の長さ(円形マスク半径)
    // もう少し小さくして明確に四隅をカットしたいなら rを小さめにしてもよい
    // 例えば r = (FRONT_WIDTH < FRONT_HEIGHT) ? (FRONT_WIDTH/2 - 2) : (FRONT_HEIGHT/2 - 2);

    for (int y = 0; y < FIRE_HEIGHT; y++) {
        for (int x = 0; x < FIRE_WIDTH; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist2 = dx*dx + dy*dy;
            int r2 = r*r;
            // 円の外側なら黒にする
            if (dist2 > r2) {
                front_canvas[y][x] = RAINBOW_PALETTE_SIZE; // 黒インデックス
            }
        }
    }
    // === 角取り（マスク）処理ここまで ===

    front_matrix_draw_from_palette(fire_pal, RAINBOW_PALETTE_SIZE, front_canvas);
    flush_front_and_wait();
}

void update_back_fire() {
    // back更新
    update_heat_back(heat_back, BACK_FIRE_WIDTH, BACK_FIRE_HEIGHT);
    draw_heat_to_canvas_back(heat_back, back_canvas, BACK_FIRE_WIDTH,
                             BACK_FIRE_HEIGHT);
    back_matrix_draw_from_palette(fire_pal, RAINBOW_PALETTE_SIZE, back_canvas);
    flush_back_and_wait();
}

void light_mode_fire() {
    frame_counter++;
    update_front_fire();
    update_back_fire();

    vTaskDelay(pdMS_TO_TICKS(15));
}
