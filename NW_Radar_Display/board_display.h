#pragma once
#include "board_select.h"
#include <lvgl.h>

namespace board_display {
bool begin();
bool lock();
void unlock();
void poll();
void show();
}
