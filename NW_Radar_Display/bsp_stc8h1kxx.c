#include "board_select.h"
#ifdef BOARD_ELECROW_P4
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "bsp_stc8h1kxx.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
#ifdef CONFIG_BSP_STC8H1KXX_ENABLED

#endif
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
#ifdef CONFIG_BSP_STC8H1KXX_ENABLED

#ifdef CONFIG_BSP_STC8H1KXX_BATTERY_ENABLED

/**
 * @brief Read battery information from STC8H1KXX via I2C
 * @param bat_info: Pointer to Battery_info_t structure to store battery data
 * @return esp_err_t: ESP_OK on successful read, error code on failure
 */
esp_err_t stc8_battery_info_get(Battery_info_t *bat_info)
{
    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < sizeof(Battery_info_t); i++)
    {
        err = i2c_read_reg(STC8_I2C_SLAVE_DEV_ADDR, STC8_REG_ADDR_BATTERY+i, (uint8_t*)bat_info+i, 1);
        if (ESP_OK != err)
        {
            STC8H1KXX_ERROR("stc8 read battery info fail");
            return err;
        }
    }
    return err;
}

#endif

#ifdef CONFIG_BSP_STC8H1KXX_GPIO_ENABLED

/**
 * @brief Get input level of specified STC8H1KXX GPIO pin
 * @param gpio_num: GPIO number (refer to EM_STC8_GPIO_IN enum for valid values)
 * @param level: Pointer to store the read GPIO level (0 = low, 1 = high)
 * @return esp_err_t: ESP_OK on successful read, ESP_FAIL on invalid GPIO or communication failure
 */
esp_err_t stc8_gpio_get_level(int gpio_num, uint8_t* level)
{
    esp_err_t err;
    if (STC8_GPIO_IN_MAX <= gpio_num) {
        STC8H1KXX_ERROR("stc8 can't get gpio=%d", gpio_num);
        return ESP_FAIL;
    }    
    err = i2c_read_reg(STC8_I2C_SLAVE_DEV_ADDR, STC8_REG_ADDR_GET_GPIO + gpio_num, level, 1);
    if (ESP_OK != err)
    {
        STC8H1KXX_ERROR("stc8 get gpio=%d fail", gpio_num);
        return err;
    }
    return err;
}

/**
 * @brief Set output level of specified STC8H1KXX GPIO pin
 * @param gpio_num: GPIO number (refer to EM_STC8_GPIO_OUT enum for valid values)
 * @param level: GPIO level to set (0 = low, 1 = high)
 * @return esp_err_t: ESP_OK on successful write, ESP_FAIL on invalid GPIO or communication failure
 */
esp_err_t stc8_gpio_set_level(int gpio_num, uint8_t level)
{
    esp_err_t err;
    if (STC8_GPIO_OUT_MAX <= gpio_num) {
        STC8H1KXX_ERROR("stc8 can't set gpio=%d", gpio_num);
        return ESP_FAIL;
    }
    err = i2c_write_reg(STC8_I2C_SLAVE_DEV_ADDR, STC8_REG_ADDR_SET_GPIO + gpio_num, level);
    if (ESP_OK != err)
    {
        STC8H1KXX_ERROR("stc8 set gpio=%d fail", gpio_num);
        return err;
    }
    return err;
}

#endif

#ifdef CONFIG_BSP_STC8H1KXX_PWM_ENABLED

/**
 * @brief Set duty cycle of specified STC8H1KXX PWM channel
 * @param pwm_num: PWM channel number (refer to EM_STC8_PWM enum for valid values)
 * @param duty: PWM duty cycle (0~100, percentage of full cycle)
 * @return esp_err_t: ESP_OK on successful write, ESP_FAIL on invalid PWM channel or communication failure
 */
esp_err_t stc8_set_pwm_duty(int pwm_num, uint8_t duty)
{
    esp_err_t err;
    if (STC8_PWM_MAX <= pwm_num) {
        STC8H1KXX_ERROR("stc8 don't have pwm=%d", pwm_num);
        return false;
    }
    err = i2c_write_reg(STC8_I2C_SLAVE_DEV_ADDR, STC8_REG_ADDR_SET_PWM + pwm_num, duty);
    if (ESP_OK != err)
    {
        STC8H1KXX_ERROR("stc8 set pwm=%d fail", pwm_num);
        return err;
    }
    return err;
}

#endif

#endif
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/
#endif // BOARD_ELECROW_P4
