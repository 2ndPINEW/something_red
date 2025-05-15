#pragma once
#include <stdint.h>
#include "led_strip.h"

#define RAINBOW_PALETTE_SIZE 128

void color_palette_init(void);

const rgb_t* color_palette_get(void);
