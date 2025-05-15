#pragma once
#include <stdint.h>
#include "led_matrix_control.h"

int get_matrix_panel_led_index(int x, int y, int width);

// フォント定義用の構造体
typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint8_t *data;
} font_t;

// テキスト描画関数
void draw_text_to_canvas(uint8_t canvas[][LED_TOTAL_WIDTH], 
                         const char *text, 
                         int x, int y, 
                         uint8_t color_index, 
                         const font_t *font);

// 1.5倍サイズのテキスト描画関数
void draw_text_to_canvas_1_5x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                             const char *text, 
                             int x, int y, 
                             uint8_t color_index, 
                             const font_t *font);

// 2倍サイズのテキスト描画関数
void draw_text_to_canvas_2x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                           const char *text, 
                           int x, int y, 
                           uint8_t color_index, 
                           const font_t *font);

// テキストを中央に配置して描画する関数
void draw_centered_text_to_canvas(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                 const char *text, 
                                 int y, 
                                 uint8_t color_index, 
                                 const font_t *font);

// 1.5倍サイズのテキストを中央に配置して描画する関数
void draw_centered_text_to_canvas_1_5x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                      const char *text, 
                                      int y, 
                                      uint8_t color_index, 
                                      const font_t *font);

// 2倍サイズのテキストを中央に配置して描画する関数
void draw_centered_text_to_canvas_2x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                    const char *text, 
                                    int y, 
                                    uint8_t color_index, 
                                    const font_t *font);

// 5x7ピクセルフォントを取得する関数
const font_t* get_font_5x7(void);
