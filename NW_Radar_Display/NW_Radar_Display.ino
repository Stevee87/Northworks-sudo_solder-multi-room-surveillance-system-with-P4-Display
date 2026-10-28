#include <Arduino.h>
#include <esp_heap_caps.h>
#include "board_display.h"
#include "floorplan.h"
#include "floorplan_storage.h"
#include "app_ui.h"
#include "radar_storage.h"
#include "radar_live.h"
#include "system_state.h"
#include "sonar_audio.h"

static void halt(const char *message) {
  Serial.println(message);
  while (true) delay(1000);
}

void setup() {
  Serial.begin(115200);
  if (!board_display::begin()) halt("ERROR: display/LVGL initialization failed");
  if (!board_display::lock()) halt("ERROR: LVGL lock failed");
  auto *display = lv_disp_get_default();
  const int w = lv_disp_get_hor_res(display);
  const int h = lv_disp_get_ver_res(display);
  const auto config=floorplan_storage::load();
  const auto radar_config=radar_storage::load();
  system_state::apply(system_state::load());
  app_ui::create(lv_scr_act(),w,h,config,floorplan_storage::save,radar_config,radar_storage::save);
  lv_refr_now(display);
  board_display::unlock();
  board_display::show();
  radar_live::begin();
  sonar_audio::update(system_state::settings(),system_state::frame(millis()));
  sonar_audio::begin();
  Serial.printf("PHASE 3 READY: %dx%d, LVGL %d.%d.%d\n", w, h,
                LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  Serial.printf("Free internal: %u; free PSRAM: %u bytes\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void loop() {
  board_display::poll();
  if (board_display::lock()) { radar_live::poll_ui(); app_ui::poll(); board_display::unlock(); }
  delay(5);
}
