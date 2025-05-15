#include "led_matrix_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "matrix_utils.h"
#include <stdio.h>

// ESP32-S3で使用可能なGPIOピン（ブートや特殊機能に使われないもの）
// 各GPIOインデックスに対応するピン番号を定義
static const int LED_GPIO_PINS[LED_GPIO_COUNT] = {4, 5, 15, 16};

static const char *TAG = "LED_MATRIX_CONTROL";

// デフォルトの明るさ
#define DEFAULT_BRIGHTNESS 5

// 各GPIOに対応するLEDストリップの配列
static led_strip_t led_strips[LED_GPIO_COUNT];

static uint8_t led_state_off = 0;
// 未使用の変数を削除

// (x, y)座標から適切なGPIOインデックス、ストリップインデックス、LEDインデックスを計算
// 返り値: GPIOインデックス（0-3）
// strip_index: そのGPIO内のストリップインデックス（0-8）
// led_index: そのストリップ内のLEDインデックス（0-107）
static int get_gpio_strip_led_index(int x, int y, int *strip_index, int *led_index) {
    // y座標から、全体のストリップインデックス（0-35）を計算
    int global_strip_index = y;
    
    // 全体のストリップインデックスから、GPIOインデックスとそのGPIO内のストリップインデックスを計算
    int gpio_index = global_strip_index / LED_STRIPS_PER_GPIO;
    *strip_index = global_strip_index % LED_STRIPS_PER_GPIO;
    
    // GPIO 1と3は右上から配置されているため、計算方法を変更
    if (gpio_index == 1 || gpio_index == 3) {
        // GPIO 1と3の場合
        if (*strip_index % 2 == 0) {
            // 偶数番目のストリップは右から左（逆順）
            *led_index = LED_TOTAL_WIDTH - 1 - x;
        } else {
            // 奇数番目のストリップは左から右（通常順）
            *led_index = x;
        }
    } else {
        // GPIO 0と2の場合（元の計算方法）
        if (*strip_index % 2 == 0) {
            // 偶数番目のストリップは左から右（通常順）
            *led_index = x;
        } else {
            // 奇数番目のストリップは右から左（逆順）
            *led_index = LED_TOTAL_WIDTH - 1 - x;
        }
    }
    
    return gpio_index;
}

void flush_led() {
    // 全GPIOのLEDストリップをフラッシュ
    for (int i = 0; i < LED_GPIO_COUNT; i++) {
        led_strip_flush(&led_strips[i]);
    }
}

void flush_led_and_wait() {
    flush_led();
    vTaskDelay(30 / portTICK_PERIOD_MS);
}

void clear_led_matrix() {
    // 全GPIOのLEDストリップをクリア
    for (int i = 0; i < LED_GPIO_COUNT; i++) {
        led_strip_fill(&led_strips[i], 0, LEDS_PER_STRIP * LED_STRIPS_PER_GPIO,
                      (rgb_t){.r = 0, .g = 0, .b = 0});
    }
}

void led_matrix_init() {
    // 各GPIOのLEDストリップを初期化
    for (int i = 0; i < LED_GPIO_COUNT; i++) {
        led_strips[i] = (led_strip_t){
            .type = LED_STRIP_WS2812,
            .is_rgbw = false,
            .length = LEDS_PER_STRIP * LED_STRIPS_PER_GPIO,
            .gpio = LED_GPIO_PINS[i],  // 定義したGPIOピン番号を使用
            .channel = i,  // RMT_CHANNEL_0, 1, 2, 3
            .brightness = DEFAULT_BRIGHTNESS
        };
        
        ESP_ERROR_CHECK(led_strip_init(&led_strips[i]));
    }
    
    clear_led_matrix();
    flush_led();
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void led_matrix_set_pixel_color(int x, int y, rgb_t color) {
    if (x < 0 || x >= LED_TOTAL_WIDTH || y < 0 || y >= LED_TOTAL_HEIGHT) {
        return;
    }

    int strip_index, led_index;
    int gpio_index = get_gpio_strip_led_index(x, y, &strip_index, &led_index);
    
    // ストリップ内での実際のインデックスを計算
    int actual_index = strip_index * LEDS_PER_STRIP + led_index;
    
    led_strip_set_pixel(&led_strips[gpio_index], actual_index, color);
}

void blank_led_matrix() {
    for (int y = 0; y < LED_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < LED_TOTAL_WIDTH; x++) {
            led_matrix_set_pixel_color(x, y, (rgb_t){.r = 0, .g = 0, .b = 0});
        }
    }
}

void led_matrix_fill_color(rgb_t color) {
    for (int i = 0; i < LED_GPIO_COUNT; i++) {
        led_strip_fill(&led_strips[i], 0, LEDS_PER_STRIP * LED_STRIPS_PER_GPIO, color);
    }
}

// palette: rgb_t配列（パレット）
// palette_count: パレット色数
// canvas: 108x36のパレットインデックス配列（0～palette_count-1の値）
void led_matrix_draw_from_palette(
    const rgb_t *palette, int palette_count,
    const uint8_t canvas[LED_TOTAL_HEIGHT][LED_TOTAL_WIDTH]) {
    led_matrix_draw_from_palette_with_brightness(palette, palette_count, canvas, DEFAULT_BRIGHTNESS);
}

// 明るさを考慮したパレット描画関数
void led_matrix_draw_from_palette_with_brightness(
    const rgb_t *palette, int palette_count,
    const uint8_t canvas[LED_TOTAL_HEIGHT][LED_TOTAL_WIDTH],
    uint8_t brightness) {
    // 全GPIOのLEDストリップの明るさを設定
    for (int i = 0; i < LED_GPIO_COUNT; i++) {
        led_strips[i].brightness = brightness;
    }
    ESP_LOGI(TAG, "Setting LED strip brightness to %d", brightness);
    
    for (int y = 0; y < LED_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < LED_TOTAL_WIDTH; x++) {
            uint8_t index = canvas[y][x];
            if (index < palette_count) {
                // パレットから色を取得（明るさはLEDストリップ全体で制御）
                led_matrix_set_pixel_color(x, y, palette[index]);
            } else {
                led_matrix_set_pixel_color(x, y, (rgb_t){.r = 0, .g = 0, .b = 0});
            }
            if (x == LED_TOTAL_WIDTH - 1 && y == LED_TOTAL_HEIGHT - 1) {
                ESP_LOGI(TAG, "x: %d, y: %d, index: %d, brightness: %d", 
                         x, y, index, brightness);
            }
        }
    }
}
