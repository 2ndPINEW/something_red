#include "matrix_utils.h"
#include <stdio.h>
#include <string.h>

int get_matrix_panel_led_index(int x, int y, int width) {
    if (y % 2 == 0) {
        // 偶数行: 右→左
        return (y + 1) * width - 1 - x;
    } else {
        // 奇数行: 左→右
        return y * width + x;
    }
}

// 5x7ピクセルフォントデータ（アルファベット大文字と数字）
// 各文字は5x7ピクセルで、1バイトの各ビットが1行を表す
static const uint8_t font_5x7_data[] = {
    // A
    0x7E, 0x09, 0x09, 0x09, 0x7E,
    // B
    0x7F, 0x49, 0x49, 0x49, 0x36,
    // C
    0x3E, 0x41, 0x41, 0x41, 0x22,
    // D
    0x7F, 0x41, 0x41, 0x22, 0x1C,
    // E
    0x7F, 0x49, 0x49, 0x49, 0x41,
    // F
    0x7F, 0x09, 0x09, 0x09, 0x01,
    // G
    0x3E, 0x41, 0x49, 0x49, 0x7A,
    // H
    0x7F, 0x08, 0x08, 0x08, 0x7F,
    // I
    0x00, 0x41, 0x7F, 0x41, 0x00,
    // J
    0x20, 0x40, 0x41, 0x3F, 0x01,
    // K
    0x7F, 0x08, 0x14, 0x22, 0x41,
    // L
    0x7F, 0x40, 0x40, 0x40, 0x40,
    // M
    0x7F, 0x02, 0x0C, 0x02, 0x7F,
    // N
    0x7F, 0x04, 0x08, 0x10, 0x7F,
    // O
    0x3E, 0x41, 0x41, 0x41, 0x3E,
    // P
    0x7F, 0x09, 0x09, 0x09, 0x06,
    // Q
    0x3E, 0x41, 0x51, 0x21, 0x5E,
    // R
    0x7F, 0x09, 0x19, 0x29, 0x46,
    // S
    0x46, 0x49, 0x49, 0x49, 0x31,
    // T
    0x01, 0x01, 0x7F, 0x01, 0x01,
    // U
    0x3F, 0x40, 0x40, 0x40, 0x3F,
    // V
    0x1F, 0x20, 0x40, 0x20, 0x1F,
    // W
    0x3F, 0x40, 0x38, 0x40, 0x3F,
    // X
    0x63, 0x14, 0x08, 0x14, 0x63,
    // Y
    0x07, 0x08, 0x70, 0x08, 0x07,
    // Z
    0x61, 0x51, 0x49, 0x45, 0x43,
    // 0
    0x3E, 0x51, 0x49, 0x45, 0x3E,
    // 1
    0x00, 0x42, 0x7F, 0x40, 0x00,
    // 2
    0x42, 0x61, 0x51, 0x49, 0x46,
    // 3
    0x21, 0x41, 0x45, 0x4B, 0x31,
    // 4
    0x18, 0x14, 0x12, 0x7F, 0x10,
    // 5
    0x27, 0x45, 0x45, 0x45, 0x39,
    // 6
    0x3C, 0x4A, 0x49, 0x49, 0x30,
    // 7
    0x01, 0x71, 0x09, 0x05, 0x03,
    // 8
    0x36, 0x49, 0x49, 0x49, 0x36,
    // 9
    0x06, 0x49, 0x49, 0x29, 0x1E,
    // スペース
    0x00, 0x00, 0x00, 0x00, 0x00
};

// 5x7フォント構造体
static const font_t font_5x7 = {
    .width = 5,
    .height = 7,
    .data = font_5x7_data
};

// フォントを取得する関数
const font_t* get_font_5x7(void) {
    return &font_5x7;
}

// 文字を描画する内部関数
static void draw_char_to_canvas(uint8_t canvas[][LED_TOTAL_WIDTH], 
                               char c, 
                               int x, int y, 
                               uint8_t color_index, 
                               const font_t *font) {
    // アルファベット大文字と数字をサポート
    int char_index = -1;
    
    if (c >= 'A' && c <= 'Z') {
        char_index = c - 'A';
    } else if (c >= '0' && c <= '9') {
        // 数字は、アルファベットの後に配置されている
        char_index = 26 + (c - '0'); // 26文字のアルファベットの後
    } else if (c == ' ') {
        // スペースは何も描画しない
        return;
    } else {
        // サポートされていない文字
        return;
    }
    
    const uint8_t *char_data = &font->data[char_index * font->width];
    
    for (int cy = 0; cy < font->height; cy++) {
        for (int cx = 0; cx < font->width; cx++) {
            // 各ビットをチェック（上から下へ）
            if ((char_data[cx] >> cy) & 0x01) {
                // キャンバスの範囲内かチェック
                if (x + cx >= 0 && x + cx < LED_TOTAL_WIDTH && 
                    y + cy >= 0 && y + cy < LED_TOTAL_HEIGHT) {
                    canvas[y + cy][x + cx] = color_index;
                }
            }
        }
    }
}

// テキストを描画する関数
void draw_text_to_canvas(uint8_t canvas[][LED_TOTAL_WIDTH], 
                         const char *text, 
                         int x, int y, 
                         uint8_t color_index, 
                         const font_t *font) {
    int cursor_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char_to_canvas(canvas, text[i], cursor_x, y, color_index, font);
        cursor_x += font->width + 1; // 文字間に1ピクセルの間隔
    }
}

// 1.5倍サイズの文字を描画する内部関数
static void draw_char_to_canvas_1_5x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                    char c, 
                                    int x, int y, 
                                    uint8_t color_index, 
                                    const font_t *font) {
    // アルファベット大文字と数字をサポート
    int char_index = -1;
    
    if (c >= 'A' && c <= 'Z') {
        char_index = c - 'A';
    } else if (c >= '0' && c <= '9') {
        // 数字は、アルファベットの後に配置されている
        char_index = 26 + (c - '0'); // 26文字のアルファベットの後
    } else if (c == ' ') {
        // スペースは何も描画しない
        return;
    } else {
        // サポートされていない文字
        return;
    }
    
    const uint8_t *char_data = &font->data[char_index * font->width];
    
    for (int cy = 0; cy < font->height; cy++) {
        for (int cx = 0; cx < font->width; cx++) {
            // 各ビットをチェック（上から下へ）
            if ((char_data[cx] >> cy) & 0x01) {
                // 1.5倍サイズで描画（3x3のパターンを2x2のグリッドに配置）
                // これにより文字がつぶれにくくなる
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        // 1.5倍のスケーリング（整数演算で近似）
                        int px = x + (cx * 3) / 2 + dx;
                        int py = y + (cy * 3) / 2 + dy;
                        
                        // キャンバスの範囲内かチェック
                        if (px >= 0 && px < LED_TOTAL_WIDTH && 
                            py >= 0 && py < LED_TOTAL_HEIGHT) {
                            canvas[py][px] = color_index;
                        }
                    }
                }
            }
        }
    }
}

// 2倍サイズの文字を描画する内部関数
static void draw_char_to_canvas_2x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                  char c, 
                                  int x, int y, 
                                  uint8_t color_index, 
                                  const font_t *font) {
    // アルファベット大文字と数字をサポート
    int char_index = -1;
    
    if (c >= 'A' && c <= 'Z') {
        char_index = c - 'A';
    } else if (c >= '0' && c <= '9') {
        // 数字は、アルファベットの後に配置されている
        char_index = 26 + (c - '0'); // 26文字のアルファベットの後
    } else if (c == ' ') {
        // スペースは何も描画しない
        return;
    } else {
        // サポートされていない文字
        return;
    }
    
    const uint8_t *char_data = &font->data[char_index * font->width];
    
    for (int cy = 0; cy < font->height; cy++) {
        for (int cx = 0; cx < font->width; cx++) {
            // 各ビットをチェック（上から下へ）
            if ((char_data[cx] >> cy) & 0x01) {
                // 2x2のブロックとして描画
                for (int dy = 0; dy < 2; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        // キャンバスの範囲内かチェック
                        if (x + cx*2 + dx >= 0 && x + cx*2 + dx < LED_TOTAL_WIDTH && 
                            y + cy*2 + dy >= 0 && y + cy*2 + dy < LED_TOTAL_HEIGHT) {
                            canvas[y + cy*2 + dy][x + cx*2 + dx] = color_index;
                        }
                    }
                }
            }
        }
    }
}

// 1.5倍サイズのテキストを描画する関数
void draw_text_to_canvas_1_5x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                             const char *text, 
                             int x, int y, 
                             uint8_t color_index, 
                             const font_t *font) {
    int cursor_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ' ') {
            // スペースは半分の幅に
            cursor_x += (font->width * 3) / 4; // 通常の1.5倍の半分
        } else {
            draw_char_to_canvas_1_5x(canvas, text[i], cursor_x, y, color_index, font);
            cursor_x += (font->width * 3) / 2 + 3; // 文字間に3ピクセルの間隔（読みやすさ向上）
        }
    }
}

// 2倍サイズのテキストを描画する関数
void draw_text_to_canvas_2x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                           const char *text, 
                           int x, int y, 
                           uint8_t color_index, 
                           const font_t *font) {
    int cursor_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ' ') {
            // スペースは半分の幅に
            cursor_x += font->width; // 通常の2倍の半分
        } else {
            draw_char_to_canvas_2x(canvas, text[i], cursor_x, y, color_index, font);
            cursor_x += font->width * 2 + 2; // 文字間に2ピクセルの間隔（2倍サイズなので）
        }
    }
}

// テキストを中央に配置して描画する関数
void draw_centered_text_to_canvas(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                 const char *text, 
                                 int y, 
                                 uint8_t color_index, 
                                 const font_t *font) {
    // テキスト全体の幅を計算（スペースの幅を考慮）
    int text_width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ' ') {
            text_width += (font->width + 1) / 2; // スペースは半分の幅
        } else {
            text_width += font->width + 1; // 通常の文字幅 + 間隔
        }
    }
    if (text_width > 0) text_width -= 1; // 最後の文字の後の間隔は不要
    
    int x = (LED_TOTAL_WIDTH - text_width) / 2;
    draw_text_to_canvas(canvas, text, x, y, color_index, font);
}

// 1.5倍サイズのテキストを中央に配置して描画する関数
void draw_centered_text_to_canvas_1_5x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                      const char *text, 
                                      int y, 
                                      uint8_t color_index, 
                                      const font_t *font) {
    // テキスト全体の幅を計算（スペースの幅を考慮）
    int text_width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ' ') {
            text_width += (font->width * 3) / 4; // スペースは半分の幅
        } else {
            text_width += (font->width * 3) / 2 + 3; // 1.5倍の文字幅 + 3ピクセルの間隔
        }
    }
    if (text_width > 0) text_width -= 3; // 最後の文字の後の間隔は不要
    
    int x = (LED_TOTAL_WIDTH - text_width) / 2;
    draw_text_to_canvas_1_5x(canvas, text, x, y, color_index, font);
}

// 2倍サイズのテキストを中央に配置して描画する関数
void draw_centered_text_to_canvas_2x(uint8_t canvas[][LED_TOTAL_WIDTH], 
                                    const char *text, 
                                    int y, 
                                    uint8_t color_index, 
                                    const font_t *font) {
    // テキスト全体の幅を計算（スペースの幅を考慮）
    int text_width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == ' ') {
            text_width += font->width; // スペースは半分の幅
        } else {
            text_width += font->width * 2 + 2; // 2倍の文字幅 + 間隔
        }
    }
    if (text_width > 0) text_width -= 2; // 最後の文字の後の間隔は不要
    
    int x = (LED_TOTAL_WIDTH - text_width) / 2;
    draw_text_to_canvas_2x(canvas, text, x, y, color_index, font);
}
