#pragma once
#include "floorplan.h"
#include "radar_map.h"
namespace app_ui {
void create(lv_obj_t *,int width,int height,floorplan::FloorplanConfig,floorplan::SaveCallback,
            const radar::Config &,radar_map::SaveCallback);
void poll();
bool radar_visible();
}
