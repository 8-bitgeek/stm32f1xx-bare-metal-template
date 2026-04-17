#ifndef __OLED_H__
#define __OLED_H__

#include "i2c.h"
#include "oled-data.h"

// OLED 屏幕尺寸定义
#define OLED_WIDTH                      128
#define OLED_HEIGHT                     64

// 字体大小定义
typedef enum {
    F6X8 = 6,
    F8X16 = 8
} FONTSIZE;

/* is_filled 参数数值 */
#define OLED_UNFILLED			0
#define OLED_FILLED				1


// 常用命令宏定义
#define OLED_CMD_SET_CONTRAST           0x81
#define OLED_CMD_DISPLAY_ALL_ON_RESUME  0xA4
#define OLED_CMD_DISPLAY_ALL_ON         0xA5
#define OLED_CMD_NORMAL_DISPLAY         0xA6
#define OLED_CMD_INVERSE_DISPLAY        0xA7
#define OLED_CMD_DISPLAY_OFF            0xAE
#define OLED_CMD_DISPLAY_ON             0xAF
#define OLED_CMD_SET_PAGE_START         0xB0
#define OLED_CMD_SET_COLUMN_LOW         0x00
#define OLED_CMD_SET_COLUMN_HIGH        0x10
#define OLED_CMD_SET_START_LINE         0x40


// 初始化函数
void oled_init(void);
// 更新显存到屏幕
void oled_update(void);
void oled_clear(void);
void oled_clear_area(int16_t x, int16_t y, uint8_t width, uint8_t height);
void oled_scroll_up(uint8_t height);

// 显示指定数据
void oled_show_img(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t * img);
// 显示字符
void oled_show_char(uint8_t x, uint8_t y, char ch, FONTSIZE font_size);
// 显示字符串
void oled_show_string(uint16_t x, uint16_t y, char * str, FONTSIZE font_size);
void oled_show_string_wrap(uint16_t x, uint16_t y, char * str, FONTSIZE font_size);
void oled_printf(int16_t x, int16_t y, FONTSIZE font_size, char * format, ...);

uint32_t oled_pow(uint32_t x, uint32_t y);
uint8_t oled_pnpoly(uint8_t nvert, int16_t * vertx, int16_t * verty, int16_t testx, int16_t testy);
uint8_t oled_is_in_angle(int16_t x, int16_t y, int16_t start_angle, int16_t end_angle);
// 显示数字
void oled_show_num(int16_t x, int16_t y, uint32_t number, uint8_t length, uint8_t font_size) ;
void oled_show_signed_num(int16_t x, int16_t y, int32_t number, uint8_t length, FONTSIZE font_size);
void oled_show_hex_num(int16_t x, int16_t y, uint32_t number, uint8_t length, FONTSIZE font_size);
void oled_show_bin_num(int16_t X, int16_t Y, uint32_t number, uint8_t length, FONTSIZE font_size);
void oled_show_float_num(int16_t x, int16_t y, double number, uint8_t int_length, uint8_t fra_length, FONTSIZE font_size);

// 画点
void oled_draw_point(int16_t x, int16_t y);
uint8_t oled_get_point(int16_t x, int16_t y);
void oled_draw_line(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void oled_draw_rectangle(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t is_filled);
void oled_draw_triangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t is_filled);
void oled_draw_circle(int16_t X, int16_t Y, uint8_t radius, uint8_t is_filled);
void oled_draw_ellipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t is_filled);
void oled_draw_arc(int16_t X, int16_t Y, uint8_t radius, int16_t start_angle, int16_t end_angle, uint8_t is_filled);

// 翻转
void oled_reverse(void);
void oled_reverse_area(int16_t x, int16_t y, uint8_t width, uint8_t height);

// 测试全屏点亮
void oled_test_screen(void);

#endif
