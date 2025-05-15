#pragma once
#include <stdint.h>

typedef enum {
    LIGHT_MODE_INIT = 0,
    LIGHT_MODE_OFF,
    LIGHT_MODE_BLUE_FLUSH,
    LIGHT_MODE_BLUE_FIRE,
} light_mode_t;

void light_controller_init();
void light_controller_set_mode(light_mode_t mode);
