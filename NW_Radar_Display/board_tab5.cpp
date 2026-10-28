#include "board_select.h"
#ifdef BOARD_M5STACK_TAB5
#include <Arduino.h>
#include <M5Unified.h>
#include <esp_heap_caps.h>
#include "board_display.h"

static_assert(LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0,
              "Tab5 flush expects native RGB565");
namespace {
lv_disp_draw_buf_t draw_buffer;
lv_disp_drv_t driver;
lv_indev_drv_t touch_driver;
uint32_t last_tick;

void read_touch(lv_indev_drv_t *, lv_indev_data_t *data) {
  // M5GFX performs panel-specific calibration and the current display rotation.
  lgfx::touch_point_t point;
  if (M5.Display.getTouch(&point,1)) {
    data->point.x=point.x;
    data->point.y=point.y;
    data->state=LV_INDEV_STATE_PRESSED;
  } else data->state=LV_INDEV_STATE_RELEASED;
}

void flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *pixels) {
  // Explicit native RGB565 type avoids dependence on M5GFX byte-swap defaults.
  M5.Display.pushImage(area->x1, area->y1,
                       area->x2 - area->x1 + 1, area->y2 - area->y1 + 1,
                       reinterpret_cast<const lgfx::rgb565_t *>(pixels));
  M5.Display.waitDMA();
  lv_disp_flush_ready(disp);
}
}

namespace board_display {
bool begin() {
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_rtc = false;
  cfg.internal_mic = false;
  cfg.internal_spk = true;
  cfg.clear_display = true;
  M5.begin(cfg); // Tab5 power, panel detection, reset and MIPI-DSI handled here.
  M5.Display.setBrightness(0);
  M5.Display.setRotation(3); // Native 180-degree landscape correction; getTouch follows it.
  if (M5.Display.width() != 1280 || M5.Display.height() != 720) return false;
  M5.Display.setColorDepth(16);
  lv_init();
  constexpr uint32_t count = 1280 * 32;
  auto *buffer = static_cast<lv_color_t *>(heap_caps_malloc(
      count * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!buffer) return false;
  // One 80 KiB strip buffer; the display driver owns its required framebuffer.
  lv_disp_draw_buf_init(&draw_buffer, buffer, nullptr, count);
  lv_disp_drv_init(&driver);
  driver.hor_res = 1280;
  driver.ver_res = 720;
  driver.flush_cb = flush;
  driver.draw_buf = &draw_buffer;
  if (!lv_disp_drv_register(&driver)) return false;
  if (!M5.Display.touch()) return false;
  lv_indev_drv_init(&touch_driver);
  touch_driver.type=LV_INDEV_TYPE_POINTER;
  touch_driver.read_cb=read_touch;
  if (!lv_indev_drv_register(&touch_driver)) return false;
  last_tick = millis();
  return true;
}
bool lock() { return true; } // LVGL belongs exclusively to Arduino's loop task.
void unlock() {}
void poll() {
  const uint32_t now = millis();
  lv_tick_inc(now - last_tick);
  last_tick = now;
  lv_timer_handler(); // LVGL only redraws invalidated areas.
}
void show() { M5.Display.setBrightness(160); }
}
#endif
