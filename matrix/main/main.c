#include "back_matrix_control.h"
#include "color_palette.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "matrix_utils.h"
#include <stdio.h>

static const char *TAG = "MAIN";

void app_main(void) {
    led_strip_install();
    back_matrix_init();
    front_matrix_init();

    // パレットの取得
    color_palette_init();
    const rgb_t *pal = color_palette_get(); // PALETTE_SIZE色の虹パレット

    static uint8_t canvas[BACK_TOTAL_HEIGHT][BACK_TOTAL_WIDTH];
    static uint8_t front_canvas[FRONT_HEIGHT][FRONT_WIDTH];
    static uint8_t tape_canvas[TAPE_LEDS];

    // 初期化（初期状態を表示した後、ループでアニメーション）
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            canvas[y][x] = (x + y) % PALETTE_SIZE;
        }
    }
    back_matrix_draw_from_palette(pal, PALETTE_SIZE, canvas);

    for (int y = 0; y < FRONT_HEIGHT; y++) {
        for (int x = 0; x < FRONT_WIDTH; x++) {
            front_canvas[y][x] = (x + y) % PALETTE_SIZE;
        }
    }
    front_matrix_draw_from_palette(pal, PALETTE_SIZE, front_canvas);

    for (int i = 0; i < TAPE_LEDS; i++) {
        tape_canvas[i] = i % PALETTE_SIZE;
    }
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE1);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE2);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE3);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE4);
    flush_front();

    int scroll_offset = 0;
    while (1) {
        // スクロールオフセットを増加し、canvasを再構築して再描画
        scroll_offset = (scroll_offset + 1) % PALETTE_SIZE;

        // Backのキャンバス更新（x方向にスクロール）
        for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
            for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
                canvas[y][x] = (x + scroll_offset) % PALETTE_SIZE;
            }
        }
        back_matrix_draw_from_palette(pal, PALETTE_SIZE, canvas);

        // vTaskDelay(2 / portTICK_PERIOD_MS);

        // Frontのキャンバス更新
        for (int y = 0; y < FRONT_HEIGHT; y++) {
            for (int x = 0; x < FRONT_WIDTH; x++) {
                front_canvas[y][x] = (x + scroll_offset) % PALETTE_SIZE;
            }
        }
        front_matrix_draw_from_palette(pal, PALETTE_SIZE, front_canvas);

        // テープライト更新
        // テープも同様に (i + scroll_offset) % PALETTE_SIZE で動かす
        for (int i = 0; i < TAPE_LEDS; i++) {
            tape_canvas[i] = (i + scroll_offset) % PALETTE_SIZE;
        }
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE1);
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE2);
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE3);
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE4);
        flush_front();
    }
}
