#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "light_controller.h"

static const char *TAG = "MAIN";

void app_main(void) {
    // 初期化のみ行う
    light_controller_init();
    
    ESP_LOGI(TAG, "System initialized and ready for animation data");
    
    // メインループ（必要に応じてシリアル通信の処理などを追加）
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
