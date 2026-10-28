#ifndef _BSP_STC8H1KXX_H_
#define _BSP_STC8H1KXX_H_

/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "bsp_i2c.h"
#include "driver/gpio.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

#ifdef __cplusplus
extern "C"
{
#endif

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#define STC8H1KXX_TAG "STC8H1KXX"
#define STC8H1KXX_INFO(fmt, ...)        ESP_LOGI(STC8H1KXX_TAG, fmt, ##__VA_ARGS__)
#define STC8H1KXX_DEBUG(fmt, ...)       ESP_LOGD(STC8H1KXX_TAG, fmt, ##__VA_ARGS__)
#define STC8H1KXX_ERROR(fmt, ...)       ESP_LOGE(STC8H1KXX_TAG, fmt, ##__VA_ARGS__)

#define CONFIG_BSP_STC8H1KXX_ENABLED
#define CONFIG_BSP_STC8H1KXX_BATTERY_ENABLED
#define CONFIG_BSP_STC8H1KXX_GPIO_ENABLED
#define CONFIG_BSP_STC8H1KXX_PWM_ENABLED

#ifdef CONFIG_BSP_STC8H1KXX_ENABLED
typedef uint8_t    u8;
typedef uint16_t   u16;
typedef uint32_t   u32;

#ifdef CONFIG_BSP_STC8H1KXX_BATTERY_ENABLED

/**************** STC I2C register address and control command ****************/
#define STC8_I2C_SLAVE_DEV_ADDR     0x2F

typedef enum
{
    STC8_REG_ADDR_BATTERY   = 0x00, // Get battery information
    STC8_REG_ADDR_GET_GPIO  = 0x10, // Get GPIO input level
    STC8_REG_ADDR_SET_GPIO  = 0x18, // Set GPIO output level
    STC8_REG_ADDR_SET_PWM   = 0x20, // Set PWM duty cycle
}EM_STC8_REG_ADDR;

typedef struct{
    u32 adc_voltage;    //Collected voltage(mV)
    u32 bat_voltage;    //The battery voltage after converting the voltage divider resistor(mV)
    u8 bat_level;       //Battery percentage charge(%)
    u8 bat_state;       //battery status (refer to EM_BAT_CHARGE_STATE)
    u8 led_state;       //led indicator light status (refer to EM_LED_STATE)
}Battery_info_t;

typedef enum
{
    BAT_CHARGE_IDLE = 0,
    BAT_CHARGE_CHARGING,        //Charging
    BAT_CHARGE_FULLY_CHARGED,   //Already full
    BAT_CHARGE_NO_CHARGE,       //Not charged
    BAT_CHARGE_ERROR,           //error condition
}EM_BAT_CHARGE_STATE;

typedef enum
{
    LED_IDLE = 0,
    LED_CHARGING,        //Charging: Red light
    LED_FULLY_CHARGED,   //Full: Green light
    LED_NO_CHARGE,       //Uncharged
    LED_LOW_POWER,       //Low voltage :0.5HZ red light
}EM_LED_STATE;

typedef enum
{
    STC8_GPIO_IN_SW_SPI_UART = 0,   // UART/SPI switch toggle detection pin
    
    STC8_GPIO_IN_MAX
}EM_STC8_GPIO_IN;

typedef enum
{
    STC8_GPIO_OUT_TP_RST = 0,   // Touch panel reset pin
    STC8_GPIO_OUT_CSI_RST,      // Camera (CSI) reset pin
    STC8_GPIO_OUT_AUDIO_SD,     // Audio amplifier shutdown/enable pin
    STC8_GPIO_OUT_LCD_BL_POWER, // LCD backlight power pin
    
    STC8_GPIO_OUT_MAX,
}EM_STC8_GPIO_OUT;

typedef enum
{
    STC8_PWM_LCD_BL_EN = 0,     // Backlight PWM enable pin
    
    STC8_PWM_MAX,
}EM_STC8_PWM;
/**************** STC I2C register address and control command ****************/

/**
 * @brief Read battery information from STC8H1KXX via I2C
 * @param bat_info: Pointer to Battery_info_t structure to store battery data
 * @return esp_err_t: ESP_OK on successful read, error code on failure
 */
esp_err_t stc8_battery_info_get(Battery_info_t *bat_info);

#endif

#ifdef CONFIG_BSP_STC8H1KXX_GPIO_ENABLED

/**
 * @brief Get input level of specified STC8H1KXX GPIO pin
 * @param gpio_num: GPIO number (refer to EM_STC8_GPIO_IN enum for valid values)
 * @param level: Pointer to store the read GPIO level (0 = low, 1 = high)
 * @return esp_err_t: ESP_OK on successful read, ESP_FAIL on invalid GPIO or communication failure
 */
esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level);

/**
 * @brief Set output level of specified STC8H1KXX GPIO pin
 * @param gpio_num: GPIO number (refer to EM_STC8_GPIO_OUT enum for valid values)
 * @param level: GPIO level to set (0 = low, 1 = high)
 * @return esp_err_t: ESP_OK on successful write, ESP_FAIL on invalid GPIO or communication failure
 */
esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level);

#endif

#ifdef CONFIG_BSP_STC8H1KXX_PWM_ENABLED

/**
 * @brief Set duty cycle of specified STC8H1KXX PWM channel
 * @param pwm_num: PWM channel number (refer to EM_STC8_PWM enum for valid values)
 * @param duty: PWM duty cycle (0~100, percentage of full cycle)
 * @return esp_err_t: ESP_OK on successful write, ESP_FAIL on invalid PWM channel or communication failure
 */
esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty);

#endif
/*———————————————————————————————————————Variable declaration end——————————————-—————————————————————————*/
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif