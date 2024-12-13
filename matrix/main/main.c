#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_ws28xx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

#define LEFT_LED_GPIO 1
#define RIGHT_LED_GPIO 2

// 全体サイズ
#define BACK_TOTAL_WIDTH 32
#define BACK_TOTAL_HEIGHT 32
#define BACK_TOTAL_LEDS (BACK_TOTAL_WIDTH * BACK_TOTAL_HEIGHT)

// 左右半分のサイズ
#define BACK_HALF_WIDTH 16
#define BACK_HALF_HEIGHT 32
#define BACK_HALF_LEDS (BACK_HALF_WIDTH * BACK_HALF_HEIGHT)

static const char *TAG = "main";
static uint8_t led_state_off = 0;

CRGB *ws2812_buffer_left;
CRGB *ws2812_buffer_right;

int getLedIndex(int x, int y, int width) {
    if (y % 2 == 0) {
        // 偶数行: 左→右
        return y * width + x;
    } else {
        // 奇数行: 右→左
        return (y + 1) * width - 1 - x;
    }
}

// グローバル座標 (x, y) を用いてLEDをセットする関数
// (x, y)は0 <= x < 32, 0 <= y < 32
void setPixelColor(int x, int y, CRGB color) {
    if (x < BACK_HALF_WIDTH) {
        // 左半分
        int index = getLedIndex(x, y, BACK_HALF_WIDTH);
        ws2812_buffer_left[index] = color;
    } else {
        // 右半分
        int index = getLedIndex(x - BACK_HALF_WIDTH, y, BACK_HALF_WIDTH);
        ws2812_buffer_right[index] = color;
    }
}

// マトリクス全体を点滅させる例
void blink_led_matrix(void) {
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            if (led_state_off) {
                setPixelColor(x, y, (CRGB){.r = 0, .g = 0, .b = 0});
            } else {
                // 赤く点灯
                setPixelColor(x, y, (CRGB){.r = 50, .g = 0, .b = 0});
            }
        }
    }

    // 左右のパネルを更新
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_update()); //
    // ws28xx_update()が全ての初期化済みストリップを更新する仕様であればこれでOK
    // 個別に更新が必要な場合は、ライブラリ仕様に合わせて ws28xx_update()
    // 呼び出し時にハンドル指定するなど対応
}

void app_main(void) {
    // 左パネル初期化
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_init(
        LEFT_LED_GPIO, WS2812B, BACK_HALF_LEDS, &ws2812_buffer_left));

    // 右パネル初期化
    ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_init(
        RIGHT_LED_GPIO, WS2812B, BACK_HALF_LEDS, &ws2812_buffer_right));

    while (1) {
        printf("%s: Turning the LED matrix %s!\n", TAG,
               led_state_off == true ? "ON" : "OFF");
        blink_led_matrix();
        led_state_off = !led_state_off;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
