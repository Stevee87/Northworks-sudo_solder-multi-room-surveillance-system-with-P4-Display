#include "floor_radar.h"
#include "floorplan.h"
#include "radar_map.h"
#include "system_state.h"
#include "radar_palette.h"
#include "radar_visuals.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace floor_radar {
radar::RoomSize room_size(unsigned node) {
  if(node>=3)return {};
  const auto r=floorplan::config().rooms[node];return {r.widthMm,r.heightMm};
}
namespace {
lv_obj_t *layers[3]{};
lv_obj_t *footer;
lv_obj_t *editButton,*actions[3],*selectors[3],*angleSlider,*angleLabel,*coordinatesLabel;
floorplan::FloorplanConfig draft;
uint32_t layoutRevision=0;
bool editing=false,dragging=false,failed=false;
unsigned selected=0;
float dragX=0,dragY=0;
char footerText[180]{};
floorplan::Viewport view;
radar::TargetFrame displayed{};
uint32_t last=0;
constexpr auto &colors=radar_palette::node;
radar_visuals::Geometry graphics[3];
radar_visuals::Regions previousRegions[3];
bool geometryDirty[3]={true,true,true},lastOnline[3]{},previousAlarmLit[3]{};
uint32_t previousRevision[3]{};
bool connected(unsigned i) { return system_state::demo_mode() || std::strcmp(system_state::status(i),"OFFLINE")!=0; }
radar_visuals::Geometry room_graphics(unsigned i) {
  const auto &layout=floorplan::geometry();const auto &r=draft.rooms[i];const auto &sensor=draft.sensors[i];
  const auto a=floorplan::to_plan(layout.scale,r.xMm,r.yMm);
  const auto b=floorplan::to_plan(layout.scale,r.xMm+r.widthMm,r.yMm+r.heightMm);
  const float x=view.x+a.x*view.width/1280,y=view.y+a.y*view.height/720;
  const float w=(b.x-a.x)*view.width/1280,h=(b.y-a.y)*view.height/720;
  const radar::MapScale map={x,y,w,h,r.widthMm};
  const radar::RadarPlacement p={uint8_t(i+1),sensor.xMm/r.widthMm,sensor.yMm/r.heightMm,sensor.angleDeg};
  auto result=radar_visuals::build(p,map);
  const auto pixelA=view.point(layout.rooms[i].interior.x1,layout.rooms[i].interior.y1);
  const auto pixelB=view.point(layout.rooms[i].interior.x2,layout.rooms[i].interior.y2);
  result.clip={lv_coord_t(pixelA.x+1),lv_coord_t(pixelA.y+1),lv_coord_t(pixelB.x-2),lv_coord_t(pixelB.y-2)};
  result.rectangular=true;result.roomSize={r.widthMm,r.heightMm};result.roomBounds={x,y,x+w,y+h};
  return result;
}
void hidden(lv_obj_t *obj,bool hide) {
  if(hide==lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN)) return;
  if(hide) lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN); else lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);
}
void angle_caption() {
  const auto &sensor=draft.sensors[selected];
  char text[180];std::snprintf(text,sizeof(text),"NODE %02u  X %.2f m  Y %.2f m  /  ROOM %.2f x %.2f m",selected+1,sensor.xMm/1000,sensor.yMm/1000,draft.rooms[selected].widthMm/1000,draft.rooms[selected].heightMm/1000);lv_label_set_text(coordinatesLabel,text);
  if(failed) lv_label_set_text(angleLabel,"SAVE FAILED");
  else lv_label_set_text_fmt(angleLabel,"NODE %02u / %03d DEG",selected+1,int(draft.sensors[selected].angleDeg));
  lv_slider_set_value(angleSlider,int(draft.sensors[selected].angleDeg),LV_ANIM_OFF);
  for(unsigned i=0;i<3;++i) lv_obj_set_style_border_width(selectors[i],selected==i?2:1,0);
}
void mode(bool next) {
  editing=next; dragging=false; failed=false;
  floorplan::set_sensor_editor_active(editing);
  const auto left=view.point(24,680),right=view.point(editing?1256:780,719);
  lv_obj_set_width(footer,right.x-left.x); lv_label_set_text(footer,""); footerText[0]=0;
  for(auto *obj:actions) hidden(obj,!editing);
  for(unsigned i=0;i<3;++i) hidden(selectors[i],!editing||!system_state::enabled(i));
  hidden(angleSlider,!editing); hidden(angleLabel,!editing); hidden(coordinatesLabel,!editing); hidden(editButton,editing||floorplan::is_editing());
  for(auto *layer:layers) {
    if(editing) lv_obj_add_flag(layer,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_PRESS_LOCK);
    else lv_obj_clear_flag(layer,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_PRESS_LOCK);
  }
  angle_caption(); refresh();
}
void action(lv_event_t *e) {
  const auto id=uintptr_t(lv_event_get_user_data(e));
  if(id==0 && !editing && !floorplan::is_editing()) { draft=floorplan::config(); selected=0; mode(true); }
  else if(editing && id==1) {
    if(floorplan::save_sensors(draft.sensors)) { draft=floorplan::config(); mode(false); }
    else { failed=true; angle_caption(); }
  } else if(editing && id==2) { draft=floorplan::config(); mode(false); }
  else if(editing && id==3) {
    for(unsigned i=0;i<3;++i) draft.sensors[i]={draft.rooms[i].widthMm*.04f,draft.rooms[i].heightMm*.06f,std::atan2(draft.rooms[i].widthMm,-draft.rooms[i].heightMm)*180/3.14159265f};
    angle_caption(); refresh();
  } else if(editing && id>=4 && id<=6 && system_state::enabled(unsigned(id-4))) { selected=unsigned(id-4); angle_caption(); refresh(); }
}
void drag(lv_event_t *e) {
  if(!editing || floorplan::is_editing()) return;
  const auto code=lv_event_get_code(e);
  if(code==LV_EVENT_RELEASED || code==LV_EVENT_PRESS_LOST) { dragging=false; return; }
  if(code!=LV_EVENT_PRESSED && code!=LV_EVENT_PRESSING) return;
  const unsigned i=unsigned(uintptr_t(lv_event_get_user_data(e)));
  if(!system_state::enabled(i)) return;
  auto *input=lv_indev_get_act(); if(!input) return;
  lv_point_t pointer; lv_indev_get_point(input,&pointer);
  const auto origin=project(i,0,0);
  if(code==LV_EVENT_PRESSED) {
    const float dx=pointer.x-origin.x,dy=pointer.y-origin.y;
    if(dx*dx+dy*dy>float(48*48)) return;
    selected=i; dragging=true; dragX=dx; dragY=dy; angle_caption();
  }
  if(!dragging || selected!=i) return;
  const float logicalX=(pointer.x-dragX-view.x)*1280.0f/view.width;
  const float logicalY=(pointer.y-dragY-view.y)*720.0f/view.height;
  const auto world=floorplan::from_plan(floorplan::geometry().scale,logicalX,logicalY);
  const auto &room=draft.rooms[i];auto &sensor=draft.sensors[i];
  sensor.xMm=std::max(0.0f,std::min(room.widthMm,std::round((world.x-room.xMm)/10)*10));
  sensor.yMm=std::max(0.0f,std::min(room.heightMm,std::round((world.y-room.yMm)/10)*10));
  angle_caption();
  refresh();
}
void position(lv_obj_t *obj,int x1,int y1,int x2,int y2) {
  const auto a=view.point(x1,y1),b=view.point(x2,y2);
  lv_obj_set_pos(obj,a.x,a.y); lv_obj_set_size(obj,b.x-a.x,std::max(44,int(b.y-a.y)));
}
lv_obj_t *button(lv_obj_t *parent,const char *text,unsigned id,int x1,int y1,int x2,int y2) {
  auto *obj=lv_btn_create(parent); lv_obj_remove_style_all(obj); position(obj,x1,y1,x2,y2);
  lv_obj_set_style_bg_color(obj,lv_color_hex(radar_palette::panel),0); lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
  lv_obj_set_style_text_color(obj,lv_color_hex(radar_palette::text),0); lv_obj_set_style_text_font(obj,&lv_font_montserrat_14,0);
  lv_obj_set_style_border_width(obj,1,0); lv_obj_set_style_border_color(obj,lv_color_hex(radar_palette::wall),0); lv_obj_set_style_radius(obj,4,0);
  lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
  auto *label=lv_label_create(obj); lv_label_set_text(label,text); lv_obj_center(label);
  lv_obj_add_event_cb(obj,action,LV_EVENT_CLICKED,reinterpret_cast<void*>(uintptr_t(id))); return obj;
}
void draw(lv_event_t *event) {
  const unsigned i=unsigned(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
  if(!system_state::enabled(i)) return;
  auto *ctx=lv_event_get_draw_ctx(event);
  // Intersect the current invalidation region with this room; always restore
  // the shared context before LVGL draws a sibling, wall or toolbar.
  if(geometryDirty[i]) {
    graphics[i]=room_graphics(i); geometryDirty[i]=false;
  }
  const auto *previousClip=ctx->clip_area;
  lv_area_t roomClip;
  if(!_lv_area_intersect(&roomClip,previousClip,&graphics[i].clip)) return;
  ctx->clip_area=&roomClip;
  radar_visuals::sector(ctx,graphics[i],i,connected(i));
  radar_visuals::room_alarm(ctx,graphics[i],i,connected(i),!editing,lv_tick_get());
  // Accurate edge/corner targets take priority over the border and warning icon.
  radar_visuals::dynamic(ctx,graphics[i],i,editing&&selected==i,connected(i),!editing,lv_tick_get());
  ctx->clip_area=previousClip;
}
}
void create(lv_obj_t *root,int width,int height) {
  view=floorplan::viewport(width,height);
  draft=floorplan::config();layoutRevision=floorplan::revision();
  footer=lv_label_create(root); const auto fa=view.point(24,680),fb=view.point(780,719);
  lv_obj_set_pos(footer,fa.x,fa.y); lv_obj_set_size(footer,fb.x-fa.x,fb.y-fa.y);
  lv_obj_set_style_bg_color(footer,lv_color_hex(radar_palette::background),0); lv_obj_set_style_bg_opa(footer,LV_OPA_COVER,0);
  lv_obj_set_style_text_color(footer,lv_color_hex(radar_palette::text),0); lv_obj_set_style_text_font(footer,&lv_font_montserrat_14,0);
  lv_label_set_text(footer,"");
  lv_label_set_recolor(footer,true);
  for(unsigned i=0;i<3;++i) {
    layers[i]=lv_obj_create(root); lv_obj_remove_style_all(layers[i]);
    lv_obj_add_flag(layers[i],LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(layers[i],LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(layers[i],draw,LV_EVENT_DRAW_MAIN,reinterpret_cast<void*>(uintptr_t(i)));
    lv_obj_add_event_cb(layers[i],drag,LV_EVENT_ALL,reinterpret_cast<void*>(uintptr_t(i)));
  }
  editButton=button(root,"RADAR EDIT",0,800,674,980,718);
  actions[0]=button(root,"SAVE",1,824,8,936,52); actions[1]=button(root,"CANCEL",2,948,8,1072,52); actions[2]=button(root,"RESET",3,1084,8,1200,52);
  for(unsigned i=0;i<3;++i) selectors[i]=button(root,i==0?"NODE 01":i==1?"NODE 02":"NODE 03",4+i,80+i*112,674,184+i*112,718);
  for(unsigned i=0;i<3;++i) { lv_obj_set_style_border_color(selectors[i],lv_color_hex(colors[i]),0); lv_obj_set_style_text_color(selectors[i],lv_color_hex(colors[i]),0); }
  coordinatesLabel=lv_label_create(root);position(coordinatesLabel,30,94,1230,122);lv_obj_set_height(coordinatesLabel,22);
  lv_obj_set_style_bg_color(coordinatesLabel,lv_color_hex(radar_palette::background),0);lv_obj_set_style_bg_opa(coordinatesLabel,LV_OPA_COVER,0);
  lv_obj_set_style_text_font(coordinatesLabel,&lv_font_montserrat_14,0);lv_obj_set_style_text_color(coordinatesLabel,lv_color_hex(radar_palette::text),0);
  angleLabel=lv_label_create(root); const auto p=view.point(450,683); lv_obj_set_pos(angleLabel,p.x,p.y);
  lv_obj_set_width(angleLabel,view.point(656,683).x-p.x); lv_label_set_long_mode(angleLabel,LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(angleLabel,lv_color_hex(radar_palette::text),0); lv_obj_set_style_text_font(angleLabel,&lv_font_montserrat_14,0);
  angleSlider=lv_slider_create(root); position(angleSlider,660,688,1160,704); lv_obj_set_height(angleSlider,16);
  lv_slider_set_range(angleSlider,0,360); lv_obj_set_ext_click_area(angleSlider,18);
  lv_obj_set_style_bg_color(angleSlider,lv_color_hex(radar_palette::panel),0); lv_obj_set_style_bg_opa(angleSlider,LV_OPA_COVER,0);
  lv_obj_set_style_bg_color(angleSlider,lv_color_hex(radar_palette::wall),LV_PART_INDICATOR); lv_obj_set_style_bg_opa(angleSlider,LV_OPA_COVER,LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(angleSlider,lv_color_hex(radar_palette::text),LV_PART_KNOB); lv_obj_set_style_bg_opa(angleSlider,LV_OPA_COVER,LV_PART_KNOB); lv_obj_set_style_pad_all(angleSlider,8,LV_PART_KNOB);
  lv_obj_add_event_cb(angleSlider,[](lv_event_t*) { if(editing) { draft.sensors[selected].angleDeg=lv_slider_get_value(angleSlider)%360; angle_caption(); refresh(); } },LV_EVENT_VALUE_CHANGED,nullptr);
  mode(false);
}
void refresh() { for(unsigned i=0;i<3;++i) { geometryDirty[i]=true; if(layers[i]) lv_obj_invalidate(layers[i]); } }
void poll(bool visible) {
  const auto now=lv_tick_get(); if(now-last<33) return; last=now;
  const auto next=system_state::frame(now);
  if(!editing&&layoutRevision!=floorplan::revision()) {
    layoutRevision=floorplan::revision();draft=floorplan::config();refresh();
  }
  const bool hideFooter=floorplan::is_editing();
  hidden(editButton,floorplan::is_editing()||editing);
  if(hideFooter!=lv_obj_has_flag(footer,LV_OBJ_FLAG_HIDDEN)) {
    if(hideFooter) lv_obj_add_flag(footer,LV_OBJ_FLAG_HIDDEN); else lv_obj_clear_flag(footer,LV_OBJ_FLAG_HIDDEN);
  }
  char text[180]; int length=std::snprintf(text,sizeof(text),"%s  %u/%u ONLINE",system_state::demo_mode()?"DEMO":"LIVE",system_state::online_count(),unsigned(system_state::settings().activeNodes));
  for(unsigned i=0;i<system_state::settings().activeNodes;++i) length+=std::snprintf(text+length,sizeof(text)-length,"    #%06lX N%u %s#",(unsigned long)(connected(i)?colors[i]:radar_palette::muted),i+1,system_state::status(i));
  if(!editing && std::strcmp(text,footerText)) { std::strcpy(footerText,text); lv_label_set_text(footer,text); }
  for(unsigned i=0;i<3;++i) {
    if(!visible || floorplan::is_editing() || !system_state::enabled(i)) {
      if(!lv_obj_has_flag(layers[i],LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(layers[i],LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    const auto r=floorplan::geometry().rooms[i].interior;
    const auto a=view.point(r.x1,r.y1),b=view.point(r.x2,r.y2);
    if(lv_obj_get_x(layers[i])!=a.x || lv_obj_get_y(layers[i])!=a.y) { lv_obj_set_pos(layers[i],a.x,a.y); geometryDirty[i]=true; }
    if(lv_obj_get_width(layers[i])!=b.x-a.x || lv_obj_get_height(layers[i])!=b.y-a.y) { lv_obj_set_size(layers[i],b.x-a.x,b.y-a.y); geometryDirty[i]=true; }
    if(lastOnline[i]!=connected(i)) { lastOnline[i]=connected(i); geometryDirty[i]=true; }
    if(geometryDirty[i]) lv_obj_invalidate(layers[i]);
    const bool alarm=radar_visuals::room_alarm_lit(geometryDirty[i]?room_graphics(i):graphics[i],i,connected(i),!editing,now);
    if(alarm!=previousAlarmLit[i]) {
      radar_visuals::invalidate_room_alarm(layers[i],geometryDirty[i]?room_graphics(i):graphics[i]);
      previousAlarmLit[i]=alarm;
    }
    const auto &fx=radar_feedback::shared().node(i);
    if(!editing && (connected(i) || fx.animated(now) || previousRevision[i]!=fx.revision)) {
      radar_visuals::invalidate(layers[i],previousRegions[i]);
      const auto regions=radar_visuals::regions(graphics[i],i,connected(i),true,now);
      radar_visuals::invalidate(layers[i],regions); previousRegions[i]=regions; previousRevision[i]=fx.revision;
    }
    if(lv_obj_has_flag(layers[i],LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(layers[i],LV_OBJ_FLAG_HIDDEN);
  }
  displayed=next;
}
bool is_editing() { return editing; }
const floorplan::FloorplanConfig &config() { return draft; }
radar::Point project_target(unsigned node,float xMm,float yMm) {
  if(node>=3) return {};
  const auto p=radar_visuals::target_point(room_graphics(node),xMm,yMm);
  return {float(p.x),float(p.y)};
}
radar::Point project(unsigned node,float xMm,float yMm) {
  if(node>=3) return {};
  const auto g=room_graphics(node);
  return radar::local_to_room(g.placement,xMm,yMm,g.roomBounds,g.roomSize);
}
}
