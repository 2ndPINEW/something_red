#include "light_controller.h"
#include "led_matrix_control.h"
#include "freertos/semphr.h"
#include "led_strip.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "esp_log.h"
#include <string.h>
#include <ctype.h>   // isspace用に追加
#include <inttypes.h>  // PRI* マクロ用に追加

static const char *TAG = "LIGHT_CONTROLLER";

// アニメーションデータの構造体
typedef struct {
    uint32_t duration_ms; // フレームの表示時間（ミリ秒）
    uint8_t brightness;   // フレームの明るさ（0-255）
    uint8_t pixels[LED_TOTAL_HEIGHT][LED_TOTAL_WIDTH]; // 各ピクセルのパレットインデックス
} frame_t;

// アニメーション全体の構造体
typedef struct {
    rgb_t palette[MAX_PALETTE_COLORS]; // カラーパレット
    int palette_size;                  // パレット内の色数
    frame_t frames[MAX_ANIMATION_FRAMES]; // フレーム配列
    int frame_count;                   // フレーム数
    int current_frame;                 // 現在再生中のフレーム
    uint32_t last_frame_time;          // 最後にフレームを更新した時間
    bool is_playing;                   // 再生中フラグ
} animation_t;

static animation_t animation = {0};
static SemaphoreHandle_t animation_mutex = NULL;

// 文字列をトリミングする関数
static char *trim(char *str) {
    char *end;
    // 先頭の空白をスキップ
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str; // 文字列が空白のみの場合
    // 末尾の空白を削除
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// HEX形式の文字列をrgb_t型に変換する関数
static rgb_t hex_to_rgb(const char *hex) {
    rgb_t color = {0};
    // #を含む場合はスキップ
    if (hex[0] == '#') hex++;
    
    unsigned int r, g, b;
    if (sscanf(hex, "%02x%02x%02x", &r, &g, &b) == 3) {
        color.r = r;
        color.g = g;
        color.b = b;
    } else {
        ESP_LOGE(TAG, "Invalid HEX color: %s", hex);
    }
    return color;
}

// パレット行を解析する関数
static bool parse_palette_line(const char *line) {
    if (strncmp(line, "palette:", 8) != 0) return false;
    
    char *palette_data = strdup(line + 8);
    if (!palette_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return false;
    }
    
    xSemaphoreTake(animation_mutex, portMAX_DELAY);
    animation.palette_size = 0;
    
    // パレットデータをカンマで分割
    char *token = strtok(palette_data, ",");
    while (token && animation.palette_size < MAX_PALETTE_COLORS) {
        char *color_str = trim(token);
        animation.palette[animation.palette_size++] = hex_to_rgb(color_str);
        token = strtok(NULL, ",");
    }
    
    xSemaphoreGive(animation_mutex);
    free(palette_data);
    ESP_LOGI(TAG, "Parsed palette with %d colors", animation.palette_size);
    return true;
}

// 明るさ行を解析する関数
static bool parse_brightness_line(const char *line) {
    if (strncmp(line, "brightness:", 11) != 0) return false;
    
    char *brightness_data = strdup(line + 11);
    if (!brightness_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return false;
    }
    
    xSemaphoreTake(animation_mutex, portMAX_DELAY);
    if (animation.frame_count > 0) {
        // 最後に追加されたフレームの明るさを設定
        int frame_index = animation.frame_count - 1;
        uint8_t brightness = (uint8_t)atoi(trim(brightness_data));
        animation.frames[frame_index].brightness = brightness;
        ESP_LOGI(TAG, "Set brightness %d for frame %d", brightness, frame_index + 1);
    }
    
    xSemaphoreGive(animation_mutex);
    free(brightness_data);
    return true;
}

// フレーム行を解析する関数
static bool parse_frame_line(const char *line) {
    if (strncmp(line, "flame:", 6) != 0) return false;
    
    char *frame_data = strdup(line + 6);
    if (!frame_data) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return false;
    }
    
    xSemaphoreTake(animation_mutex, portMAX_DELAY);
    if (animation.frame_count >= MAX_ANIMATION_FRAMES) {
        ESP_LOGW(TAG, "Max frame count reached: %d", MAX_ANIMATION_FRAMES);
        xSemaphoreGive(animation_mutex);
        free(frame_data);
        return false;
    }
    
    frame_t *frame = &animation.frames[animation.frame_count];
    memset(frame, 0, sizeof(frame_t));
    frame->brightness = 255; // デフォルトは最大輝度
    
    // フレームデータをカンマで分割
    char *token = strtok(frame_data, ",");
    
    // 最初のトークンはタイムスタンプ
    if (token) {
        frame->duration_ms = atoi(trim(token));
        token = strtok(NULL, ",");
        
        // ピクセルデータを順番に読み込み
        int pixel_index = 0;
        while (token && pixel_index < LED_TOTAL_WIDTH * LED_TOTAL_HEIGHT) {
            int y = pixel_index / LED_TOTAL_WIDTH;
            int x = pixel_index % LED_TOTAL_WIDTH;
            
            char *index_str = trim(token);
            uint8_t color_index = (uint8_t)strtol(index_str, NULL, 0);
            
            if (y < LED_TOTAL_HEIGHT && x < LED_TOTAL_WIDTH) {
                frame->pixels[y][x] = color_index;
            }
            
            pixel_index++;
            token = strtok(NULL, ",");
        }
        
        animation.frame_count++;
        ESP_LOGI(TAG, "Added frame %" PRId32 " with duration %" PRId32 " ms", 
                (int32_t)animation.frame_count, (int32_t)frame->duration_ms);
    }
    
    xSemaphoreGive(animation_mutex);
    free(frame_data);
    return true;
}

// アニメーションデータを受信する関数
void animation_receive_data(const char *data) {
    // 改行で分割して行ごとに処理
    char *data_copy = strdup(data);
    if (!data_copy) {
        ESP_LOGE(TAG, "Memory allocation failed");
        return;
    }
    
    xSemaphoreTake(animation_mutex, portMAX_DELAY);
    // アニメーションデータをリセット
    animation.frame_count = 0;
    animation.palette_size = 0;
    animation.current_frame = 0;
    animation.last_frame_time = 0;
    animation.is_playing = false;
    xSemaphoreGive(animation_mutex);
    
    char *line = strtok(data_copy, "\n");
    while (line) {
        char *trimmed_line = trim(line);
        
        if (strcmp(trimmed_line, "END") == 0) {
            ESP_LOGI(TAG, "End of animation data");
            
            xSemaphoreTake(animation_mutex, portMAX_DELAY);
            animation.is_playing = true;
            animation.current_frame = 0;
            animation.last_frame_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            xSemaphoreGive(animation_mutex);
            
            break;
        } else if (parse_palette_line(trimmed_line)) {
            // パレット行の処理は完了
        } else if (parse_frame_line(trimmed_line)) {
            // フレーム行の処理は完了
        } else if (parse_brightness_line(trimmed_line)) {
            // 明るさ行の処理は完了
        } else {
            ESP_LOGW(TAG, "Unknown data format: %s", trimmed_line);
        }
        
        line = strtok(NULL, "\n");
    }
    
    free(data_copy);
    ESP_LOGI(TAG, "Animation loaded: %d frames with %d colors", 
            animation.frame_count, animation.palette_size);
}

// 初期化パターンを表示する関数
static void show_init_pattern() {
    rgb_t off_color = (rgb_t){.r = 0, .g = 0, .b = 0};
    rgb_t on_color = (rgb_t){.r = 0, .g = 0, .b = 254};
    
    // 最初に全部消灯
    led_matrix_fill_color(off_color);
    flush_led_and_wait();
    
    // 点滅を5回繰り返す
    for (int i = 0; i < 5; i++) {
        // 全ての LED を白く点灯
        led_matrix_fill_color(on_color);
        flush_led_and_wait();
        
        // 少し待機
        vTaskDelay(300 / portTICK_PERIOD_MS);
        
        // 全ての LED を消灯
        led_matrix_fill_color(off_color);
        flush_led_and_wait();
        
        // 少し待機
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    
    // 最後に全部消灯（念のため）
    led_matrix_fill_color(off_color);
    flush_led_and_wait();
}

// アニメーション再生関数
static void play_animation() {
    xSemaphoreTake(animation_mutex, portMAX_DELAY);
    
    // アニメーションデータが無いか再生中でない場合は終了
    if (animation.frame_count == 0 || !animation.is_playing) {
        xSemaphoreGive(animation_mutex);
        return;
    }
    
    // 現在のフレームを取得
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t elapsed = current_time - animation.last_frame_time;
    
    // フレーム切り替え判定
    frame_t *current_frame = &animation.frames[animation.current_frame];
    if (elapsed >= current_frame->duration_ms) {
        // 次のフレームへ
        animation.current_frame = (animation.current_frame + 1) % animation.frame_count;
        animation.last_frame_time = current_time;
        current_frame = &animation.frames[animation.current_frame];
        ESP_LOGI(TAG, "Playing frame %" PRId32 "/%" PRId32 " (duration: %" PRId32 " ms)", 
                (int32_t)(animation.current_frame + 1), (int32_t)animation.frame_count, 
                (int32_t)current_frame->duration_ms);
    }
    
    // 現在のフレームを描画（明るさを考慮）
    led_matrix_draw_from_palette_with_brightness(animation.palette, animation.palette_size, 
                                  current_frame->pixels, current_frame->brightness);
    
    ESP_LOGI(TAG, "Frame %d brightness: %d", animation.current_frame + 1, current_frame->brightness);
    
    xSemaphoreGive(animation_mutex);
    
    // 表示を更新
    flush_led_and_wait();
}

// アニメーションタスク
static void light_task(void *arg) {
    // 初期化パターンを表示
    show_init_pattern();
    
    while (1) {
        xSemaphoreTake(animation_mutex, portMAX_DELAY);
        bool has_animation = (animation.frame_count > 0 && animation.is_playing);
        xSemaphoreGive(animation_mutex);
        
        if (has_animation) {
            // アニメーションがある場合は再生
            play_animation();
            vTaskDelay(10 / portTICK_PERIOD_MS);
        } else {
            // アニメーションがない場合は初期化パターンを表示
            show_init_pattern();
        }
    }
}

void light_controller_init() {
    // ミューテックス作成
    animation_mutex = xSemaphoreCreateMutex();
    
    // LED初期化
    led_strip_install();
    led_matrix_init();
    
    // アニメーション初期化
    xSemaphoreTake(animation_mutex, portMAX_DELAY);
    animation.frame_count = 0;
    animation.palette_size = 0;
    animation.current_frame = 0;
    animation.last_frame_time = 0;
    animation.is_playing = false;
    
    // デフォルトパレット設定（黒と白）
    animation.palette[0] = (rgb_t){.r = 0, .g = 0, .b = 0};   // 黒
    animation.palette[1] = (rgb_t){.r = 255, .g = 255, .b = 255}; // 白
    animation.palette_size = 2;
    xSemaphoreGive(animation_mutex);
    
    // タスク起動
    xTaskCreate(light_task, "light_task", 4096, NULL, 3, NULL);
}
