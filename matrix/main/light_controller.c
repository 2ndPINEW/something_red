#include "light_controller.h"
#include "back_matrix_control.h"
#include "color_palette.h"
#include "freertos/semphr.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "light_mode_rainbow_scroll.h"
#include "light_mode_off.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

static SemaphoreHandle_t mode_mutex = NULL;

static light_mode_t current_mode = LIGHT_MODE_INIT;

void light_controller_set_mode(light_mode_t mode) {
    if (mode_mutex) {
        xSemaphoreTake(mode_mutex, portMAX_DELAY);
        current_mode = mode;
        xSemaphoreGive(mode_mutex);
    }
}

// ----------------------------------
// 点滅
// ----------------------------------
static uint8_t blink_state = 0;
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

// ----------------------------------
// 初期モード
// ----------------------------------
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
    case LIGHT_MODE_INIT:
        light_mode_init();
        break;
    case LIGHT_MODE_OFF:
        light_mode_off();
        break;
    case LIGHT_MODE_RAINBOW_SCROLL: {
        light_mode_rainbow_scroll();
        break;
    }
    case LIGHT_MODE_BLINK_RED:
        light_mode_blink_red();
        break;
    case LIGHT_MODE_TEXT:
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

    reset_light_mode_rainbow_scroll();
    blink_state = 0;

    // タスク起動
    xTaskCreate(light_task, "light_task", 4096, NULL, 3, NULL);
}
