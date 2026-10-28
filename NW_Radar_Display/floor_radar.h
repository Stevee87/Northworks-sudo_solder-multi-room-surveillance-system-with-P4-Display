#pragma once
#include <lvgl.h>
#include "radar_model.h"
#include "floorplan_geometry.h"
namespace floor_radar {
void create(lv_obj_t *floorRoot,int width,int height);
void poll(bool visible);
void refresh();
bool is_editing();
const floorplan::FloorplanConfig &config();
// Current physical room size from the editable, persisted metric layout.
radar::RoomSize room_size(unsigned node);
// Exact metric position inside the room; outside detections get a visible edge marker.
radar::Point project_target(unsigned node,float xMm,float yMm);
radar::Point project(unsigned node,float xMm,float yMm);
}
