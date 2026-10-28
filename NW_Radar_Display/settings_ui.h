#pragma once
#include <lvgl.h>
namespace settings_ui {
void create(lv_obj_t *parent,int width,int height);
void open();
bool is_open();
bool mute_preview();
}
