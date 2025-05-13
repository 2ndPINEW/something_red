#pragma once
#include "led_strip.h"

#define FRONT_WIDTH 16
#define FRONT_HEIGHT 16
#define FRONT_LEDS (FRONT_WIDTH * FRONT_HEIGHT)

// テープライトの各セグメント当たりのLED数
#define TAPE_LEDS 30

// テープライトが4本あると仮定
#define TAPE_COUNT 4
// 合計LED数 (フロント + テープ4本)
#define TOTAL_LEDS (FRONT_LEDS + TAPE_LEDS * TAPE_COUNT)

// 各テープの開始インデックス
#define TAPE1_START (FRONT_LEDS)                          // 256
#define TAPE2_START (TAPE1_START + TAPE_LEDS)             // 256+30=286
#define TAPE3_START (TAPE2_START + TAPE_LEDS)             // 286+30=316
#define TAPE4_START (TAPE3_START + TAPE_LEDS)             // 316+30=346

void front_matrix_init();
void front_matrix_set_pixel_color(int x, int y, rgb_t color);
void front_matrix_fill_color(rgb_t color);
void front_matrix_blink();
void front_matrix_light_sequentially();
void clear_front_matrix();
void flush_front();
void flush_front_and_wait();
void front_matrix_draw_from_palette(const rgb_t *palette, int palette_count,
                                    const uint8_t canvas[FRONT_HEIGHT][FRONT_WIDTH]);

// テープライトセグメント識別子
typedef enum {
    TAPE1, // 左後ろ 手前→奥 (forward)
    TAPE2, // 左前 奥→手前 (reverse)
    TAPE3, // 右前 手前→奥 (forward)
    TAPE4  // 右後ろ 奥→手前 (reverse)
} tape_segment_t;

void tape_set_pixel_color(tape_segment_t tape, int i, rgb_t color);
void tape_fill(tape_segment_t tape, rgb_t color);
void tape_light_sequentially(tape_segment_t tape);
void tape_draw_from_palette(const rgb_t *palette, int palette_count,
                            const uint8_t tape_canvas[TAPE_LEDS], tape_segment_t tape);
void tape_draw_all_from_palette(const rgb_t *palette, int palette_count,
                                const uint8_t tape_canvases[TAPE_COUNT][TAPE_LEDS]);
