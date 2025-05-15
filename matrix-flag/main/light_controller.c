#include "light_controller.h"
#include "led_matrix_control.h"
#include "color_palette.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include "led_matrix_control.h"
#include "light_modes/light_mode_init.h"
#include "light_modes/light_mode_off.h"
#include "light_modes/light_mode_blue_flush.h"
#include "light_modes/light_mode_blue_fire.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

static SemaphoreHandle_t mode_mutex = NULL;

static light_mode_t current_mode = LIGHT_MODE_INIT;

void reset_all_light_mode() {
    reset_light_mode_init();
    reset_light_mode_off();
    reset_light_mode_blue_flush();
    reset_light_mode_blue_fire();
}

void light_controller_set_mode(light_mode_t mode) {
    if (mode_mutex) {
        reset_all_light_mode();
        xSemaphoreTake(mode_mutex, portMAX_DELAY);
        current_mode = mode;
        xSemaphoreGive(mode_mutex);
    }
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
    case LIGHT_MODE_BLUE_FLUSH:
        light_mode_blue_flush();
        break;
    case LIGHT_MODE_BLUE_FIRE:
        light_mode_blue_fire();
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
    led_matrix_init();

    reset_all_light_mode();

    // タスク起動
    xTaskCreate(light_task, "light_task", 4096, NULL, 3, NULL);
}
