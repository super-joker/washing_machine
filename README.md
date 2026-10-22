| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 | ESP32-S31 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- | --------- |

# Washing machine

  小洗衣机控制板

## PWM 呼吸灯（ESP32-C3）

当前入口为 `main/main.c`，通过 LEDC 硬件 PWM 和渐变功能实现呼吸灯：

- 输出引脚：GPIO8，可修改 `PWM_GPIO`。
- 频率：1 kHz（周期 1 ms），可修改 `PWM_FREQ_HZ`。
- 分辨率：10 位，当前配置下占空比计数 0～1024 对应 0%～100%。
- 效果：约 2 秒渐亮 → 约 2 秒渐暗 → 熄灭停顿 200 ms，循环呼吸。
- 调整速度：修改 `PWM_FADE_TIME_MS`（默认 2048 ms，单程渐变时间）；修改 `PWM_DARK_HOLD_MS` 调整熄灭后的停顿时间。硬件渐变时间受 PWM 频率和占空比步进量化影响，其他配置下实际时间可能与设置值不同。

在 ESP-IDF 终端执行（串口号按实际连接修改）：

```sh
idf.py build
idf.py -p COM4 flash monitor
```

示波器或逻辑分析仪信号端接 GPIO8，地线接开发板 GND。PWM 周期约 1 ms，高电平宽度随呼吸过程逐渐增加、再逐渐减小。

普通 LED 接线：GPIO8 → 330 Ω 电阻 → LED 阳极，LED 阴极 → GND。如果板载灯是 WS2812 等可寻址 RGB 灯，不能用本例的普通 PWM 直接调光。如果 GPIO8 已连接其他电路，请将 `PWM_GPIO` 改为板上空闲且支持输出的引脚。测试时使用示波器或普通 LED 作为负载，不要直接用 GPIO 驱动电机。

若只需要固定 50% 占空比，可将 `app_main()` 中的 `while (1)` 循环替换为：

```c
ESP_ERROR_CHECK(ledc_set_duty(PWM_SPEED_MODE, PWM_CHANNEL, PWM_FULL_DUTY / 2));
ESP_ERROR_CHECK(ledc_update_duty(PWM_SPEED_MODE, PWM_CHANNEL));
```

`ledc_fade_func_install()` 安装渐变服务，`ledc_set_fade_with_time()` 设置目标占空比和时间，`ledc_fade_start(..., LEDC_FADE_WAIT_DONE)` 启动并等待本次渐变结束，再开始反向渐变。

PWM 由硬件持续输出，无需通过循环翻转 GPIO。原来的 `CONFIG_BLINK_*` 设置不再控制本例。
