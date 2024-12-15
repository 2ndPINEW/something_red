#include "light_controller.h"
#include "back_matrix_control.h"
#include "color_palette.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

static light_mode_t current_mode = LIGHT_MODE_INIT;
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

static void flush_back_and_wait() {
    flush_back();
    vTaskDelay(30 / portTICK_PERIOD_MS);
}

static void flush_front_and_wait() {
    flush_front();
    vTaskDelay(15 / portTICK_PERIOD_MS);
}

// 虹色横スクロール
static void light_mode_rainbow_scroll() {
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

static void light_mode_off() {
    // 全部消灯
    rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
    back_matrix_fill_color(off);
    flush_back_and_wait();
    vTaskDelay(10 / portTICK_PERIOD_MS);

    front_matrix_fill_color(off);
    tape_fill(TAPE1, off);
    tape_fill(TAPE2, off);
    tape_fill(TAPE3, off);
    tape_fill(TAPE4, off);
    flush_front_and_wait();
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void light_mode_blink_red() {
    // blink_stateでon/off切替
    blink_state = !blink_state;

    if (blink_state) {
        // 全部赤
        rgb_t red = (rgb_t){.r = 255, .g = 0, .b = 0};
        back_matrix_fill_color(red);
        flush_back_and_wait();

        front_matrix_fill_color(red);
        tape_fill(TAPE1, red);
        tape_fill(TAPE2, red);
        tape_fill(TAPE3, red);
        tape_fill(TAPE4, red);
        flush_front_and_wait();
    } else {
        // 全部黒
        rgb_t off = (rgb_t){.r = 0, .g = 0, .b = 0};
        back_matrix_fill_color(off);
        flush_back_and_wait();
        vTaskDelay(30 / portTICK_PERIOD_MS);

        front_matrix_fill_color(off);
        tape_fill(TAPE1, off);
        tape_fill(TAPE2, off);
        tape_fill(TAPE3, off);
        tape_fill(TAPE4, off);
        flush_front_and_wait();
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}

// 初期モード: TAPE4の最後の一個だけ点滅させる
static void light_mode_init() {
    static const rgb_t init_pal[2] = {{.r = 0, .g = 0, .b = 0},
                                      {.r = 255, .g = 255, .b = 255}};

    static uint8_t init_tape_canvas[TAPE_LEDS];

    for (int i = 0; i < TAPE_LEDS; i++) {
        init_tape_canvas[i] = 0;
    }
    init_tape_canvas[TAPE_LEDS - 1] = 1;

    tape_draw_from_palette(init_pal, 2, init_tape_canvas, TAPE4);
    flush_front_and_wait();
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int i = 0; i < TAPE_LEDS; i++) {
        init_tape_canvas[i] = 0;
    }
    tape_draw_from_palette(init_pal, 2, init_tape_canvas, TAPE4);
    flush_front_and_wait();
    vTaskDelay(pdMS_TO_TICKS(500));
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
        light_mode_off();
        break;
    case LIGHT_MODE_BLINK_RED:
        light_mode_blink_red();
        break;
    case LIGHT_MODE_INIT:
        light_mode_init();
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
    xTaskCreate(light_task, "light_task", 4096, NULL, 3, NULL);
}
