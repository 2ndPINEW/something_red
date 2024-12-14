#include "back_matrix_control.h"
#include "front_matrix_control.h"
#include "matrix_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

void app_main(void) {
    back_matrix_init();
    front_matrix_init();
    matrix_update();
    while (1) {
        printf("Blinking back matrix...\n");
        back_matrix_blink();
        front_matrix_blink();
        matrix_update();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
