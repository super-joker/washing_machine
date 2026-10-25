#include "seg.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"

#define SEG_BIT_US  5

/* 段码 bit0=a … bit6=g，与 CH455 的 SEG0～SEG6 一致。 */
static const uint8_t s_font[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F,
};

static const uint8_t s_dig_addr[4] = {0x68, 0x6A, 0x6C, 0x6E};

static uint8_t s_digits[4];
static uint8_t s_brightness = 8;

static void seg_delay(void)
{
    esp_rom_delay_us(SEG_BIT_US);
}

static void scl_set(int level)
{
    gpio_set_level(SEG_SCL_GPIO, level);
}

static void sda_set(int level)
{
    gpio_set_level(SEG_SDA_GPIO, level);
}

/* SCL 为高时 SDA 下降沿。 */
static void ch455_start(void)
{
    sda_set(1);
    scl_set(1);
    seg_delay();
    sda_set(0);
    seg_delay();
    scl_set(0);
    seg_delay();
}

/* SCL 为高时 SDA 上升沿。 */
static void ch455_stop(void)
{
    sda_set(0);
    seg_delay();
    scl_set(1);
    seg_delay();
    sda_set(1);
    seg_delay();
}

/* 高位在前。第 9 位应答固定为 1，由主机发出。 */
static void ch455_write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        scl_set(0);
        sda_set((data & 0x80) != 0);
        seg_delay();
        scl_set(1);
        seg_delay();
        data <<= 1;
    }

    scl_set(0);
    sda_set(1);
    seg_delay();
    scl_set(1);
    seg_delay();
    scl_set(0);
    seg_delay();
}

static void ch455_write(uint8_t byte1, uint8_t byte2)
{
    ch455_start();
    ch455_write_byte(byte1);
    ch455_write_byte(byte2);
    ch455_stop();
}

static int physical_digit(uint8_t left_index)
{
#if SEG_LEFT_IS_DIG3
    return 3 - (int)left_index;
#else
    return (int)left_index;
#endif
}

static void flush_digit(uint8_t left_index)
{
    int dig = physical_digit(left_index);
    ch455_write(s_dig_addr[dig], s_digits[left_index]);
}

static void apply_system(void)
{
    uint8_t intens = (s_brightness >= 8) ? 0 : (uint8_t)(s_brightness << 4);
    /* bit7 关闭键盘扫描，bit0 打开显示，8 段模式以保留小数点。 */
    ch455_write(0x48, intens | 0x81);
}

void seg_init(void)
{
    const gpio_config_t scl_conf = {
        .pin_bit_mask = 1ULL << SEG_SCL_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const gpio_config_t sda_conf = {
        .pin_bit_mask = 1ULL << SEG_SDA_GPIO,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&scl_conf);
    gpio_config(&sda_conf);
    scl_set(1);
    sda_set(1);

    apply_system();
    seg_clear();
}

void seg_set_brightness(uint8_t level)
{
    if (level < 1) {
        level = 1;
    } else if (level > 8) {
        level = 8;
    }
    s_brightness = level;
    apply_system();
}

void seg_set_segments(uint8_t index, uint8_t segments)
{
    if (index >= 4) {
        return;
    }
    s_digits[index] = segments;
    flush_digit(index);
}

void seg_show_number(int value)
{
    uint8_t segs[4] = {SEG_BLANK, SEG_BLANK, SEG_BLANK, SEG_BLANK};

    if (value > 9999 || value < -999) {
        for (int i = 0; i < 4; i++) {
            segs[i] = SEG_MINUS;
        }
    } else {
        bool negative = value < 0;
        int remain = negative ? -value : value;
        int pos = 3;

        do {
            segs[pos] = s_font[remain % 10];
            remain /= 10;
            pos--;
        } while (remain > 0 && pos >= 0);

        if (negative && pos >= 0) {
            segs[pos] = SEG_MINUS;
        }
    }

    for (uint8_t i = 0; i < 4; i++) {
        seg_set_segments(i, segs[i]);
    }
}

void seg_show_mmss(uint8_t minutes, uint8_t seconds)
{
    if (minutes > 99) {
        minutes = 99;
    }
    if (seconds > 59) {
        seconds = 59;
    }
    seg_set_segments(0, s_font[minutes / 10]);
    seg_set_segments(1, s_font[minutes % 10]);
    seg_set_segments(2, s_font[seconds / 10]);
    seg_set_segments(3, s_font[seconds % 10]);
}

void seg_show_zeros(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        seg_set_segments(i, s_font[0]);
    }
}

void seg_show_dots(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        seg_set_segments(i, SEG_DP);
    }
}

void seg_clear(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        seg_set_segments(i, SEG_BLANK);
    }
}
