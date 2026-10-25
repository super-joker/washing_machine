#include "button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "button_gpio.h"
#include "iot_button.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "button";

#define BUTTON_QUEUE_LEN  4

typedef struct {
    gpio_num_t gpio;
    button_handle_t handle;
    QueueHandle_t queue;
} button_slot_t;

static button_slot_t s_buttons[] = {
    { .gpio = BUTTON_LEFT },
    { .gpio = BUTTON_RIGHT },
};

static void button_event_cb(void *button_handle, void *usr_data)
{
    button_slot_t *slot = usr_data;
    button_state_t state = BUTTON_STATE_NONE;

    switch (iot_button_get_event(button_handle)) {
    case BUTTON_SINGLE_CLICK:
        state = BUTTON_STATE_SHORT_PRESS;
        break;
    case BUTTON_LONG_PRESS_START:
        state = BUTTON_STATE_LONG_PRESS;
        break;
    case BUTTON_DOUBLE_CLICK:
        state = BUTTON_STATE_DOUBLE_CLICK;
        break;
    default:
        return;
    }

    /* 回调跑在 esp_timer 任务里，不能阻塞。队列满时丢掉最旧的一次。 */
    if (xQueueSend(slot->queue, &state, 0) != pdTRUE) {
        button_state_t dropped;
        xQueueReceive(slot->queue, &dropped, 0);
        xQueueSend(slot->queue, &state, 0);
    }
}

static esp_err_t button_create(button_slot_t *slot)
{
    const button_config_t btn_cfg = {
        .long_press_time = BUTTON_LONG_PRESS_MS,
        .short_press_time = BUTTON_SHORT_PRESS_MS,
    };
    const button_gpio_config_t gpio_cfg = {
        .gpio_num = slot->gpio,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .enable_power_save = false,
        .disable_pull = BUTTON_DISABLE_PULL,
    };
    const button_event_t events[] = {
        BUTTON_SINGLE_CLICK,
        BUTTON_DOUBLE_CLICK,
        BUTTON_LONG_PRESS_START,
    };

    slot->queue = xQueueCreate(BUTTON_QUEUE_LEN, sizeof(button_state_t));
    if (slot->queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &slot->handle);
    if (err != ESP_OK) {
        return err;
    }

    for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++) {
        err = iot_button_register_cb(slot->handle, events[i], NULL, button_event_cb, slot);
        if (err != ESP_OK) {
            return err;
        }
    }
    return ESP_OK;
}

void button_init(void)
{
    for (size_t i = 0; i < sizeof(s_buttons) / sizeof(s_buttons[0]); i++) {
        esp_err_t err = button_create(&s_buttons[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "create button gpio %d failed: %s",
                     (int)s_buttons[i].gpio, esp_err_to_name(err));
            button_deinit();
            ESP_ERROR_CHECK(err);
        }
    }
}

void button_deinit(void)
{
    for (size_t i = 0; i < sizeof(s_buttons) / sizeof(s_buttons[0]); i++) {
        if (s_buttons[i].handle != NULL) {
            ESP_ERROR_CHECK(iot_button_delete(s_buttons[i].handle));
            s_buttons[i].handle = NULL;
        }
        if (s_buttons[i].queue != NULL) {
            vQueueDelete(s_buttons[i].queue);
            s_buttons[i].queue = NULL;
        }
    }
}

button_state_t acquire_button_state(gpio_num_t gpio_num)
{
    for (size_t i = 0; i < sizeof(s_buttons) / sizeof(s_buttons[0]); i++) {
        if (s_buttons[i].gpio != gpio_num || s_buttons[i].queue == NULL) {
            continue;
        }

        button_state_t state = BUTTON_STATE_NONE;
        if (xQueueReceive(s_buttons[i].queue, &state, 0) == pdTRUE) {
            return state;
        }
        return BUTTON_STATE_NONE;
    }
    return BUTTON_STATE_NONE;
}
