#include "light_mode_pachinko.h"
#include "front_matrix_control.h"
#include "led_strip.h"
#include "color_palette.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>

#define PHASE_TAPE_STREAM 0
#define PHASE_FINAL 1

static const rgb_t *pal = NULL;

static uint8_t front_canvas[FRONT_HEIGHT][FRONT_WIDTH];
static uint8_t tape_canvas[TAPE_LEDS];

static int phase = PHASE_TAPE_STREAM;
static int frame_counter = 0;

// テープ関連
static int tape_run_count = 0; 
static bool tape_on = false;  
static int tape_color_range = 256/6; // 42程度
static int tape_color_start = 0;

// フロント侵食関連
static int front_invasion_level = 0; // 0～6

// 最終フェーズ関連
static int final_blink_count = 0;  // 10回点滅
static bool final_blink_state = false; // ON/OFF
static bool final_done_blink = false; 

void reset_light_mode_pachinko() {
    color_palette_init();
    pal = color_palette_get();
    phase = PHASE_TAPE_STREAM;
    frame_counter=0;
    tape_run_count=0;
    tape_on=false;
    tape_color_start=0;
    front_invasion_level=0;
    final_blink_count=0;
    final_blink_state=false;
    final_done_blink=false;

    front_matrix_fill_color((rgb_t){.r=0,.g=0,.b=0});
    for (int i=0;i<TAPE_LEDS;i++) tape_canvas[i]=0;
    flush_front();
}

// テープ1回流す処理
static void run_tape_once() {
    if(!tape_on) {
        tape_on=true;
        frame_counter=0;
        tape_color_start = tape_run_count * tape_color_range;
    } else {
        int scroll_offset = frame_counter % tape_color_range; 
        for (int i=0;i<TAPE_LEDS;i++){
            int idx = (tape_color_start + (i+scroll_offset))%256;
            tape_canvas[i]=idx;
        }
        tape_draw_from_palette(pal, 256, tape_canvas,TAPE1);
        tape_draw_from_palette(pal, 256, tape_canvas,TAPE2);
        tape_draw_from_palette(pal, 256, tape_canvas,TAPE3);
        tape_draw_from_palette(pal, 256, tape_canvas,TAPE4);
        flush_front();
        frame_counter++;

        // 100フレームで1回分終了
        if (frame_counter>42) {
            // 消灯
            for (int i=0;i<TAPE_LEDS;i++) tape_canvas[i]=0;
            tape_draw_from_palette(pal,256,tape_canvas,TAPE1);
            tape_draw_from_palette(pal,256,tape_canvas,TAPE2);
            tape_draw_from_palette(pal,256,tape_canvas,TAPE3);
            tape_draw_from_palette(pal,256,tape_canvas,TAPE4);
            flush_front();

            tape_on=false;
            tape_run_count++;
            front_invasion_level++;

            // フロント侵食反映
            int diagonal = FRONT_WIDTH + FRONT_HEIGHT;
            int infiltration_dist = diagonal * front_invasion_level / 6;
            for(int y=0;y<FRONT_HEIGHT;y++){
                for(int x=0;x<FRONT_WIDTH;x++){
                    // 右下原点で計算: dist = (FRONT_WIDTH-1 - x)+(FRONT_HEIGHT-1 - y)
                    int dist = (FRONT_WIDTH-1 - x)+(FRONT_HEIGHT-1 - y);
                    if(dist < infiltration_dist) {
                        // 虹色適用: (x*3 + y*5 + front_invasion_level*10)で分散
                        int idx = (x*3 + y*5 + front_invasion_level*10) % 256;
                        front_canvas[y][x] = idx;
                    } else {
                        front_canvas[y][x] = PALETTE_SIZE + 1;
                    }
                }
            }
            front_matrix_draw_from_palette(pal,256,front_canvas);
            flush_front();
        }
    }
}

// フェーズ1: テープ6回流す
static void phase_tape_stream_func() {
    if(tape_run_count<6) {
        if(!tape_on) {
            run_tape_once();
        } else {
            run_tape_once();
        }
    } else {
        // 全部完了 → 最終フェーズ
        phase=PHASE_FINAL;
        frame_counter=0;
        final_blink_count=0;
        final_blink_state=false;
        final_done_blink=false;
    }
}

// フェーズ2: 最終フェーズ
// 10回点滅しながら虹色スクロール→点滅後はスクロールのみ
static void phase_final_func() {
    for(int y=0;y<FRONT_HEIGHT;y++){
        for(int x=0;x<FRONT_WIDTH;x++){
            int idx=(x+y+frame_counter)%256;
            if(!final_done_blink) {
                // 点滅中(20フレームで1回点滅)
                if(!final_blink_state) {
                    front_canvas[y][x]=PALETTE_SIZE + 1;
                } else {
                    front_canvas[y][x]=idx;
                }
            } else {
                // 点滅終了後は常にon(虹色スクロール)
                front_canvas[y][x]=idx;
            }
        }
    }

    front_matrix_draw_from_palette(pal,256,front_canvas);
    flush_front();

    frame_counter++;

    if(!final_done_blink) {
        if(frame_counter%5==0) {
            final_blink_state=!final_blink_state;
            if(!final_blink_state) {
                final_blink_count++;
                if(final_blink_count>=10) {
                    final_done_blink=true; 
                }
            }
        }
    }

}

void light_mode_pachinko() {
    switch(phase) {
        case PHASE_TAPE_STREAM:
            phase_tape_stream_func();
            break;
        case PHASE_FINAL:
            phase_final_func();
            break;
    }
}
