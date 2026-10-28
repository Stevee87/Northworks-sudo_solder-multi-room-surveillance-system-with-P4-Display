#pragma once
#include <lvgl.h>
#include "floorplan_geometry.h"
namespace floorplan {
struct Viewport {
  int x,y,width,height;
  lv_point_t point(int logical_x,int logical_y) const;
  lv_point_t room_point(unsigned room_index,float u,float v) const;
};
Viewport viewport(int display_width,int display_height);
using SaveCallback=bool (*)(const FloorplanConfig &);
const Geometry &geometry();
FloorplanConfig config();
uint32_t revision();
bool is_editing();
void set_sensor_editor_active(bool active);
bool save_sensors(const Sensor (&sensors)[3]);
lv_obj_t *create(lv_obj_t *parent,int display_width,int display_height,
                FloorplanConfig initial=defaults,SaveCallback save=nullptr);
}
