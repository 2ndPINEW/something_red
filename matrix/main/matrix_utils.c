#include "matrix_utils.h"
#include "esp_log.h"
#include "esp_ws28xx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "esp_ws28xx.h"

int get_matrix_led_index(int x, int y, int width) {
    if (y % 2 == 0) {
        // 偶数行: 左→右
        return y * width + x;
    } else {
        // 奇数行: 右→左
        return (y + 1) * width - 1 - x;
    }
}

void matrix_update() { ESP_ERROR_CHECK_WITHOUT_ABORT(ws28xx_update()); }
