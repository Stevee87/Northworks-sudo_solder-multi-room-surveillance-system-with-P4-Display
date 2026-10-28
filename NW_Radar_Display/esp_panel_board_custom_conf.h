/*
 * esp_panel_board_custom_conf.h
 * ------------------------------
 * Custom ESP32_Display_Panel configuration for the Elecrow CrowPanel
 * Advance ESP32-P4 5-inch display (800x480, ST7262 RGB, GT911 touch).
 *
 * Pin/timing sources: the official Elecrow repository
 * "Elecrow-RD/-CrowPanel-Advanced-5inch-ESP32-P4-HMI-AI-Display-800x480-IPS-Touch-Screen"
 * and Elecrow's LCD driver documentation. This project configuration
 * has been compiled with the versions listed in the package README.
 *
 * The Arduino library ZIP also includes this configuration so the
 * library can find it without additional compiler flags.
 * LCD and touch mirror X/Y values are set to 1 for 180-degree rotation.
 */

#pragma once

// *INDENT-OFF*

#define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM (1)

#if ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM

////////////////////////////////////////////////////////////////////////////
// General parameters
////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_NAME                "Elecrow:CrowPanel_Advance_P4_5inch"

#define ESP_PANEL_BOARD_WIDTH                (800)
#define ESP_PANEL_BOARD_HEIGHT               (480)

////////////////////////////////////////////////////////////////////////////
// LCD panel
////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_LCD              (1)

#if ESP_PANEL_BOARD_USE_LCD

#define ESP_PANEL_BOARD_LCD_CONTROLLER       ST7262

#define ESP_PANEL_BOARD_LCD_BUS_TYPE         (ESP_PANEL_BUS_TYPE_RGB)

#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB

// The Elecrow board uses a direct RGB interface (no 3-wire SPI control panel)
#define ESP_PANEL_BOARD_LCD_RGB_USE_CONTROL_PANEL   (0)

/* Timing based on Elecrow's formula:
 * Refresh Rate = 18000000/(1+40+20+800)/(1+10+5+480) = 42Hz
 * PCLK = 18 MHz
 */
#define ESP_PANEL_BOARD_LCD_RGB_CLK_HZ        (18 * 1000 * 1000)
#define ESP_PANEL_BOARD_LCD_RGB_HPW           (4)
#define ESP_PANEL_BOARD_LCD_RGB_HBP           (8)
#define ESP_PANEL_BOARD_LCD_RGB_HFP           (8)
#define ESP_PANEL_BOARD_LCD_RGB_VPW           (4)
#define ESP_PANEL_BOARD_LCD_RGB_VBP           (16)
#define ESP_PANEL_BOARD_LCD_RGB_VFP           (16)
#define ESP_PANEL_BOARD_LCD_RGB_PCLK_ACTIVE_NEG (0)

#define ESP_PANEL_BOARD_LCD_RGB_DATA_WIDTH    (16)
#define ESP_PANEL_BOARD_LCD_RGB_PIXEL_BITS    (ESP_PANEL_LCD_COLOR_BITS_RGB565)

#define ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE (ESP_PANEL_BOARD_WIDTH * 10)

#define ESP_PANEL_BOARD_LCD_RGB_IO_HSYNC      (40)
#define ESP_PANEL_BOARD_LCD_RGB_IO_VSYNC      (41)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DE         (2)
#define ESP_PANEL_BOARD_LCD_RGB_IO_PCLK       (3)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DISP       (-1)

// B0-4
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA0      (8)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA1      (7)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA2      (6)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA3      (5)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA4      (4)
// G0-5
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA5      (14)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA6      (13)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA7      (12)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA8      (11)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA9      (10)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA10     (9)
// R0-4
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA11     (19)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA12     (18)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA13     (17)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA14     (16)
#define ESP_PANEL_BOARD_LCD_RGB_IO_DATA15     (15)

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB

#define ESP_PANEL_BOARD_LCD_COLOR_BITS        (ESP_PANEL_LCD_COLOR_BITS_RGB565)
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER   (0)
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT  (0)

#define ESP_PANEL_BOARD_LCD_SWAP_XY           (0)
#define ESP_PANEL_BOARD_LCD_MIRROR_X          (1)   // 180-degree rotation
#define ESP_PANEL_BOARD_LCD_MIRROR_Y          (1)   // 180-degree rotation
#define ESP_PANEL_BOARD_LCD_GAP_X             (0)
#define ESP_PANEL_BOARD_LCD_GAP_Y             (0)

// No separate LCD reset pin is documented
#define ESP_PANEL_BOARD_LCD_RST_IO            (-1)
#define ESP_PANEL_BOARD_LCD_RST_LEVEL         (0)

#endif // ESP_PANEL_BOARD_USE_LCD

////////////////////////////////////////////////////////////////////////////
// Touch panel (GT911, I2C1 according to the Elecrow pinout)
////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_TOUCH             (1)

#if ESP_PANEL_BOARD_USE_TOUCH

#define ESP_PANEL_BOARD_TOUCH_CONTROLLER      GT911
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE        (ESP_PANEL_BUS_TYPE_I2C)

#if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C

#define ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID          (0)
#define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST   (1)

#if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
#define ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ      (400 * 1000)
#define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP  (1)
#define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP  (1)
#define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL      (46)
#define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA      (45)
#endif // !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST

#define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS     (0)

#endif // ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C

#define ESP_PANEL_BOARD_TOUCH_SWAP_XY         (0)
#define ESP_PANEL_BOARD_TOUCH_MIRROR_X        (1)   // 180-degree rotation
#define ESP_PANEL_BOARD_TOUCH_MIRROR_Y        (1)   // 180-degree rotation

#define ESP_PANEL_BOARD_TOUCH_RST_IO          (36)
#define ESP_PANEL_BOARD_TOUCH_RST_LEVEL       (0)
#define ESP_PANEL_BOARD_TOUCH_INT_IO          (42)
#define ESP_PANEL_BOARD_TOUCH_INT_LEVEL       (0)

#endif // ESP_PANEL_BOARD_USE_TOUCH

////////////////////////////////////////////////////////////////////////////
// Backlight
////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_BACKLIGHT         (0)

////////////////////////////////////////////////////////////////////////////
// IO expander (not required for this board)
////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_USE_EXPANDER          (0)

////////////////////////////////////////////////////////////////////////////
// Required configuration version (must match the installed
// ESP32_Display_Panel version; do not change)
////////////////////////////////////////////////////////////////////////////
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR 2
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH 0

#endif // ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
// *INDENT-ON*
