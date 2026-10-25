#ifndef __SEG_H__
#define __SEG_H__

#include <stdbool.h>
#include <stdint.h>

/* CH455G：SCL = GPIO7，SDA = GPIO8。 */
#define SEG_SCL_GPIO    7
#define SEG_SDA_GPIO    8

/* 原理图符号左侧标的是 DIG3。1：第 0 位是最左边。显示左右颠倒时改成 0。 */
#define SEG_LEFT_IS_DIG3  0

#define SEG_BLANK  0x00
#define SEG_MINUS  0x40
#define SEG_DP     0x80

void seg_init(void);
/* level 为 1～8，8 最亮。 */
void seg_set_brightness(uint8_t level);
/* index 0 是最左边一位。segments 的 bit0～bit6 对应 a～g，bit7 是小数点。 */
void seg_set_segments(uint8_t index, uint8_t segments);
void seg_show_number(int value);
void seg_clear(void);

#endif /* __SEG_H__ */
