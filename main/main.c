#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "button.h"
#include "led.h"

static const char *TAG = "main";

static const char *button_state_str(button_state_t state)
{
    switch (state) {
    case BUTTON_STATE_SHORT_PRESS:
        return "短按";
    case BUTTON_STATE_LONG_PRESS:
        return "长按";
    case BUTTON_STATE_DOUBLE_CLICK:
        return "双击";
    default:
        return "无";
    }
}

static void poll_button(const char *name, gpio_num_t gpio)
{
    button_state_t state;

    while ((state = acquire_button_state(gpio)) != BUTTON_STATE_NONE) {
        ESP_LOGI(TAG, "%s GPIO%d: %s", name, (int)gpio, button_state_str(state));
    }
}

/* pwm_fade_to() 会等到这一程渐变结束。放在单独任务里，才不会挡住按键轮询。 */
static void breathing_led_task(void *arg)
{
    (void)arg;

    while (1) {
        pwm_fade_to(PWM_FULL_DUTY);
        pwm_fade_to(0);
        vTaskDelay(pdMS_TO_TICKS(PWM_DARK_HOLD_MS));
    }
}

void app_main(void)
{
    pwm_init();
    button_init();

    if (xTaskCreate(breathing_led_task, "breath_led", 3072, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "创建呼吸灯任务失败");
    }

    ESP_LOGI(TAG, "按键测试: 左键 GPIO%d, 右键 GPIO%d", BUTTON_LEFT, BUTTON_RIGHT);

    while (1) {
        poll_button("左键", BUTTON_LEFT);
        poll_button("右键", BUTTON_RIGHT);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
