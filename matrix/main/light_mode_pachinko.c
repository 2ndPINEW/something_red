#include "light_mode_pachinko.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "color_palette.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>

#define PALETTE_SIZE 128
#define PHASE_TAPE_STREAM 0
#define PHASE_INJECT 1
#define PHASE_STARS 2

static const rgb_t *pal = NULL;

static uint8_t front_canvas[FRONT_HEIGHT][FRONT_WIDTH];
static uint8_t tape_canvas[TAPE_LEDS];

static int phase = PHASE_TAPE_STREAM;
static int scroll_offset = 0;
static int frame_counter = 0;

// 星関連
#define MAX_STARS 20
static struct {
    int x,y;
    uint8_t c;
    int blink;
} stars[MAX_STARS];
static int star_count = 0;

void reset_light_mode_pachinko() {
    color_palette_init();
    pal = color_palette_get();
    phase = PHASE_TAPE_STREAM;
    scroll_offset = 0;
    frame_counter = 0;
    star_count = 0;

    front_matrix_fill_color((rgb_t){.r=0,.g=0,.b=0});
    for (int i=0; i<TAPE_LEDS; i++) tape_canvas[i]=0;
    flush_front();
}

static void add_star() {
    if (star_count<MAX_STARS) {
        stars[star_count].x = rand()%FRONT_WIDTH;
        stars[star_count].y = rand()%FRONT_HEIGHT;
        stars[star_count].c = rand()%PALETTE_SIZE;
        stars[star_count].blink = 10+rand()%20;
        star_count++;
    }
}

static void draw_stars() {
    // front_canvasクリア
    for (int y=0;y<FRONT_HEIGHT;y++) {
        for (int x=0;x<FRONT_WIDTH;x++) {
            front_canvas[y][x]=0;
        }
    }

    for (int i=0;i<star_count;i++) {
        if (stars[i].blink >0) {
            // 偶数フレームで表示
            if (stars[i].blink %2==0) {
                front_canvas[stars[i].y][stars[i].x]=stars[i].c;
            }
            stars[i].blink--;
        } else {
            stars[i]=stars[star_count-1];
            star_count--;
            i--;
        }
    }

    front_matrix_draw_from_palette(pal, PALETTE_SIZE, front_canvas);
    flush_front();
}

// PHASE_TAPE_STREAM：テープに虹がスクロール
static void phase_tape_stream() {
    scroll_offset=(scroll_offset+1)%PALETTE_SIZE;

    // テープに虹色を流す
    for (int i=0;i<TAPE_LEDS;i++) {
        tape_canvas[i]=(i+scroll_offset)%PALETTE_SIZE;
    }
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE1);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE2);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE3);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE4);
    flush_front();

    vTaskDelay(pdMS_TO_TICKS(100));
    frame_counter++;

    // 一定時間後にPHASE_INJECTへ
    if (frame_counter > 50) {
        phase=PHASE_INJECT;
        frame_counter=0;
    }
}

// PHASE_INJECT:
// テープはまだ虹スクロールしつつ、フロントに下から虹が徐々に侵食
static void phase_inject() {
    scroll_offset=(scroll_offset+1)%PALETTE_SIZE;

    // テープ継続
    for (int i=0;i<TAPE_LEDS;i++) {
        tape_canvas[i]=(i+scroll_offset)%PALETTE_SIZE;
    }
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE1);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE2);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE3);
    tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE4);

    // 侵食処理
    // frame_counterを使って下から1列ずつ侵食
    // infiltration_depth: 何行が完全侵食されたか(下から)
    // infiltration_width: 次の行で何列侵食されたか
    int infiltration_depth = frame_counter / FRONT_WIDTH; 
    int infiltration_width = frame_counter % FRONT_WIDTH;

    for (int y=0; y<FRONT_HEIGHT; y++){
        for (int x=0;x<FRONT_WIDTH;x++){
            front_canvas[y][x]=0;
        }
    }

    // 下からinfiltration_depth行は全て虹
    // y=0がトップ、y=FRONT_HEIGHT-1がボトムなので、ボトムから上へ
    // ボトム行はy=FRONT_HEIGHT-1
    for (int done_rows=0; done_rows<infiltration_depth && done_rows<FRONT_HEIGHT; done_rows++) {
        int y = FRONT_HEIGHT-1 - done_rows;
        for (int x=0;x<FRONT_WIDTH;x++){
            front_canvas[y][x]=(x+y+scroll_offset)%PALETTE_SIZE;
        }
    }

    // 次の行を部分的に侵食
    if (infiltration_depth < FRONT_HEIGHT) {
        int y_partial = FRONT_HEIGHT-1 - infiltration_depth;
        for (int x=0;x<infiltration_width; x++){
            front_canvas[y_partial][x]=(x+y_partial+scroll_offset)%PALETTE_SIZE;
        }
    }

    front_matrix_draw_from_palette(pal, PALETTE_SIZE, front_canvas);
    flush_front();

    vTaskDelay(pdMS_TO_TICKS(100));
    frame_counter++;

    // 全画面侵食完了(全FRONT_HEIGHT行が侵食)
    if (infiltration_depth >= FRONT_HEIGHT) {
        phase=PHASE_STARS;
        frame_counter=0;
        // テープオフ
        for (int i=0;i<TAPE_LEDS;i++) tape_canvas[i]=0;
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE1);
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE2);
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE3);
        tape_draw_from_palette(pal, PALETTE_SIZE, tape_canvas, TAPE4);
        flush_front();

        // 初期星
        for (int i=0;i<5;i++) add_star();
    }
}

// PHASE_STARS: テープは消え、フロントに星が点滅
static void phase_stars() {
    if (rand()%10==0) {
        add_star();
    }
    draw_stars();
    vTaskDelay(pdMS_TO_TICKS(200));
}

void light_mode_pachinko() {
    switch (phase) {
        case PHASE_TAPE_STREAM:
            phase_tape_stream();
            break;
        case PHASE_INJECT:
            phase_inject();
            break;
        case PHASE_STARS:
            phase_stars();
            break;
    }
}
