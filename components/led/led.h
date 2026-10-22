#ifndef __LED_H__
#define __LED_H__

#include <stdint.h>


/* 修改这里可以选择 PWM 输出引脚和频率。 */
#define PWM_GPIO             8
#define PWM_FREQ_HZ          1000
#define PWM_SPEED_MODE       LEDC_LOW_SPEED_MODE
#define PWM_TIMER            LEDC_TIMER_0
#define PWM_CHANNEL          LEDC_CHANNEL_0
#define PWM_DUTY_RESOLUTION  LEDC_TIMER_10_BIT
/* 当前 1 kHz、10 位配置下，单程渐变约 2 秒。 */
#define PWM_FADE_TIME_MS     2048
#define PWM_DARK_HOLD_MS     200

/* ESP32-C3 使用 10 位分辨率时，1024 对应 100%（低于硬件最大分辨率）。 */
#define PWM_FULL_DUTY        (1U << PWM_DUTY_RESOLUTION)

void pwm_init(void);
void pwm_fade_to(uint32_t target_duty);

#endif /* __LED_H__ */
