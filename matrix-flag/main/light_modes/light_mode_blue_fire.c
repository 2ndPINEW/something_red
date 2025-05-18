#include "light_mode_blue_fire.h"
#include "../led_matrix_control.h"
#include "../color_palette.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include "../matrix_utils.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define FIRE_WIDTH LED_TOTAL_WIDTH
#define FIRE_HEIGHT LED_TOTAL_HEIGHT
#define TEXT_SWITCH_FRAMES 200
#define SCROLL_ANIMATION_FRAMES 20

static const rgb_t *blue_fire_pal = NULL;

// キャンバスと熱分布データ
static uint8_t canvas[FIRE_HEIGHT][FIRE_WIDTH];
static uint8_t heat[FIRE_HEIGHT][FIRE_WIDTH];

static int frame_counter = 0;
static int animation_counter = 0;
static bool is_animating = false;
static bool completed_one_cycle = false; // 1周したかどうかのフラグ

#define TEXT_OPTIONS_LENGTH 20

// テキスト切り替え用
static const char* text_options[TEXT_OPTIONS_LENGTH] = {
    "BLUE GIANTS",
    "55 GOGO",
    "9 MAYU",
    "38 SAYA",
    "8 EITO",
    "14 TENMA",
    "62 MUTSUMI",
    "88 SARA",
    "24 SAKI",
    "73 KODAMA",
    "28 KAJIYA",
    "17 SHU",
    "81 SHOKO",
    "91 AYATAKA",
    "78 TOMOYA",
    "21 HORISO",
    "31 MAI",
    "22 HISHO",
    "40 NAGAO",
    "29 MORITA",
};
static int current_text_index = 0;

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
    
    // パレットの最後の色を純粋な白色に設定（テキスト表示用）
    blue_fire_palette[RAINBOW_PALETTE_SIZE - 1] = (rgb_t){.r = 255, .g = 255, .b = 255};
    
    blue_fire_pal = blue_fire_palette;
}

void reset_light_mode_blue_fire() {
    generate_blue_fire_palette();
    
    // カウンターとアニメーション状態をリセット
    frame_counter = 0;
    animation_counter = 0;
    is_animating = false;
    current_text_index = 0;
    completed_one_cycle = false; // 周回フラグもリセット

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

// スクロールアニメーション付きでテキストを描画する関数
static void draw_text_overlay() {
    // 縦方向の位置を調整
    int y_pos = (FIRE_HEIGHT - get_font_5x7()->height * 3 / 2) / 2 - 2;
    
    // 純粋な白色を使用するために、パレットの最大値を使用
    uint8_t white_color_index = RAINBOW_PALETTE_SIZE - 1;
    
    // アニメーション中かどうかで描画方法を変える
    if (is_animating) {
        // アニメーション中
        const char* current_text = text_options[current_text_index];
        const char* next_text = text_options[(current_text_index + 1) % TEXT_OPTIONS_LENGTH];
        
        // アニメーションの進行度（0.0～1.0）
        float progress = (float)animation_counter / SCROLL_ANIMATION_FRAMES;
        
        // アニメーションを2段階に分ける
        if (progress < 0.5) {
            // 前半: 現在のテキストを左にスクロールアウト
            // 0→0.5の進行度を0→1にスケーリング
            float exit_progress = progress * 2.0;
            int current_offset = -(int)(exit_progress * LED_TOTAL_WIDTH * 1.2);
            
            // 現在のテキストを描画（左にスクロールアウト）
            draw_text_to_canvas_1_5x(canvas, current_text, current_offset, y_pos, white_color_index, get_font_5x7());
        } else {
            // 後半: 次のテキストを右からスクロールイン
            // 0.5→1.0の進行度を0→1にスケーリング
            float enter_progress = (progress - 0.5) * 2.0;
            
            // テキストの長さを取得して、完全に入った時に中央に表示されるよう調整
            int text_width = 0;
            for (int i = 0; next_text[i] != '\0'; i++) {
                if (next_text[i] == ' ') {
                    text_width += (get_font_5x7()->width * 3) / 4; // スペースは半分の幅
                } else {
                    text_width += (get_font_5x7()->width * 3) / 2 + 3; // 1.5倍の文字幅 + 間隔
                }
            }
            if (text_width > 0) text_width -= 3; // 最後の文字の後の間隔は不要
            
            // 中央位置を計算
            int center_x = (LED_TOTAL_WIDTH - text_width) / 2;
            
            // 右からスワイプして最終的に中央に停止するように位置を計算
            int next_offset = center_x + (int)((1.0 - enter_progress) * LED_TOTAL_WIDTH);
            
            // 次のテキストを描画（右からスクロールイン）
            draw_text_to_canvas_1_5x(canvas, next_text, next_offset, y_pos, white_color_index, get_font_5x7());
        }
        
        // アニメーションカウンターを進める
        animation_counter++;
        
        // アニメーション終了判定
        if (animation_counter >= SCROLL_ANIMATION_FRAMES) {
            is_animating = false;
            animation_counter = 0;
            
            // 次のテキストインデックスを計算
            int next_index = (current_text_index + 1) % TEXT_OPTIONS_LENGTH;
            current_text_index = next_index;
            
            // 一周したかチェック（最初のテキストに戻ったら）
            if (next_index == 0) {
                completed_one_cycle = true;
            }
        }
    } else {
        // 通常時は現在のテキストを中央に表示
        const char* text = text_options[current_text_index];
        draw_centered_text_to_canvas_1_5x(canvas, text, y_pos, white_color_index, get_font_5x7());
    }
}

void light_mode_blue_fire() {
    frame_counter++;
    
    // 3秒ごとにテキスト切り替えアニメーションを開始（1周したら停止）
    if (!is_animating && !completed_one_cycle && frame_counter >= TEXT_SWITCH_FRAMES) {
        is_animating = true;
        animation_counter = 0;
        frame_counter = 0;
    }
    
    // 熱分布の更新
    update_heat();
    
    // キャンバスに描画
    draw_heat_to_canvas();
    
    // テキストをオーバーレイ
    draw_text_overlay();
    
    // LEDマトリックスに描画
    led_matrix_draw_from_palette(blue_fire_pal, RAINBOW_PALETTE_SIZE, canvas);
    flush_led_and_wait();
    
    // 少し待機
    vTaskDelay(pdMS_TO_TICKS(15));
}
