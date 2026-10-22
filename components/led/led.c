#include "led.h"
#include "driver/ledc.h"


// static const char *TAG = "breathing_led";


void pwm_init(void)
{
    /* 先配置定时器：决定 PWM 的频率和占空比分辨率。 */
    const ledc_timer_config_t timer_config = {
        .speed_mode = PWM_SPEED_MODE,
        .duty_resolution = PWM_DUTY_RESOLUTION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    /* 再配置通道：将定时器的 PWM 信号输出到指定 GPIO，初始为低电平。 */
    const ledc_channel_config_t channel_config = {
        .gpio_num = PWM_GPIO,
        .speed_mode = PWM_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

    /* 安装硬件渐变服务，只需初始化一次。 */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
}



void pwm_fade_to(uint32_t target_duty)
{
    ESP_ERROR_CHECK(ledc_set_fade_with_time(PWM_SPEED_MODE, PWM_CHANNEL,
                                           target_duty, PWM_FADE_TIME_MS));
    /* 阻塞当前任务直到渐变结束，期间硬件自动改变占空比。 */
    ESP_ERROR_CHECK(ledc_fade_start(PWM_SPEED_MODE, PWM_CHANNEL, LEDC_FADE_WAIT_DONE));
}

