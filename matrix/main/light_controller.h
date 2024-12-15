#pragma once
#include <stdint.h>

typedef enum {
    LIGHT_MODE_RAINBOW_SCROLL = 0,
    LIGHT_MODE_OFF,
    LIGHT_MODE_BLINK,
    // 必要に応じて他モード追加
} light_mode_t;

void light_controller_init();       // 初期化: LEDやパレットのセットアップ、タスク起動
void light_controller_set_mode(light_mode_t mode); // モード変更
