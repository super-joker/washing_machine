#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "button.h"
#include "motor.h"
#include "seg.h"

#include <stdint.h>

static const char *TAG = "main";

#define APP_BOOT_DELAY_MS  2000
#define APP_SPIN_MS        3500
#define APP_REST_MS        2000
#define APP_ROUND_MS       (8U * 60U * 1000U)

typedef enum {
    MODE_OFF = 0,
    MODE_WAIT_START,
    MODE_RUN,
    MODE_PAUSE,
    MODE_ROUND_END,
} app_mode_t;

typedef enum {
    PHASE_FORWARD = 0,
    PHASE_REST_AFTER_FORWARD,
    PHASE_REVERSE,
    PHASE_REST_AFTER_REVERSE,
} run_phase_t;

static app_mode_t s_mode = MODE_OFF;
static run_phase_t s_phase = PHASE_FORWARD;
static uint32_t s_wait_ms;
static uint32_t s_run_ms;
static uint32_t s_phase_ms;
static uint32_t s_shown_sec = UINT32_MAX;

static void show_countdown(void)
{
    uint32_t remain_ms = (s_run_ms >= APP_ROUND_MS) ? 0 : (APP_ROUND_MS - s_run_ms);
    uint32_t remain_sec = remain_ms / 1000U;

    if (remain_sec == s_shown_sec) {
        return;
    }
    s_shown_sec = remain_sec;
    seg_show_mmss((uint8_t)(remain_sec / 60U), (uint8_t)(remain_sec % 60U));
}

static uint32_t phase_limit(run_phase_t phase)
{
    if (phase == PHASE_FORWARD || phase == PHASE_REVERSE) {
        return APP_SPIN_MS;
    }
    return APP_REST_MS;
}

static void drive_phase(void)
{
    if (s_phase == PHASE_FORWARD) {
        motor_set(MOTOR_FORWARD, 100);
    } else if (s_phase == PHASE_REVERSE) {
        motor_set(MOTOR_REVERSE, 100);
    } else {
        motor_set(MOTOR_STOP, 0);
    }
}

static void begin_phase(run_phase_t phase)
{
    s_phase = phase;
    s_phase_ms = 0;
    drive_phase();
    show_countdown();
}

static void power_off(void)
{
    motor_set(MOTOR_STOP, 0);
    seg_clear();
    s_mode = MODE_OFF;
    s_wait_ms = 0;
    s_run_ms = 0;
    s_phase_ms = 0;
    s_shown_sec = UINT32_MAX;
    ESP_LOGI(TAG, "关机");
}

static void power_on(void)
{
    motor_set(MOTOR_STOP, 0);
    s_mode = MODE_WAIT_START;
    s_wait_ms = 0;
    s_run_ms = 0;
    s_phase_ms = 0;
    s_shown_sec = UINT32_MAX;
    show_countdown();
    ESP_LOGI(TAG, "开机，2 秒后开始转动");
}

static void enter_round_end(void)
{
    motor_set(MOTOR_STOP, 0);
    seg_show_dots();
    s_mode = MODE_ROUND_END;
    s_shown_sec = UINT32_MAX;
    ESP_LOGI(TAG, "8 分钟到，暂停");
}

static void start_round(void)
{
    s_mode = MODE_RUN;
    s_run_ms = 0;
    begin_phase(PHASE_FORWARD);
    ESP_LOGI(TAG, "开始新的 8 分钟");
}

static void pause_run(void)
{
    motor_set(MOTOR_STOP, 0);
    seg_show_dots();
    s_mode = MODE_PAUSE;
    s_shown_sec = UINT32_MAX;
    ESP_LOGI(TAG, "暂停");
}

static void resume_run(void)
{
    s_mode = MODE_RUN;
    if (s_phase == PHASE_REST_AFTER_FORWARD) {
        begin_phase(PHASE_REVERSE);
    } else if (s_phase == PHASE_REST_AFTER_REVERSE) {
        begin_phase(PHASE_FORWARD);
    } else {
        begin_phase(s_phase);
    }
    ESP_LOGI(TAG, "继续");
}

static void advance_wait(uint32_t elapsed_ms)
{
    s_wait_ms += elapsed_ms;
    if (s_wait_ms >= APP_BOOT_DELAY_MS) {
        start_round();
    }
}

static run_phase_t next_phase(run_phase_t phase)
{
    switch (phase) {
    case PHASE_FORWARD:
        return PHASE_REST_AFTER_FORWARD;
    case PHASE_REST_AFTER_FORWARD:
        return PHASE_REVERSE;
    case PHASE_REVERSE:
        return PHASE_REST_AFTER_REVERSE;
    default:
        return PHASE_FORWARD;
    }
}

static void advance_run(uint32_t elapsed_ms)
{
    s_run_ms += elapsed_ms;
    if (s_run_ms >= APP_ROUND_MS) {
        enter_round_end();
        return;
    }

    s_phase_ms += elapsed_ms;
    while (s_phase_ms >= phase_limit(s_phase)) {
        s_phase_ms -= phase_limit(s_phase);
        s_phase = next_phase(s_phase);
        drive_phase();
    }
    show_countdown();
}

static void on_long_press(void)
{
    if (s_mode == MODE_OFF) {
        power_on();
    } else {
        power_off();
    }
}

static void on_short_press(void)
{
    if (s_mode == MODE_OFF || s_mode == MODE_WAIT_START) {
        return;
    }
    if (s_mode == MODE_RUN) {
        pause_run();
    } else if (s_mode == MODE_PAUSE) {
        resume_run();
    } else if (s_mode == MODE_ROUND_END) {
        start_round();
    }
}

static void handle_buttons(void)
{
    button_state_t state;

    while ((state = acquire_button_state(BUTTON_LEFT)) != BUTTON_STATE_NONE) {
        if (state == BUTTON_STATE_LONG_PRESS) {
            on_long_press();
        } else if (state == BUTTON_STATE_SHORT_PRESS) {
            on_short_press();
        }
    }

    while (acquire_button_state(BUTTON_RIGHT) != BUTTON_STATE_NONE) {
    }
}

void app_main(void)
{
    TickType_t last_tick;

    button_init();
    motor_init();
    seg_init();
    ESP_LOGI(TAG, "长按左键 2 秒开机");

    last_tick = xTaskGetTickCount();
    while (1) {
        TickType_t now = xTaskGetTickCount();
        uint32_t elapsed_ms = (uint32_t)pdTICKS_TO_MS(now - last_tick);
        app_mode_t mode_before;

        last_tick = now;
        mode_before = s_mode;
        handle_buttons();

        if (s_mode == MODE_WAIT_START && mode_before == MODE_WAIT_START) {
            advance_wait(elapsed_ms);
        } else if (s_mode == MODE_RUN && mode_before == MODE_RUN) {
            advance_run(elapsed_ms);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
