#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "driver/gpio.h"

/* 按原理图：左键 GPIO5，右键 GPIO6。 */
#define BUTTON_LEFT    GPIO_NUM_5
#define BUTTON_RIGHT   GPIO_NUM_6

/* 按下时为高电平。启用内部下拉，松手时保持低电平。 */
#define BUTTON_ACTIVE_LEVEL    1
#define BUTTON_DISABLE_PULL    false
/* 按住超过该时间上报长按。单击会在松手后再等 BUTTON_SHORT_PRESS_MS 才确认，
 * 这段时间也是双击的最大间隔。 */
#define BUTTON_LONG_PRESS_MS   1000
#define BUTTON_SHORT_PRESS_MS  400

typedef enum {
    BUTTON_STATE_NONE = 0,
    BUTTON_STATE_SHORT_PRESS,   /* 短按（单击） */
    BUTTON_STATE_LONG_PRESS,    /* 长按 */
    BUTTON_STATE_DOUBLE_CLICK,  /* 双击 */
} button_state_t;

void button_init(void);
void button_deinit(void);

/* 取出并清除该按键最早一次尚未读取的手势。没有新事件时返回 BUTTON_STATE_NONE。 */
button_state_t acquire_button_state(gpio_num_t gpio_num);

#endif /* __BUTTON_H__ */
