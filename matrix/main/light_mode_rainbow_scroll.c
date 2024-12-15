#include "light_mode_rainbow_scroll.h"
#include "back_matrix_control.h"
#include "color_palette.h"
#include "freertos/semphr.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

// ----------------------------------
// 虹色スクロール
// ----------------------------------
static rgb_t const *pal = NULL;
static uint8_t back_canvas[BACK_TOTAL_HEIGHT][BACK_TOTAL_WIDTH];
static uint8_t front_canvas[FRONT_HEIGHT][FRONT_WIDTH];
static uint8_t tape_canvas[TAPE_LEDS];
static int scroll_offset = 0;

void reset_light_mode_rainbow_scroll() {
    color_palette_init();
    pal = color_palette_get();
    scroll_offset = 0;
}

void light_mode_rainbow_scroll() {
    // スクロールオフセットを増やして虹表示
    scroll_offset = (scroll_offset + 1) % PALETTE_SIZE;

    // Back
    for (int y = 0; y < BACK_TOTAL_HEIGHT; y++) {
        for (int x = 0; x < BACK_TOTAL_WIDTH; x++) {
            back_canvas[y][x] = (x + scroll_offset) % PALETTE_SIZE;
        }
    }
    back_matrix_draw_from_palette(pal, PALETTE_SIZE, back_canvas);
    flush_back_and_wait();

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
    flush_front_and_wait();

    vTaskDelay(10 / portTICK_PERIOD_MS);
}
