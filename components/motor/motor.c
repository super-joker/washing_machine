#include "motor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

/* 半桥：PMOS 为高时这一端接电池，NMOS 用 PWM 接到地。 */
#define MOTOR_PMOS1_GPIO    GPIO_NUM_0
#define MOTOR_NMOS1_GPIO    GPIO_NUM_1
#define MOTOR_PMOS2_GPIO    GPIO_NUM_2
#define MOTOR_NMOS2_GPIO    GPIO_NUM_3

#define MOTOR_SPEED_MODE      LEDC_LOW_SPEED_MODE
#define MOTOR_TIMER           LEDC_TIMER_1
#define MOTOR_CH_NMOS1        LEDC_CHANNEL_1
#define MOTOR_CH_NMOS2        LEDC_CHANNEL_2
#define MOTOR_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define MOTOR_PWM_HZ          20000
#define MOTOR_DUTY_MAX        ((1U << MOTOR_DUTY_RESOLUTION) - 1U)
/* 换向时上下管都关掉的时间，避开直通。 */
#define MOTOR_DEAD_TIME_MS    30

static motor_dir_t s_dir = MOTOR_STOP;
static uint8_t s_speed;
static SemaphoreHandle_t s_lock;

static void set_low_side(ledc_channel_t channel, uint8_t percent)
{
    uint32_t duty = ((uint32_t)MOTOR_DUTY_MAX * percent) / 100U;
    ESP_ERROR_CHECK(ledc_set_duty(MOTOR_SPEED_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(MOTOR_SPEED_MODE, channel));
}

static void all_off(void)
{
    set_low_side(MOTOR_CH_NMOS1, 0);
    set_low_side(MOTOR_CH_NMOS2, 0);
    gpio_set_level(MOTOR_PMOS1_GPIO, 0);
    gpio_set_level(MOTOR_PMOS2_GPIO, 0);
}

static void apply_run(motor_dir_t dir, uint8_t speed)
{
    if (dir == MOTOR_FORWARD) {
        set_low_side(MOTOR_CH_NMOS1, 0);
        gpio_set_level(MOTOR_PMOS2_GPIO, 0);
        gpio_set_level(MOTOR_PMOS1_GPIO, 1);
        set_low_side(MOTOR_CH_NMOS2, speed);
    } else {
        set_low_side(MOTOR_CH_NMOS2, 0);
        gpio_set_level(MOTOR_PMOS1_GPIO, 0);
        gpio_set_level(MOTOR_PMOS2_GPIO, 1);
        set_low_side(MOTOR_CH_NMOS1, speed);
    }
}

void motor_init(void)
{
    const gpio_config_t high_side = {
        .pin_bit_mask = (1ULL << MOTOR_PMOS1_GPIO) | (1ULL << MOTOR_PMOS2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const ledc_timer_config_t timer = {
        .speed_mode = MOTOR_SPEED_MODE,
        .duty_resolution = MOTOR_DUTY_RESOLUTION,
        .timer_num = MOTOR_TIMER,
        .freq_hz = MOTOR_PWM_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    const ledc_channel_config_t nmos1 = {
        .gpio_num = MOTOR_NMOS1_GPIO,
        .speed_mode = MOTOR_SPEED_MODE,
        .channel = MOTOR_CH_NMOS1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    const ledc_channel_config_t nmos2 = {
        .gpio_num = MOTOR_NMOS2_GPIO,
        .speed_mode = MOTOR_SPEED_MODE,
        .channel = MOTOR_CH_NMOS2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_TIMER,
        .duty = 0,
        .hpoint = 0,
    };

    /* 先把高边拉低。GPIO2 在复位时内部上拉，会误打开电机 2 的高边。 */
    ESP_ERROR_CHECK(gpio_config(&high_side));
    gpio_set_level(MOTOR_PMOS1_GPIO, 0);
    gpio_set_level(MOTOR_PMOS2_GPIO, 0);

    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    ESP_ERROR_CHECK(ledc_channel_config(&nmos1));
    ESP_ERROR_CHECK(ledc_channel_config(&nmos2));

    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
    }
    s_dir = MOTOR_STOP;
    s_speed = 0;
}

void motor_set(motor_dir_t dir, uint8_t speed_percent)
{
    if (s_lock == NULL) {
        return;
    }
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    if (speed_percent == 0 || dir == MOTOR_STOP) {
        dir = MOTOR_STOP;
        speed_percent = 0;
    } else if (dir != MOTOR_FORWARD && dir != MOTOR_REVERSE) {
        dir = MOTOR_STOP;
        speed_percent = 0;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);

    if (dir == s_dir && speed_percent == s_speed) {
        xSemaphoreGive(s_lock);
        return;
    }

    if (dir == s_dir) {
        if (dir == MOTOR_FORWARD) {
            set_low_side(MOTOR_CH_NMOS2, speed_percent);
        } else if (dir == MOTOR_REVERSE) {
            set_low_side(MOTOR_CH_NMOS1, speed_percent);
        }
        s_speed = speed_percent;
        xSemaphoreGive(s_lock);
        return;
    }

    all_off();
    if (s_dir != MOTOR_STOP && dir != MOTOR_STOP) {
        vTaskDelay(pdMS_TO_TICKS(MOTOR_DEAD_TIME_MS));
    }

    if (dir != MOTOR_STOP) {
        apply_run(dir, speed_percent);
    }
    s_dir = dir;
    s_speed = speed_percent;
    xSemaphoreGive(s_lock);
}
