#pragma once
#include <stdint.h>
#include "led_strip.h"

#define PALETTE_SIZE 256

void color_palette_init(void);

const rgb_t* color_palette_get(void);
