#pragma once
#include "esp_ws28xx.h"

#define FRONT_WIDTH 16
#define FRONT_HEIGHT 16
#define FRONT_LEDS (FRONT_WIDTH * FRONT_HEIGHT)

void front_matrix_init();
void front_matrix_set_pixel_color(int x, int y, CRGB color);
void front_matrix_blink();
void front_matrix_light_sequentially();
