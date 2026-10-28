#pragma once

// ELECROW CrowPanel Advance 5.0" ESP32-P4 (800x480 RGB ST7262, V1.0)
// Pin values harvested from ELECROW's own Arduino lessons for this board:
//   panel/touch  -> Lesson07 (Turn_on_the_screen)
//   C6 SDIO      -> Lesson16 (Wi-Fi_function)
//   audio        -> Lesson12 (Playing_Loca_Music_from_SD_Card)

/*********************** Pin define ***********************/
// GPIO pins for GT911 touch panel
#define Touch_GPIO_RST      (36)    // Reset pin
#define Touch_GPIO_INT      (42)    // Interrupt pin

// GPIO pins for I2C, has touch chip GT911 (and the STC8H1KXX helper MCU
// that owns backlight PWM / battery / amp-shutdown, addr 0x2F)
#define I2C_GPIO_SCL        (46)    // GPIO number used for I2C SCL (clock) line
#define I2C_GPIO_SDA        (45)    // GPIO number used for I2C SDA (data) line

// display size
#define H_size              (800)   // Horizontal resolution (X-axis)
#define V_size              (480)   // Vertical resolution (Y-axis)

// panel parameters
// Refresh Rate = 18000000/(4+8+8+800)/(4+16+16+480) = 42Hz
#define LCD_CLK_MHZ         (18)
#define LCD_HPW             ( 4)
#define LCD_HBP             ( 8)
#define LCD_HFP             ( 8)
#define LCD_VPW             ( 4)
#define LCD_VBP             (16)
#define LCD_VFP             (16)

// RGB interface Pin
#define LCD_GPIO_RST        (-1)    // LCD reset GPIO
#define RGB_PIN_NUM_DISP_EN (-1)
#define RGB_PIN_NUM_HSYNC   (40)
#define RGB_PIN_NUM_VSYNC   (41)
#define RGB_PIN_NUM_DE      ( 2)
#define RGB_PIN_NUM_PCLK    ( 3)

#define RGB_PIN_NUM_DATA0   ( 8)
#define RGB_PIN_NUM_DATA1   ( 7)
#define RGB_PIN_NUM_DATA2   ( 6)
#define RGB_PIN_NUM_DATA3   ( 5)
#define RGB_PIN_NUM_DATA4   ( 4)
#define RGB_PIN_NUM_DATA5   (14)
#define RGB_PIN_NUM_DATA6   (13)
#define RGB_PIN_NUM_DATA7   (12)
#define RGB_PIN_NUM_DATA8   (11)
#define RGB_PIN_NUM_DATA9   (10)
#define RGB_PIN_NUM_DATA10  ( 9)
#define RGB_PIN_NUM_DATA11  (19)
#define RGB_PIN_NUM_DATA12  (18)
#define RGB_PIN_NUM_DATA13  (17)
#define RGB_PIN_NUM_DATA14  (16)
#define RGB_PIN_NUM_DATA15  (15)

// SDIO interface pins for the onboard ESP32-C6 (esp-hosted Wi-Fi)
// NOTE: these are ELECROW's V1.0 values. ELECROW has remapped wireless-adjacent
// pins between revisions in this series -- if Wi-Fi never comes up on a later
// board revision, check these against the wiki for your exact rev.
#define WIFI_HOSTED_SDIO_PIN_CMD    (54)  // SDIO Command/Response line
#define WIFI_HOSTED_SDIO_PIN_CLK    (53)  // SDIO Serial Clock
#define WIFI_HOSTED_SDIO_PIN_D0     (52)  // SDIO Data line 0
#define WIFI_HOSTED_SDIO_PIN_D1     (51)  // SDIO Data line 1
#define WIFI_HOSTED_SDIO_PIN_D2     (50)  // SDIO Data line 2
#define WIFI_HOSTED_SDIO_PIN_D3     (49)  // SDIO Data line 3 (4-bit mode)
#define WIFI_HOSTED_SDIO_PIN_RESET  (20)  // Hardware reset for the C6

// Speaker I2S (no codec; amp shutdown pin lives on the STC8H1KXX)
#define AUDIO_GPIO_LRCLK        (21)
#define AUDIO_GPIO_BCLK         (22)
#define AUDIO_GPIO_SDATA        (23)
#define AUDIO_POWER_ENABLE      (LOW)   // STC8 AUDIO_SD: low = amp on
#define AUDIO_POWER_DISABLE     (HIGH)  // STC8 AUDIO_SD: high = amp off
/*********************** Pin define ***********************/
