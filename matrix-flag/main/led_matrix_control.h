#pragma once
#include <stdint.h>
#include "led_strip.h"

// LEDマトリクス全体サイズ
#define LED_TOTAL_WIDTH 108
#define LED_TOTAL_HEIGHT 36
#define LED_TOTAL_LEDS (LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT)

// ESP32-S3には4つのRMTチャネルしかないため、GPIOを4つに制限
// 1つのGPIOあたりのLEDテープ本数
#define LED_STRIPS_PER_GPIO 9  // 36本のテープを4つのGPIOで制御（9本/GPIO）
// GPIOの数
#define LED_GPIO_COUNT 4       // ESP32-S3のRMTチャネル数に合わせる
// 1本のLEDテープあたりのLED数
#define LEDS_PER_STRIP LED_TOTAL_WIDTH

void led_matrix_init();
void led_matrix_set_pixel_color(int x, int y, rgb_t color);
void led_matrix_fill_color(rgb_t color);
void blank_led_matrix();
void clear_led_matrix();
void flush_led();
void flush_led_and_wait();
void led_matrix_draw_from_palette(const rgb_t *palette, int palette_count,
                                   const uint8_t canvas[LED_TOTAL_HEIGHT][LED_TOTAL_WIDTH]);
                                   
// 明るさを考慮したパレット描画関数
void led_matrix_draw_from_palette_with_brightness(const rgb_t *palette, int palette_count,
                                   const uint8_t canvas[LED_TOTAL_HEIGHT][LED_TOTAL_WIDTH],
                                   uint8_t brightness);
