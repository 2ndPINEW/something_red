#pragma once
#include <stdint.h>

// アニメーション関連の定数
#define MAX_PALETTE_COLORS 256
#define MAX_ANIMATION_FRAMES 64

// アニメーションデータを受信する関数
void animation_receive_data(const char *data);

// 初期化関数
void light_controller_init();
