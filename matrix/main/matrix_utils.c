#include "matrix_utils.h"
#include <stdio.h>

int get_matrix_panel_led_index(int x, int y, int width) {
    if (y % 2 == 0) {
        // 偶数行: 右→左
        return (y + 1) * width - 1 - x;
    } else {
        // 奇数行: 左→右
        return y * width + x;
    }
}
