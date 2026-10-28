#include "app_ui.h"
#include "system_state.h"
#include "settings_ui.h"
#include "floor_radar.h"
#include "sonar_audio.h"
#include "radar_palette.h"
#include "radar_feedback.h"
#include <initializer_list>
#include <cstdio>
#include <cstring>
#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace app_ui {
namespace {
lv_obj_t *floor_view,*selector,*caption;
lv_obj_t *settingsButton,*floorDemo,*floorDemoLabel,*signalLabel;
char previousSignal[180]{};
bool radar_active=false,locked=false;
void change(lv_event_t *) {
  if (floorplan::is_editing() || radar_map::is_editing() || floor_radar::is_editing()) return;
  radar_active=!radar_active;
  if (radar_active) lv_obj_add_flag(floor_view,LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(floor_view,LV_OBJ_FLAG_HIDDEN);
  radar_map::set_visible(radar_active);
  lv_label_set_text(caption,radar_active?"RADAR MAP":"FLOORPLAN");
#ifdef ARDUINO
  Serial.println(radar_active?"DISPLAY MODE: RADAR MAP":"DISPLAY MODE: FLOORPLAN");
#endif
}
}
void create(lv_obj_t *parent,int w,int h,floorplan::FloorplanConfig fc,floorplan::SaveCallback fs,
            const radar::Config &rc,radar_map::SaveCallback rs) {
  floor_view=floorplan::create(parent,w,h,fc,fs);
  floor_radar::create(floor_view,w,h);
  radar_map::create(parent,w,h,rc,rs);
  selector=lv_btn_create(parent); lv_obj_remove_style_all(selector);
  auto vp=floorplan::viewport(w,h); auto a=vp.point(620,8),b=vp.point(812,52);
  lv_obj_set_pos(selector,a.x,a.y); lv_obj_set_size(selector,b.x-a.x,44);
  lv_obj_set_style_bg_color(selector,lv_color_hex(radar_palette::panel),0);
  lv_obj_set_style_bg_opa(selector,LV_OPA_COVER,0);
  lv_obj_set_style_border_width(selector,1,0); lv_obj_set_style_border_color(selector,lv_color_hex(radar_palette::wall),0);
  lv_obj_set_style_radius(selector,4,0); lv_obj_set_style_opa(selector,LV_OPA_50,LV_STATE_DISABLED);
  lv_obj_set_style_text_font(selector,&lv_font_montserrat_14,0);
  lv_obj_set_style_text_color(selector,lv_color_hex(radar_palette::text),0);
  lv_obj_clear_flag(selector,LV_OBJ_FLAG_SCROLLABLE);
  caption=lv_label_create(selector); lv_label_set_text(caption,"FLOORPLAN"); lv_obj_center(caption);
  lv_obj_add_event_cb(selector,change,LV_EVENT_CLICKED,nullptr);
  settingsButton=lv_btn_create(parent);
  a=vp.point(840,8); b=vp.point(1010,52);
  lv_obj_set_pos(settingsButton,a.x,a.y); lv_obj_set_size(settingsButton,b.x-a.x,44);
  auto *label=lv_label_create(settingsButton); lv_label_set_text(label,"SETTINGS"); lv_obj_center(label);
  lv_obj_add_event_cb(settingsButton,[](lv_event_t*) { settings_ui::open(); },LV_EVENT_CLICKED,nullptr);
  floorDemo=lv_btn_create(floor_view); a=vp.point(1000,674); b=vp.point(1200,718);
  lv_obj_set_pos(floorDemo,a.x,a.y); lv_obj_set_size(floorDemo,b.x-a.x,44);
  floorDemoLabel=lv_label_create(floorDemo); lv_label_set_text(floorDemoLabel,"LIVE"); lv_obj_center(floorDemoLabel);
  for(auto *button:{settingsButton,floorDemo}) {
    lv_obj_set_style_bg_color(button,lv_color_hex(radar_palette::panel),0); lv_obj_set_style_bg_opa(button,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(button,1,0); lv_obj_set_style_border_color(button,lv_color_hex(radar_palette::wall),0);
    lv_obj_set_style_radius(button,4,0); lv_obj_set_style_text_font(button,&lv_font_montserrat_14,0);
    lv_obj_set_style_text_color(button,lv_color_hex(radar_palette::text),0);
    lv_obj_clear_flag(button,LV_OBJ_FLAG_SCROLLABLE);
  }
  lv_obj_add_event_cb(floorDemo,[](lv_event_t*) { radar_map::toggle_demo(); },LV_EVENT_CLICKED,nullptr);
  signalLabel=lv_label_create(parent);a=vp.point(24,46);b=vp.point(610,76);
  lv_obj_set_pos(signalLabel,a.x,a.y);lv_obj_set_size(signalLabel,b.x-a.x,20);
  lv_obj_set_style_bg_color(signalLabel,lv_color_hex(radar_palette::background),0);lv_obj_set_style_bg_opa(signalLabel,LV_OPA_COVER,0);
  lv_obj_set_style_text_color(signalLabel,lv_color_hex(radar_palette::text),0);lv_obj_set_style_text_font(signalLabel,&lv_font_montserrat_14,0);
  lv_label_set_text(signalLabel,"");previousSignal[0]=0;
  settings_ui::create(parent,w,h);
}
void poll() {
  static bool feedbackDemo=false;
  if(feedbackDemo!=system_state::demo_mode()) {
    feedbackDemo=system_state::demo_mode(); radar_feedback::shared().clear_trails();
  }
  const auto frame=system_state::frame(lv_tick_get());
  if(!system_state::demo_mode()) radar_map::set_targets(frame);
  auto audioSettings=system_state::settings();
  if(settings_ui::mute_preview()) audioSettings.audioMode=system_state::AudioMode::Off;
  sonar_audio::update(audioSettings,frame,lv_tick_get());
  sonar_audio::DetectionEvent acquisitions[3];
  const auto acquisitionCount=radar_feedback::shared().update(system_state::settings(),frame,lv_tick_get(),acquisitions);
  for(unsigned i=0;i<acquisitionCount;++i) sonar_audio::submit(acquisitions[i]);
  floor_radar::poll(!radar_active);
  static bool shownDemo=false;
  if(shownDemo!=system_state::demo_mode()) {
    shownDemo=system_state::demo_mode(); lv_label_set_text(floorDemoLabel,shownDemo?"DEMO":"LIVE");
  }
#ifdef ARDUINO
  // Serial-only evidence of completed demo updates, with no HUD clutter.
  static uint32_t start=0,count=0;
  const auto now=lv_tick_get();
  if (!radar_active || !radar_map::demo_enabled() || radar_map::is_editing()) start=0;
  else if (!start) { start=now; count=radar_map::demo_updates(); }
  else if (now-start>=5000) {
    Serial.printf("RADAR DEMO: %.1f updates/s\n",(radar_map::demo_updates()-count)*1000.0f/(now-start));
    start=now; count=radar_map::demo_updates();
  }
#endif
  const bool editing=floorplan::is_editing() || radar_map::is_editing() || floor_radar::is_editing();
  if(editing!=lv_obj_has_flag(settingsButton,LV_OBJ_FLAG_HIDDEN)) {
    if(editing) { lv_obj_add_flag(settingsButton,LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(floorDemo,LV_OBJ_FLAG_HIDDEN); }
    else { lv_obj_clear_flag(settingsButton,LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(floorDemo,LV_OBJ_FLAG_HIDDEN); }
  }
  const bool hideSignal=editing||settings_ui::is_open();
  if(hideSignal!=lv_obj_has_flag(signalLabel,LV_OBJ_FLAG_HIDDEN)) {
    if(hideSignal)lv_obj_add_flag(signalLabel,LV_OBJ_FLAG_HIDDEN);else lv_obj_clear_flag(signalLabel,LV_OBJ_FLAG_HIDDEN);
  }
  char signal[180]="";size_t length=0;
  for(unsigned n=0;n<system_state::settings().activeNodes;++n) {
    const int dbm=system_state::rssi(n);char value[24];
    if(dbm)std::snprintf(value,sizeof(value),"%d dBm",dbm);
    else std::snprintf(value,sizeof(value),"%s",!std::strcmp(system_state::status(n),"OFFLINE")?"OFFLINE":"-- dBm");
    length+=std::snprintf(signal+length,sizeof(signal)-length,"%sN%u %s",n?"  ":"",n+1,value);
  }
  if(std::strcmp(signal,previousSignal)) { std::strcpy(previousSignal,signal);lv_label_set_text(signalLabel,signal); }
  const bool next=editing || settings_ui::is_open();
  if (next==locked) return;
  locked=next;
  if (locked) lv_obj_add_state(selector,LV_STATE_DISABLED);
  else lv_obj_clear_state(selector,LV_STATE_DISABLED);
}
bool radar_visible() { return radar_active; }
}
