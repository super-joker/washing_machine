#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "led.h"


static const char *TAG = "main";

void app_main(void)
{
    pwm_init();
    ESP_LOGI(TAG, "Breathing LED: GPIO%d, %d Hz, fade time %d ms",
             PWM_GPIO, PWM_FREQ_HZ, PWM_FADE_TIME_MS);

    while (1) {
        pwm_fade_to(PWM_FULL_DUTY);  // 从灭逐渐变亮
        pwm_fade_to(0);              // 从亮逐渐熄灭
        vTaskDelay(pdMS_TO_TICKS(PWM_DARK_HOLD_MS));
    }
}
