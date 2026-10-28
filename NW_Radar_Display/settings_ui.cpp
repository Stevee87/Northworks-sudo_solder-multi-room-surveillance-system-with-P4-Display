#include "settings_ui.h"
#include "radar_palette.h"
#include "system_state.h"
#include "radar_map.h"
#include "floor_radar.h"
#include <algorithm>
namespace settings_ui {
namespace {
lv_obj_t *overlay,*panel,*nodeButtons[3],*scale,*volume,*audio,*scaleText,*volumeText,*error;
system_state::Settings draft;
bool opened=false;
void sync() {
  for(unsigned i=0;i<3;++i) {
    if(draft.activeNodes==i+1) lv_obj_add_state(nodeButtons[i],LV_STATE_CHECKED);
    else lv_obj_clear_state(nodeButtons[i],LV_STATE_CHECKED);
  }
  lv_slider_set_value(scale,draft.scalePercent/5,LV_ANIM_OFF);
  lv_slider_set_value(volume,draft.volume,LV_ANIM_OFF);
  lv_dropdown_set_selected(audio,unsigned(draft.audioMode));
  lv_label_set_text_fmt(scaleText,"%u%%",unsigned(draft.scalePercent));
  lv_label_set_text_fmt(volumeText,"%u%%",unsigned(draft.volume));
  lv_label_set_text(error,"");
}
void close() { opened=false; lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN); }
void action(lv_event_t *e) {
  const auto id=uintptr_t(lv_event_get_user_data(e));
  if(id>=1 && id<=3) { draft.activeNodes=id; sync(); }
  else if(id==4) { draft.scalePercent=lv_slider_get_value(scale)*5; lv_label_set_text_fmt(scaleText,"%u%%",unsigned(draft.scalePercent)); }
  else if(id==5) { draft.audioMode=system_state::AudioMode(lv_dropdown_get_selected(audio)); }
  else if(id==6) { draft.volume=lv_slider_get_value(volume); lv_label_set_text_fmt(volumeText,"%u%%",unsigned(draft.volume)); }
  else if(id==7) {
    if(!system_state::save(draft)) { lv_label_set_text(error,"SAVE FAILED"); return; }
    system_state::apply(draft); radar_map::settings_changed(); floor_radar::refresh(); close();
  } else if(id==8) close();
  else if(id==9) { draft={}; sync(); }
}
lv_obj_t *label(const char *text) {
  auto *obj=lv_label_create(panel); lv_label_set_text(obj,text); return obj;
}
lv_obj_t *button(lv_obj_t *parent,const char *text,uintptr_t id) {
  auto *obj=lv_btn_create(parent); lv_obj_set_size(obj,145,44);
  lv_obj_set_style_bg_color(obj,lv_color_hex(radar_palette::panel),0);
  lv_obj_set_style_bg_opa(obj,LV_OPA_COVER,0);
  lv_obj_set_style_text_color(obj,lv_color_hex(radar_palette::text),0);
  lv_obj_set_style_bg_color(obj,lv_color_hex(radar_palette::wall),LV_STATE_CHECKED);
  lv_obj_set_style_radius(obj,4,0);
  auto *caption=lv_label_create(obj); lv_label_set_text(caption,text); lv_obj_center(caption);
  lv_obj_add_event_cb(obj,action,LV_EVENT_CLICKED,reinterpret_cast<void*>(id)); return obj;
}
lv_obj_t *row() {
  auto *obj=lv_obj_create(panel); lv_obj_remove_style_all(obj); lv_obj_set_size(obj,LV_PCT(100),46);
  lv_obj_set_flex_flow(obj,LV_FLEX_FLOW_ROW); lv_obj_set_flex_align(obj,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
  return obj;
}
}
void create(lv_obj_t *parent,int w,int h) {
  overlay=lv_obj_create(parent); lv_obj_remove_style_all(overlay); lv_obj_set_size(overlay,w,h);
  lv_obj_set_style_bg_color(overlay,lv_color_hex(radar_palette::background),0); lv_obj_set_style_bg_opa(overlay,220,0);
  panel=lv_obj_create(overlay); lv_obj_set_size(panel,std::min(w-24,610),std::min(h-24,620)); lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel,lv_color_hex(radar_palette::map),0); lv_obj_set_style_border_color(panel,lv_color_hex(radar_palette::wall),0);
  lv_obj_set_style_bg_opa(panel,LV_OPA_COVER,0); lv_obj_set_style_border_width(panel,1,0); lv_obj_set_style_pad_all(panel,16,0);
  lv_obj_set_style_radius(panel,6,0); lv_obj_set_style_text_color(panel,lv_color_hex(radar_palette::text),0);
  lv_obj_set_style_text_font(panel,&lv_font_montserrat_14,0); lv_obj_set_style_pad_row(panel,12,0);
  lv_obj_set_flex_flow(panel,LV_FLEX_FLOW_COLUMN);
  label("SYSTEM SETTINGS"); label("ACTIVE RADAR NODES");
  auto *nodes=row(); for(unsigned i=0;i<3;++i) nodeButtons[i]=button(nodes,i==0?"1":i==1?"2":"3",i+1);
  label("RADAR MAP SCALE / FLOORPLAN METRIC"); scaleText=label("");
  scale=lv_slider_create(panel); lv_obj_set_width(scale,LV_PCT(95)); lv_obj_set_height(scale,16); lv_obj_set_ext_click_area(scale,14);
  lv_slider_set_range(scale,8,20); lv_obj_add_event_cb(scale,action,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(4));
  label("AUDIO MODE"); audio=lv_dropdown_create(panel); lv_dropdown_set_options(audio,"OFF\nCLASSIC PING\nTACTICAL ALERT");
  lv_obj_set_size(audio,LV_PCT(100),44); lv_obj_add_event_cb(audio,action,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(5));
  lv_obj_set_style_bg_color(audio,lv_color_hex(radar_palette::panel),0); lv_obj_set_style_bg_opa(audio,LV_OPA_COVER,0);
  lv_obj_set_style_text_color(audio,lv_color_hex(radar_palette::text),0); lv_obj_set_style_pad_all(audio,12,0);
  auto *list=lv_dropdown_get_list(audio);
  lv_obj_set_style_bg_color(list,lv_color_hex(radar_palette::panel),0); lv_obj_set_style_bg_opa(list,LV_OPA_COVER,0);
  lv_obj_set_style_text_color(list,lv_color_hex(radar_palette::text),0); lv_obj_set_style_pad_all(list,12,0);
  lv_obj_set_style_bg_color(list,lv_color_hex(radar_palette::wall),LV_PART_SELECTED); lv_obj_set_style_bg_opa(list,LV_OPA_COVER,LV_PART_SELECTED);
  label("MASTER VOLUME"); volumeText=label("");
  volume=lv_slider_create(panel); lv_obj_set_width(volume,LV_PCT(95)); lv_obj_set_height(volume,16); lv_obj_set_ext_click_area(volume,14);
  lv_slider_set_range(volume,0,100); lv_obj_add_event_cb(volume,action,LV_EVENT_VALUE_CHANGED,reinterpret_cast<void*>(6));
  for(auto *slider:{scale,volume}) {
    lv_obj_set_style_bg_color(slider,lv_color_hex(radar_palette::grid),LV_PART_MAIN); lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider,lv_color_hex(radar_palette::wall),LV_PART_INDICATOR); lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider,lv_color_hex(radar_palette::text),LV_PART_KNOB); lv_obj_set_style_bg_opa(slider,LV_OPA_COVER,LV_PART_KNOB);
    lv_obj_set_style_radius(slider,LV_RADIUS_CIRCLE,LV_PART_KNOB); lv_obj_set_style_pad_all(slider,7,LV_PART_KNOB);
  }
  error=label(""); auto *actions=row(); button(actions,"SAVE",7); button(actions,"CANCEL",8); button(actions,"DEFAULTS",9);
  lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN);
}
void open() { draft=system_state::settings(); sync(); opened=true; lv_obj_clear_flag(overlay,LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(overlay); }
bool is_open() { return opened; }
bool mute_preview() { return opened && draft.audioMode==system_state::AudioMode::Off; }
}
