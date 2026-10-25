#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <stdint.h>

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_REVERSE,
} motor_dir_t;

void motor_init(void);

/* speed_percent 为 0～100。0 或 MOTOR_STOP 都会停转。
 * 换向时内部先关断上下管，停一段死区后再接通。从任务里调用。 */
void motor_set(motor_dir_t dir, uint8_t speed_percent);

#endif /* __MOTOR_H__ */
