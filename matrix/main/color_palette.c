#include "color_palette.h"
#include <math.h>
#include <stdbool.h>

// 内部で保持するパレット配列
static rgb_t palette[PALETTE_SIZE];
static bool palette_initialized = false;

// HSV→RGB変換
static rgb_t hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float hh = fmodf(h / 60.0f, 6);
    float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    float r=0, g=0, b=0;
    if (0 <= hh && hh < 1) {r=c;g=x;b=0;}
    else if (1 <= hh && hh < 2){r=x;g=c;b=0;}
    else if (2 <= hh && hh < 3){r=0;g=c;b=x;}
    else if (3 <= hh && hh < 4){r=0;g=x;b=c;}
    else if (4 <= hh && hh < 5){r=x;g=0;b=c;}
    else if (5 <= hh && hh < 6){r=c;g=0;b=x;}

    float m = v - c;
    r += m; g += m; b += m;

    rgb_t color = {
        .r = (uint8_t)(r * 255.0f),
        .g = (uint8_t)(g * 255.0f),
        .b = (uint8_t)(b * 255.0f)
    };
    return color;
}

void color_palette_init(void) {
    if (!palette_initialized) {
        for (int i = 0; i < PALETTE_SIZE; i++) {
            float h = (360.0f / PALETTE_SIZE) * i;
            float s = 1.0f;
            float v = 1.0f;
            palette[i] = hsv_to_rgb(h, s, v);
        }
        palette_initialized = true;
    }
}

const rgb_t* color_palette_get(void) {
    if (!palette_initialized) {
        // 必要なら自動初期化
        color_palette_init();
    }
    return palette;
}
