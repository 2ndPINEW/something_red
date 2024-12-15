#include "light_controller.h"
#include "back_matrix_control.h"
#include "front_matrix_control.h"
#include "color_palette.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

static light_mode_t current_mode = LIGHT_MODE_RAINBOW_SCROLL;
static rgb_t const *pal = NULL;

// バック・フロント・テープのキャンバスを保持（32x32, 16x16, テープ用など）
static uint8_t back_canvas[BACK_TOTAL_HEIGHT][BACK_TOTAL_WIDTH];
static uint8_t front_canvas[FRONT_HEIGHT][FRONT_WIDTH];
static uint8_t tape_canvas[TAPE_LEDS];

// スクロールオフセット
static int scroll_offset = 0;
// 点滅用フラグ
static uint8_t blink_state = 0;

#include "freertos/semphr.h"

static SemaphoreHandle_t mode_mutex = NULL;

void light_controller_set_mode(light_mode_t mode) {
    if (mode_mutex) {
        xSemaphoreTake(mode_mutex, portMAX_DELAY);
        current_mode = mode;
        xSemaphoreGive(mode_mutex);
    }
}

// 虹色横スクロール
static void light_mode_rainbow_scroll () {
    // スクロールオフセットを増やして虹表示
    scroll_offset = (scroll_offset + 1) % PALETTE_SIZE;

    // Back
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            back_canvas[y][x] = (x + scroll_offset) % PALETTE_SIZE;
        }
    }
    back_matrix_draw_from_palette(pal, PALETTE_SIZE, back_canvas);
    flush_back();

    vTaskDelay(10 / portTICK_PERIOD_MS);

    for (int y = 0; y < FRONT_HEIGHT; y++) {
        for (int x = 0; x < FRONT_WIDTH; x++) {
            front_canvas[y][x] = (x + scroll_offset) % PALETTE_SIZE;
        }
    }
    front_matrix_draw_from_palette(pal, PALETTE_SIZE, front_canvas);

    for (int i = 0; i < TAPE_LEDS; i++) {
        tape_canvas[i] = (i + scroll_offset) % PALETTE_SIZE;
    }
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE1);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE2);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE3);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE4);
    flush_front();

    vTaskDelay(10 / portTICK_PERIOD_MS);
}

// モードに応じて描画処理
static void update_display() {
    xSemaphoreTake(mode_mutex, portMAX_DELAY);
    light_mode_t mode = current_mode;
    xSemaphoreGive(mode_mutex);

    switch (mode) {
        case LIGHT_MODE_RAINBOW_SCROLL: {
            light_mode_rainbow_scroll();
            break;
        }
        case LIGHT_MODE_OFF:
            // 全部消灯
            //back_matrixとfront_matrix, tapeをクリア
            // led_strip_fill_all_black(); // 仮：全チャンネル消灯用(実装必要) 
            break;
        case LIGHT_MODE_BLINK:
            // blink_stateでon/off切替
            blink_state = !blink_state;

            if (blink_state) {
                // 全部赤
                rgb_t red = (rgb_t){.r = 255, .g = 0, .b = 0};
                // back_matrix_fill_color(red);
                // front_matrix_fill_color(red);
                tape_fill(TAPE1, red);
                tape_fill(TAPE2, red);
                tape_fill(TAPE3, red);
                tape_fill(TAPE4, red);
                flush_front();
            } else {
                // 全部黒
                rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
                // back_matrix_fill_color(off);
                // front_matrix_fill_color(off);
                tape_fill(TAPE1, off);
                tape_fill(TAPE2, off);
                tape_fill(TAPE3, off);
                tape_fill(TAPE4, off);
                flush_front();
            }
            break;
    }
}

// アニメーションタスク
static void light_task(void *arg) {
    while (1) {
        update_display();
    }
}

void light_controller_init() {
    mode_mutex = xSemaphoreCreateMutex();

    led_strip_install();
    back_matrix_init();
    front_matrix_init();

    color_palette_init();
    pal = color_palette_get();

    // 初期状態は虹スクロールなど
    scroll_offset = 0;
    blink_state = 0;

    // タスク起動
    xTaskCreate(light_task, "light_task", 4096, NULL, 5, NULL);
}
