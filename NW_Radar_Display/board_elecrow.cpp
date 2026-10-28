#include "board_select.h"
#ifdef BOARD_ELECROW_P4
#include <Arduino.h>
#include "board_display.h"
#include "board_config.h"
#include "bsp_i2c.h"
#include "bsp_stc8h1kxx.h"
#include "lvgl_v8_port.h"

namespace {
esp_panel::board::Board board;
}
namespace board_display {
bool begin() {
  // Preserve Steve's I2C -> helper MCU -> panel -> LVGL sequence and pins.
  if (i2c_init(I2C_GPIO_SCL,I2C_GPIO_SDA)!=ESP_OK) return false;
  if (stc8_gpio_set_level(STC8_GPIO_OUT_AUDIO_SD,AUDIO_POWER_DISABLE)!=ESP_OK) return false;
  if (stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN,0)!=ESP_OK) return false;
  if (!board.init() || !board.begin()) return false;
  auto *touch=board.getTouch();
  if(!touch || !touch->getPanelHandle()) {
    Serial.println("ELECROW: GT911 initialization failed");
    return false;
  }
  const bool ready=lvgl_port_init(board.getLCD(),touch);
  Serial.printf("ELECROW: 800x480 display + GT911 touch %s\n",ready?"READY":"FAILED");
  return ready;
}
bool lock() { return lvgl_port_lock(-1); }
void unlock() { lvgl_port_unlock(); }
void poll() {} // Steve's port owns the LVGL tick and GUI task.
void show() { stc8_set_pwm_duty(STC8_PWM_LCD_BL_EN,100); }
}
#endif
