#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "button.h"
#include "seg.h"

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

void app_main(void)
{
    int value = 0;
    int elapsed_ms = 0;

    button_init();
    seg_init();
    seg_show_number(value);
    ESP_LOGI(TAG, "数码管测试，左键 GPIO%d，右键 GPIO%d", BUTTON_LEFT, BUTTON_RIGHT);

    while (1) {
        poll_button("左键", BUTTON_LEFT);
        poll_button("右键", BUTTON_RIGHT);
        vTaskDelay(pdMS_TO_TICKS(20));

        elapsed_ms += 20;
        if (elapsed_ms >= 500) {
            elapsed_ms = 0;
            value = (value + 1) % 10000;
            seg_show_number(value);
        }
    }
}
