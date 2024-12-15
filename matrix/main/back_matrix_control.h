#pragma once
#include <stdint.h>
#include "led_strip.h"

// バック側マトリクス全体サイズ
#define BACK_TOTAL_WIDTH 32
#define BACK_TOTAL_HEIGHT 32
#define BACK_TOTAL_LEDS (BACK_TOTAL_WIDTH * BACK_TOTAL_HEIGHT)

void back_matrix_init();
void back_matrix_set_pixel_color(int x, int y, rgb_t color);
void back_matrix_blink();
void back_matrix_light_sequentially();
void blank_back_matrix();
void clear_back_matrix();
void flush_back();
void back_matrix_draw_from_palette(const rgb_t *palette, int palette_count,
                                   const uint8_t canvas[BACK_TOTAL_HEIGHT][BACK_TOTAL_WIDTH]);
