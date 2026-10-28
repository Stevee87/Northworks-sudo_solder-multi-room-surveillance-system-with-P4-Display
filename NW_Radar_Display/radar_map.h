#pragma once
#include <lvgl.h>
#include "radar_model.h"
namespace radar_map {
using SaveCallback=bool (*)(const radar::Config &);
lv_obj_t *create(lv_obj_t *parent,int width,int height,const radar::Config &,SaveCallback);
void set_visible(bool visible);
bool is_editing();
bool demo_enabled();
uint32_t demo_updates();
const radar::Config &config();
// Same renderer consumes decoded local-mm targets from any future input source.
// Call only from the LVGL task (or while holding its board-specific mutex).
// External frames are accepted only when the simulated DEMO source is off.
void set_targets(const radar::TargetFrame &);
void set_node_status(unsigned node,bool online,bool target);
void settings_changed();
void toggle_demo();
}
