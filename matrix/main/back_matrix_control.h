#pragma once
#include "esp_ws28xx.h"

// バック側マトリクス全体サイズ
#define BACK_TOTAL_WIDTH 32
#define BACK_TOTAL_HEIGHT 32
#define BACK_TOTAL_LEDS (BACK_TOTAL_WIDTH * BACK_TOTAL_HEIGHT)

void back_matrix_init();
void back_matrix_set_pixel_color(int x, int y, CRGB color);
void back_matrix_blink();
void back_matrix_light_sequentially();
