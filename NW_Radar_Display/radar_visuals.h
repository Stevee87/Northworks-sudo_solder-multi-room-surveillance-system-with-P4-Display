#pragma once
#include <lvgl.h>
#include "radar_feedback.h"
namespace radar_visuals {
struct Geometry { radar::RadarPlacement placement{}; radar::MapScale map{}; radar::RoomSize roomSize{}; radar::Bounds roomBounds{}; lv_point_t origin{},arc[25]{}; lv_area_t clip{}; int radius=0; bool rectangular=false; };
struct Regions { lv_area_t target{},sensor{}; bool hasTarget=false,hasSensor=false; };
Geometry build(const radar::RadarPlacement &,const radar::MapScale &);
lv_point_t target_point(const Geometry &,float xMm,float yMm);
void sector(lv_draw_ctx_t *,const Geometry &,unsigned node,bool online);
void dynamic(lv_draw_ctx_t *,const Geometry &,unsigned node,bool selected,bool online,bool animate,uint32_t now);
Regions regions(const Geometry &,unsigned node,bool online,bool animate,uint32_t now);
void invalidate(lv_obj_t *,const Regions &);
// Room alarms use the same current target as the renderer, never decorative sweeps.
bool room_alarm_active(const Geometry &,unsigned node,bool online);
bool room_alarm_lit(const Geometry &,unsigned node,bool online,bool animate,uint32_t now);
void room_alarm(lv_draw_ctx_t *,const Geometry &,unsigned node,bool online,bool animate,uint32_t now);
void invalidate_room_alarm(lv_obj_t *,const Geometry &);
}
