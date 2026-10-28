#include "radar_map.h"
#include "floorplan.h"
#include "system_state.h"
#include "radar_palette.h"
#include "radar_visuals.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace radar_map {
namespace {
constexpr auto &colors=radar_palette::node;
constexpr uint32_t bg=radar_palette::background, interior=radar_palette::map, text=radar_palette::text, muted=radar_palette::muted;
constexpr int segments=24;
constexpr float radians=0.01745329252f;
floorplan::Viewport view;
radar::MapScale scale;
radar::Config saved,draft;
radar::TargetFrame frame{};
SaveCallback persist;
lv_obj_t *root,*map_layer,*target_layer,*hitbox,*buttons[4],*nodes[3],*slider,*angle_label,*demo_button;
lv_timer_t *timer;
bool visible=false, editing=false, failed=false, dragging=false;
bool &demo=system_state::demo_mode();
bool node_online[3]{},node_target[3]{};
unsigned selected=0;
float offset_x,offset_y;
uint32_t demo_start,last_history,edit_start;
uint32_t updates=0;
radar_visuals::Geometry graphics[3];
radar_visuals::Regions previousRegions[3];
uint32_t previousRevision[3]{};
bool connected(unsigned i) { return system_state::demo_mode() || node_online[i]; }
enum Action { Edit,Save,Cancel,Reset,Demo,Select0,Select1,Select2 };

lv_point_t pixel(radar::Point p) { return {lv_coord_t(std::lround(p.x)),lv_coord_t(std::lround(p.y))}; }
int size(int n) { return std::max(1,(n*view.width+640)/1280); }
lv_point_t origin(unsigned i) { return pixel(radar::local_to_map(draft.nodes[i],0,0,scale)); }
void rect(lv_draw_ctx_t *ctx,int x1,int y1,int x2,int y2,uint32_t color,lv_opa_t opa=LV_OPA_COVER) {
  lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
  d.bg_color=lv_color_hex(color); d.bg_opa=opa;
  lv_area_t area={lv_coord_t(x1),lv_coord_t(y1),lv_coord_t(x2),lv_coord_t(y2)};
  lv_draw_rect(ctx,&d,&area);
}
void line(lv_draw_ctx_t *ctx,lv_point_t a,lv_point_t b,uint32_t color,int width=1,lv_opa_t opa=LV_OPA_COVER) {
  lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d);
  d.color=lv_color_hex(color); d.width=size(width); d.opa=opa;
  lv_draw_line(ctx,&d,&a,&b);
}
void logical_line(lv_draw_ctx_t *ctx,int x1,int y1,int x2,int y2,uint32_t color,int width=1) {
  line(ctx,view.point(x1,y1),view.point(x2,y2),color,width);
}
void label(lv_draw_ctx_t *ctx,int x,int y,int width,const char *caption,uint32_t color,const lv_font_t *font) {
  auto p=view.point(x,y), q=view.point(x+width,y+35);
  lv_area_t a={p.x,p.y,q.x,q.y};
  lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d);
  d.color=lv_color_hex(color); d.font=font;
  lv_draw_label(ctx,&d,&a,caption,nullptr);
}
void circle(lv_draw_ctx_t *ctx,lv_point_t center,int radius,uint32_t color,bool filled,lv_opa_t opa=LV_OPA_COVER) {
  lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d);
  d.radius=LV_RADIUS_CIRCLE;
  d.bg_color=lv_color_hex(color); d.bg_opa=filled?opa:LV_OPA_TRANSP;
  d.border_color=lv_color_hex(color); d.border_width=filled?0:size(1); d.border_opa=opa;
  lv_area_t a={lv_coord_t(center.x-radius),lv_coord_t(center.y-radius),
               lv_coord_t(center.x+radius),lv_coord_t(center.y+radius)};
  lv_draw_rect(ctx,&d,&a);
}
void position(lv_obj_t *obj,int x1,int y1,int x2,int y2) {
  auto a=view.point(x1,y1),b=view.point(x2,y2);
  lv_obj_set_pos(obj,a.x,a.y); lv_obj_set_size(obj,b.x-a.x,b.y-a.y);
}
void clear_trails() {
  for(unsigned i=0;i<3;++i) {
    if(target_layer) radar_visuals::invalidate(target_layer,previousRegions[i]);
    previousRegions[i]={}; previousRevision[i]=UINT32_MAX;
  }
}
void rebuild() {
  scale.widthMm=draft.mapWidthMm;
  scale.visualScale=system_state::settings().scalePercent/100.0f;
  for (unsigned i=0;i<3;++i) {
    graphics[i]=radar_visuals::build(draft.nodes[i],scale);
  }
  lv_obj_invalidate(map_layer);
}
void draw_frame(lv_event_t *e) {
  auto *ctx=lv_event_get_draw_ctx(e);
  const auto a=view.point(24,84),b=view.point(1256,656);
  rect(ctx,a.x,a.y,b.x,b.y,radar_palette::panel);
  for (int pass=0;pass<2;++pass) {
    const auto color=pass==0?radar_palette::panel:radar_palette::wall;
    const int width=pass==0?3:1;
    for (int inset: {0,8}) {
      logical_line(ctx,24+inset,84+inset,1256-inset,84+inset,color,width);
      logical_line(ctx,24+inset,656-inset,1256-inset,656-inset,color,width);
      logical_line(ctx,24+inset,84+inset,24+inset,656-inset,color,width);
      logical_line(ctx,1256-inset,84+inset,1256-inset,656-inset,color,width);
    }
  }
  label(ctx,24,14,586,"TACTICAL SECURITY SYSTEM",text,view.width<1000?&lv_font_montserrat_18:&lv_font_montserrat_24);
  label(ctx,24,46,586,failed?"SAVE FAILED - RETRY OR CANCEL":"MULTI-ZONE RADAR MONITORING",failed?radar_palette::node[2]:muted,&lv_font_montserrat_14);
  logical_line(ctx,24,80,1256,80,radar_palette::grid);
  logical_line(ctx,24,668,1256,668,radar_palette::grid);
  if (!editing) {
    for (unsigned i=0;i<3;++i) {
      if(!system_state::enabled(i)) continue;
      const char *status=system_state::status(i);
      char caption[48]; std::snprintf(caption,sizeof(caption),"N%u %s",i+1,status);
      label(ctx,300+i*220,683,210,caption,connected(i)?colors[i]:muted,&lv_font_montserrat_14);
    }
    char summary[40]; std::snprintf(summary,sizeof(summary),"%s  %u/%u ONLINE",demo?"DEMO":"LIVE",system_state::online_count(),unsigned(system_state::settings().activeNodes));
    label(ctx,24,683,266,summary,text,&lv_font_montserrat_14);
  }
}
void draw_map(lv_event_t *e) {
  auto *ctx=lv_event_get_draw_ctx(e);
  for (int x=64;x<1240;x+=40) logical_line(ctx,x,92,x,648,radar_palette::grid);
  for (int y=112;y<648;y+=40) logical_line(ctx,40,y,1240,y,radar_palette::grid);
  for (unsigned i=0;i<3;++i) {
    if(!system_state::enabled(i)) continue;
    radar_visuals::sector(ctx,graphics[i],i,connected(i));
  }
}
void dirty_targets() {
  const auto now=lv_tick_get();
  for(unsigned i=0;i<3;++i) {
    const auto &fx=radar_feedback::shared().node(i);
    const bool animate=!editing && connected(i) && system_state::enabled(i);
    if(!animate && !fx.animated(now) && previousRevision[i]==fx.revision) continue;
    radar_visuals::invalidate(target_layer,previousRegions[i]);
    const auto next=system_state::enabled(i)?radar_visuals::regions(graphics[i],i,connected(i),!editing,now):radar_visuals::Regions{};
    radar_visuals::invalidate(target_layer,next); previousRegions[i]=next; previousRevision[i]=fx.revision;
  }
}
void draw_targets(lv_event_t *e) {
  auto *ctx=lv_event_get_draw_ctx(e);
  const auto now=lv_tick_get();
  for(unsigned i=0;i<3;++i) if(system_state::enabled(i)) radar_visuals::dynamic(ctx,graphics[i],i,editing&&selected==i,connected(i),!editing,now);
}
void update_frame(const radar::TargetFrame &next,bool history) {
  (void)history; frame=next;
  dirty_targets();
}
void tick(lv_timer_t *) {
  if(visible && !editing) { dirty_targets(); if(demo) ++updates; }
}
void update_angle() {
  char caption[40]; std::snprintf(caption,sizeof(caption),"NODE %02u / %03d DEG",selected+1,int(draft.nodes[selected].angleDeg));
  lv_label_set_text(angle_label,caption);
  lv_slider_set_value(slider,int(draft.nodes[selected].angleDeg),LV_ANIM_OFF);
  for (unsigned i=0;i<3;++i) {
    lv_obj_set_style_border_width(nodes[i],i==selected?2:1,0);
    if(!system_state::enabled(i)) lv_obj_add_flag(nodes[i],LV_OBJ_FLAG_HIDDEN);
  }
}
void set_mode(bool edit) {
  if (edit && !editing) edit_start=lv_tick_get();
  if (!edit && editing && demo) demo_start+=lv_tick_get()-edit_start;
  editing=edit; dragging=false; failed=false;
  for (int i=0;i<4;++i) {
    if ((i==0)!=edit) lv_obj_clear_flag(buttons[i],LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(buttons[i],LV_OBJ_FLAG_HIDDEN);
  }
  for (auto *obj: {hitbox,nodes[0],nodes[1],nodes[2],slider,angle_label}) {
    if (edit) lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
  }
  if (edit) { lv_obj_add_flag(demo_button,LV_OBJ_FLAG_HIDDEN); lv_timer_pause(timer); clear_trails(); }
  else {
    lv_obj_clear_flag(demo_button,LV_OBJ_FLAG_HIDDEN);
    if (visible) lv_timer_resume(timer);
  }
  update_angle();
  lv_obj_invalidate(root);
}
void act(lv_event_t *e) {
  auto action=static_cast<Action>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
  if (action==Edit && !editing) { draft=saved; set_mode(true); }
  else if (editing && action==Cancel) { draft=saved; rebuild(); set_mode(false); }
  else if (editing && action==Reset) { draft=radar::defaults(); rebuild(); update_angle(); }
  else if (editing && action==Save) {
    if (persist && persist(draft)) { saved=draft; set_mode(false); }
    else { failed=true; lv_obj_invalidate(root); }
  } else if (editing && action>=Select0 && action<=Select2) {
    selected=unsigned(action-Select0); update_angle(); lv_obj_invalidate(map_layer);
  } else if (action==Demo && !editing) {
    demo=!demo; clear_trails(); frame={};
    if(visible) lv_timer_resume(timer);
    lv_label_set_text(lv_obj_get_child(demo_button,0),demo?"DEMO":"LIVE");
    lv_obj_invalidate(root);
  }
}
void rotate(lv_event_t *) {
  if (!editing) return;
  draft.nodes[selected].angleDeg=lv_slider_get_value(slider);
  update_angle(); rebuild();
}
void drag(lv_event_t *e) {
  if (!editing) return;
  const auto code=lv_event_get_code(e);
  if (code==LV_EVENT_RELEASED || code==LV_EVENT_PRESS_LOST) { dragging=false; return; }
  if (code!=LV_EVENT_PRESSED && code!=LV_EVENT_PRESSING) return;
  auto *input=lv_indev_get_act(); if (!input) return;
  lv_point_t p; lv_indev_get_point(input,&p);
  if (code==LV_EVENT_PRESSED) {
    float best=float(size(42)*size(42)); int found=-1;
    // Footer node selectors make even completely coincident origins editable.
    auto distance=[&](unsigned i) { auto o=origin(i); return float((p.x-o.x)*(p.x-o.x)+(p.y-o.y)*(p.y-o.y)); };
    if (distance(selected)<=best) found=selected;
    else for (unsigned i=0;i<system_state::settings().activeNodes;++i) if (distance(i)<best) { best=distance(i); found=i; }
    if (found<0) return;
    selected=unsigned(found); dragging=true;
    auto o=origin(selected); offset_x=p.x-o.x; offset_y=p.y-o.y;
    update_angle(); lv_obj_invalidate(map_layer);
  }
  if (!dragging) return;
  auto &node=draft.nodes[selected];
  const float x=std::max(0.0f,std::min(1.0f,(p.x-offset_x-scale.x)/scale.width));
  const float y=std::max(0.0f,std::min(1.0f,(p.y-offset_y-scale.y)/scale.height));
  if (x!=node.normalizedX || y!=node.normalizedY) { node.normalizedX=x; node.normalizedY=y; rebuild(); }
}
lv_obj_t *button(const char *caption,Action action,int x1,int y1,int x2,int y2,uint32_t color=radar_palette::wall) {
  auto *obj=lv_btn_create(root); lv_obj_remove_style_all(obj);
  position(obj,x1,y1,x2,y2); lv_obj_set_height(obj,std::max(44,int(lv_obj_get_height(obj))));
  lv_obj_set_style_bg_color(obj,lv_color_hex(radar_palette::panel),0); lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
  lv_obj_set_style_bg_color(obj,lv_color_hex(radar_palette::panel),LV_STATE_PRESSED);
  lv_obj_set_style_border_color(obj,lv_color_hex(color),0); lv_obj_set_style_border_width(obj,1,0);
  lv_obj_set_style_radius(obj,4,0); lv_obj_set_style_text_color(obj,lv_color_hex(text),0);
  lv_obj_set_style_text_font(obj,&lv_font_montserrat_14,0); lv_obj_clear_flag(obj,LV_OBJ_FLAG_SCROLLABLE);
  auto *label=lv_label_create(obj); lv_label_set_text(label,caption); lv_obj_center(label);
  lv_obj_add_event_cb(obj,act,LV_EVENT_CLICKED,reinterpret_cast<void*>(intptr_t(action)));
  return obj;
}
}
lv_obj_t *create(lv_obj_t *parent,int w,int h,const radar::Config &initial,SaveCallback save) {
  view=floorplan::viewport(w,h); saved=draft=radar::valid_config(initial)?initial:radar::defaults(); persist=save;
  root=lv_obj_create(parent); lv_obj_remove_style_all(root); lv_obj_set_size(root,w,h);
  lv_obj_clear_flag(root,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(root,lv_color_hex(bg),0); lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);
  lv_obj_add_event_cb(root,draw_frame,LV_EVENT_DRAW_MAIN,nullptr);
  const auto bounds=floorplan::monitoring_interior;
  map_layer=lv_obj_create(root); lv_obj_remove_style_all(map_layer); position(map_layer,bounds.x1,bounds.y1,bounds.x2,bounds.y2);
  lv_obj_clear_flag(map_layer,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(map_layer,lv_color_hex(interior),0); lv_obj_set_style_bg_opa(map_layer,LV_OPA_COVER,0);
  lv_obj_add_event_cb(map_layer,draw_map,LV_EVENT_DRAW_MAIN,nullptr);
  auto a=view.point(bounds.x1,bounds.y1),b=view.point(bounds.x2,bounds.y2);
  scale={float(a.x),float(a.y),float(b.x-a.x-1),float(b.y-a.y-1),draft.mapWidthMm};
  target_layer=lv_obj_create(map_layer); lv_obj_remove_style_all(target_layer);
  lv_obj_set_size(target_layer,b.x-a.x,b.y-a.y); lv_obj_set_pos(target_layer,0,0);
  lv_obj_clear_flag(target_layer,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(target_layer,draw_targets,LV_EVENT_DRAW_MAIN,nullptr);
  hitbox=lv_obj_create(root); lv_obj_remove_style_all(hitbox); position(hitbox,bounds.x1,bounds.y1,bounds.x2,bounds.y2);
  lv_obj_clear_flag(hitbox,LV_OBJ_FLAG_SCROLLABLE); lv_obj_add_flag(hitbox,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_add_event_cb(hitbox,drag,LV_EVENT_ALL,nullptr);
  buttons[Edit]=button("EDIT",Edit,1076,8,1200,52);
  buttons[Save]=button("SAVE",Save,824,8,936,52);
  buttons[Cancel]=button("CANCEL",Cancel,948,8,1072,52);
  buttons[Reset]=button("RESET",Reset,1084,8,1200,52);
  for (unsigned i=0;i<3;++i) {
    char caption[12]; std::snprintf(caption,sizeof(caption),"NODE %02u",i+1);
    nodes[i]=button(caption,Action(Select0+i),80+112*i,674,180+112*i,718,colors[i]);
  }
  angle_label=lv_label_create(root); lv_obj_set_style_text_color(angle_label,lv_color_hex(text),0);
  lv_obj_set_style_text_font(angle_label,&lv_font_montserrat_14,0);
  auto pos=view.point(440,686); lv_obj_set_pos(angle_label,pos.x,pos.y);
  slider=lv_slider_create(root); lv_obj_remove_style_all(slider); position(slider,660,688,1160,704);
  lv_obj_set_style_bg_color(slider,lv_color_hex(radar_palette::panel),LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_MAIN); lv_obj_set_style_radius(slider,8,LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider,lv_color_hex(radar_palette::wall),LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider,lv_color_hex(radar_palette::text),LV_PART_KNOB);
  lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB); lv_obj_set_style_pad_all(slider,size(8),LV_PART_KNOB);
  lv_obj_set_style_radius(slider,LV_RADIUS_CIRCLE,LV_PART_KNOB);
  lv_obj_set_ext_click_area(slider,18); lv_slider_set_range(slider,0,359);
  lv_obj_add_event_cb(slider,rotate,LV_EVENT_VALUE_CHANGED,nullptr);
  demo_button=button("LIVE",Demo,1000,674,1200,718);
  demo_start=last_history=lv_tick_get(); frame={};
  timer=lv_timer_create(tick,33,nullptr); lv_timer_pause(timer);
  rebuild(); set_mode(false); set_visible(false);
  return root;
}
void set_visible(bool show) {
  visible=show; clear_trails();
  if (show) { lv_obj_clear_flag(root,LV_OBJ_FLAG_HIDDEN); if (!editing) lv_timer_resume(timer); }
  else { lv_obj_add_flag(root,LV_OBJ_FLAG_HIDDEN); lv_timer_pause(timer); }
}
bool is_editing() { return editing; }
bool demo_enabled() { return demo; }
uint32_t demo_updates() { return updates; }
const radar::Config &config() { return draft; }
void set_targets(const radar::TargetFrame &targets) {
  if(demo) return;
  bool changed=false;
  for(unsigned n=0;n<3;++n) for(unsigned t=0;t<3;++t) {
    const auto &a=frame.nodes[n].targets[t], &b=targets.nodes[n].targets[t];
    if(a.valid!=b.valid || (b.valid && (a.xMm!=b.xMm || a.yMm!=b.yMm))) changed=true;
  }
  if(changed) update_frame(targets,false);
}
void set_node_status(unsigned node,bool online,bool target) {
  system_state::set_status(node,online,target);
  if(node>=3 || (node_online[node]==online && node_target[node]==target)) return;
  const bool connectionChanged=node_online[node]!=online;
  node_online[node]=online; node_target[node]=target;
  if(connectionChanged && map_layer && system_state::enabled(node)) lv_obj_invalidate(map_layer);
  if(root && !demo && !editing) {
    const auto a=view.point(24,680),b=view.point(970,719);
    lv_area_t area={a.x,a.y,b.x,b.y}; lv_obj_invalidate_area(root,&area);
  }
}
void settings_changed() {
  dirty_targets(); clear_trails(); frame={}; selected=0; rebuild();
  set_mode(editing); lv_obj_invalidate(root);
}
void toggle_demo() {
  if(editing) return;
  demo=!demo; clear_trails(); frame={};
  if(visible) lv_timer_resume(timer); else lv_timer_pause(timer);
  lv_label_set_text(lv_obj_get_child(demo_button,0),demo?"DEMO":"LIVE");
  lv_obj_invalidate(root);
}
}
