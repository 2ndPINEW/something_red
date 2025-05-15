#include "light_mode_blue_flush.h"
#include "../led_matrix_control.h"
#include "../color_palette.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

// 点滅状態を保持する変数
static bool is_blue_on = false;

void reset_light_mode_blue_flush() {
    // リセット時に状態を初期化
    is_blue_on = false;
}

void light_mode_blue_flush() {
    // 青色の定義
    rgb_t blue = (rgb_t){.r = 0, .g = 0, .b = 255};
    rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
    
    // 点滅状態を切り替え
    is_blue_on = !is_blue_on;
    
    // 状態に応じて青色または消灯
    if (is_blue_on) {
        led_matrix_fill_color(blue);
    } else {
        led_matrix_fill_color(off);
    }
    
    // LEDの更新と待機
    flush_led_and_wait();
    
    // 100ms待機
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
