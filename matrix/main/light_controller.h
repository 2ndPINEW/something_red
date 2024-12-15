#pragma once
#include <stdint.h>

typedef enum {
    LIGHT_MODE_INIT = 0,
    LIGHT_MODE_OFF,
    LIGHT_MODE_RAINBOW_SCROLL,
    LIGHT_MODE_BLINK_RED,
    LIGHT_MODE_PACHINKO,
    LIGHT_MODE_FIRE,
} light_mode_t;

void light_controller_init();
void light_controller_set_mode(light_mode_t mode);
