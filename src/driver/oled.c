#include "oled.h"
#include "i2c.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/* SSD1306 的通信格式: [S] [从机地址+写] [A] [控制字节] [A] [数据字节] [A] [数据字节] [A] ... [P] */

/**
  * 数据存储格式:
  * 纵向 8 点, 高位在下, 先从左到右, 再从上到下
  * 每一个 bit 对应一个像素点
  * 
  *      B0 B0                  B0 B0
  *      B1 B1                  B1 B1
  *      B2 B2                  B2 B2
  *      B3 B3  ------------->  B3 B3 --
  *      B4 B4                  B4 B4  |
  *      B5 B5                  B5 B5  |
  *      B6 B6                  B6 B6  |
  *      B7 B7                  B7 B7  |
  *                                    |
  *  -----------------------------------
  *  |   
  *  |   B0 B0                  B0 B0
  *  |   B1 B1                  B1 B1
  *  |   B2 B2                  B2 B2
  *  --> B3 B3  ------------->  B3 B3
  *      B4 B4                  B4 B4
  *      B5 B5                  B5 B5
  *      B6 B6                  B6 B6
  *      B7 B7                  B7 B7
  * 
  * 坐标轴定义:
  * 左上角为 (0, 0) 点
  * 横向向右为 X 轴, 取值范围: 0~127
  * 纵向向下为 Y 轴, 取值范围: 0~63
  * 
  *       0             X 轴          127 
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  * Y 轴 |
  *      |
  *      |
  *      |
  *   63 |
  *      v
  * 
  */

#define OLED_7BIT_ADDR      0x3C                // SSD1306 的 7 位地址 (不是 0x78)

typedef enum {
    C_CMD = 0x00,                               // 命令控制字节
    C_DATA = 0x40,                              // 数据控制字节 
} CTRL_BYTE;

static uint8_t current_x;                       // 当前光标所在列
static uint8_t current_y;                       // 当前光标所在行

/* 显存缓冲区 */
uint8_t oled_display_buffer[OLED_HEIGHT / 8][OLED_WIDTH];

/**
  * @brief  写数据到 OLED SSD1306 RAM
  * @param  ctrl_byte 控制字节: @see CTRL_BYTE 
  * @param  byte 要发送的数据指针
  * @param  len 要发送的数据长度
  * @retval 1 - 发送成功; 0 - 发送失败;  
  */
static uint8_t oled_write_bytes(CTRL_BYTE ctrl_byte, uint8_t * byte, uint8_t len) {
    if (i2c_start() == 0) {
        goto error;
    }

    if (i2c_send_addr(OLED_7BIT_ADDR, 0) == 0) {
        goto error;
    }

    // Send control byte
    // 0b00000000 - Co = 0, D/C = 0
    if (i2c_send_data(ctrl_byte) == 0) {
        goto error;
    }
    for (uint8_t i = 0; i < len; i++) {
        if (i2c_send_data(byte[i]) == 0) {
            goto error;
        }
    }
    i2c_stop();
    return 1;

error: 
    i2c_stop();
    return 0;
}

/********************* 驱动函数 *********************/

/**
  * @brief 向 OLED 发送命令序列
  * @param cmd 命令字节
  * @retval 1 - 成功; 0 - 失败
  */
static uint8_t oled_write_cmd(uint8_t cmd) {
    return oled_write_bytes(C_CMD, &cmd, 1);
}

/**
  * @brief 向 OLED 发送多个命令
  */
static uint8_t oled_write_cmds(uint8_t * cmds, uint8_t len) {
    return oled_write_bytes(C_CMD, cmds, len);
}

/**
  * @brief 向 OLED 发送多个数据
  */
static uint8_t oled_write_data(uint8_t * data, uint16_t len) {
    return oled_write_bytes(C_DATA, data, len);
}

/**
  * @brief  设置光标位置
  * @param  x - 列号 (0-127); y - 行号 (0 - 7);
  */
uint8_t oled_set_cursor(uint8_t x, uint8_t y) {
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT / 8) {
        return 0;
    }

    // Set page start address
    oled_write_cmd(0xB0 | y);
    // Set column start address
    oled_write_cmd(0x00 | (x & 0x0F));                              // lower column address
    oled_write_cmd(0x10 | ((x & 0xF0) >> 4));                       // higher column address (0x10~0x17)

    return 1;
}

/***************************** 驱动函数结束 *****************************/


/******************************* 配置函数 *******************************/
/**
  * @brief OLED 初始化 (使用官方推荐的初始化序列)
  */
void oled_init(void) {
    // 初始化 I2C
    i2c_init();

    // SSD1306 官方推荐初始化序列
    uint8_t init_cmds[] = {
        0xAE,       // 关闭显示
        
        0xD5,       // 设置显示时钟分频比/振荡器频率
        0x80,       // 0x00~0xFF
        
        0xA8,       // 设置多路复用率
        0x3F,       // 0x0E~0x3F (64行)
        
        0xD3,       // 设置显示偏移
        0x00,       // 0x00~0x7F (无偏移)
        
        0x40,       // 设置显示开始行 (0x40~0x7F)
        
        0xA1,       // 设置左右方向 (0xA1正常, 0xA0左右反置)
        
        0xC8,       // 设置上下方向 (0xC8正常, 0xC0上下反置)
        
        0xDA,       // 设置COM引脚硬件配置
        0x12,       // 交替配置
        
        0x81,       // 设置对比度
        0xCF,       // 0x00~0xFF
        
        0xD9,       // 设置预充电周期
        0xF1,       // 建议值
        
        0xDB,       // 设置VCOMH取消选择级别
        0x30,       // 注意：你用的是0x30，之前版本是0x40
        
        0xA4,       // 设置整个显示打开/关闭 (0xA4恢复显示, 0xA5忽略显存全亮)
        
        0xA6,       // 设置正常/反色显示 (0xA6正常, 0xA7反色)
        
        0x8D,       // 设置充电泵
        0x14,       // 使能充电泵
        
        0xAF        // 开启显示
    };
    
    // 发送初始化命令序列
    oled_write_cmds(init_cmds, sizeof(init_cmds));
    
    // 清屏并更新
    oled_update();
}

void oled_test_screen(void) {
    // 直接操作显存并更新
    for (uint8_t page = 0; page < 8; page++) {
        for (uint8_t col = 0; col < 128; col++) {
            oled_display_buffer[page][col] = 0xFF;  // 全亮
        }
    }
    oled_update();
}

/***************************** 配置函数结束 *****************************/


/******************************* 工具函数 *******************************/

/**
  * @brief  次方函数
  * @param  x 底数
  * @param  y 指数
  * @retval 等于 x 的 y 次方
  */
uint32_t oled_pow(uint32_t x, uint32_t y) {
	uint32_t res = 1;	                                // 结果默认为 1
	while (y --) {			                            // 累乘 y 次
		res *= x;		                                // 每次把 x 累乘到结果上
	}
	return res;
}

/**
  * @brief  判断指定点是否在指定多边形内部
  * @param  nvert 多边形的顶点数
  * @param  vertx verty 包含多边形顶点的x和y坐标的数组
  * @param  testx testy 测试点的X和y坐标
  * @retval 指定点是否在指定多边形内部, 1: 在内部, 0: 不在内部
  */
uint8_t oled_pnpoly(uint8_t nvert, int16_t * vertx, int16_t * verty, int16_t testx, int16_t testy) {
	int16_t i, j, c = 0;
	
	/* 此算法由 W. Randolph Franklin 提出*/
	/* 参考链接: https://wrfranklin.org/Research/Short_Notes/pnpoly.html */
	for (i = 0, j = nvert - 1; i < nvert; j = i++) {
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i])) {
			c = !c;
		}
	}
	return c;
}

/**
  * @brief  判断指定点是否在指定角度内部
  * @param  X Y 指定点的坐标
  * @param  StartAngle EndAngle 起始角度和终止角度, 范围: -180~180
  *           水平向右为0度, 水平向左为180度或-180度, 下方为正数, 上方为负数, 顺时针旋转
  * @retval 指定点是否在指定角度内部: 1 - 在内部; 0 - 不在内部;
  */
uint8_t oled_is_in_angle(int16_t x, int16_t y, int16_t start_angle, int16_t end_angle) {
	int16_t point_angle;
	point_angle = atan2(y, x) / 3.14 * 180;	        // 计算指定点的弧度, 并转换为角度表示
	if (start_angle < end_angle) {	                // 起始角度小于终止角度的情况
		/* 如果指定角度在起始终止角度之间, 则判定指定点在指定角度 */
		if (point_angle >= start_angle && point_angle <= end_angle) {
			return 1;
		}
	} else {			                            // 起始角度大于于终止角度的情况
		/* 如果指定角度大于起始角度或者小于终止角度, 则判定指定点在指定角度 */
		if (point_angle >= start_angle || point_angle <= end_angle) {
			return 1;
		}
	}
	return 0;		                                // 不满足以上条件, 则判断判定指定点不在指定角度
}


/***************************** 工具函数结束 *****************************/


/******************************* 功能函数 *******************************/

/**
  * @brief  将 OLED 显存数组更新到 OLED 屏幕
  * @note   所有的显示函数, 都只是对 OLED 显存数组进行读写
  *           随后调用 oled_update 函数或 oled_update_area 函数
  *           才会将显存数组的数据发送到 OLED 硬件, 进行显示
  *           故调用显示函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_update(void) {
	/* 遍历每一页 */
	for (uint8_t i = 0; i < 8; i++) {
		/* 设置光标位置为每一页的第一列 */
		oled_set_cursor(0, i);
		/* 连续写入 128 个数据, 将显存数组的数据写入到 OLED 硬件*/
		oled_write_data(oled_display_buffer[i], 128);
	}
}

/**
  * @brief  将 OLED 显存数组部分更新到 OLED 屏幕
  * @param  x 指定区域左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定区域左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  width 指定区域的宽度, 范围: 0~128
  * @param  height 指定区域的高度, 范围: 0~64
  * @note   此函数会至少更新参数指定的区域
  *           如果更新区域 y 轴只包含部分页, 则同一页的剩余部分会跟随一起更新
  * @note   所有的显示函数, 都只是对 OLED 显存数组进行读写
  *           随后调用 oled_update 函数或 oled_update_area 函数
  *           才会将显存数组的数据发送到 OLED 硬件, 进行显示
  *           故调用显示函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void OLED_UpdateArea(int16_t x, int16_t y, uint8_t width, uint8_t height) {
	int16_t i;
	int16_t page, page1;
	
	/* 负数坐标在计算页地址时需要加一个偏移 */
	/* (y + height - 1) / 8 + 1 的目的是 (y + height) / 8 并向上取整 */
	page = y / 8;
	page1 = (y + height - 1) / 8 + 1;
	if (y < 0) {
		page -= 1;
		page1 -= 1;
	}
	
	/* 遍历指定区域涉及的相关页 */
	for (i = page; i < page1; i++) {
		if (x >= 0 && x <= 127 && i >= 0 && i <= 7) {		    // 超出屏幕的内容不显示
			/* 设置光标位置为相关页的指定列 */
			oled_set_cursor(i, x);
			/* 连续写入 width 个数据, 将显存数组的数据写入到 OLED 硬件 */
			oled_write_data(&oled_display_buffer[i][x], width);
		}
	}
}

/**
  * @brief  将 OLED 显存数组全部清零
  * @note   调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_clear(void) {
	for (uint8_t i = 0; i < 8; i++) {				// 遍历 8 页
		for (uint8_t j = 0; j < 128; j++) {		    // 遍历 128 列
			oled_display_buffer[i][j] = 0x00;	    // 将显存数组数据全部清零
		}
	}
}

/**
  * @brief  将 OLED 显存数组部分清零
  * @param  x - 指定区域左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y - 指定区域左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  width - 指定区域的宽度, 范围: 0~128
  * @param  height - 指定区域的高度, 范围: 0~64
  * @note   调用此函数后, 只是更新了显存数组, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_clear_area(int16_t x, int16_t y, uint8_t width, uint8_t height) {
	for (int16_t i = y; i < y + height; i++) {                          // 遍历指定页
		for (int16_t j = x; j < x + width; j++) {	                    // 遍历指定列
			if (j >= 0 && j <= 127 && i >=0 && i <= 63) {				// 超出屏幕的内容不显示
				oled_display_buffer[i / 8][j] &= ~(0x01 << (i % 8));	// 将显存数组指定数据清零
			}
		}
	}
}

/**
  * @brief	将 OLED 显存数组全部取反
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_reverse(void) {
	for (uint8_t i = 0; i < 8; i++) {				// 遍历 8 页
		for (uint8_t j = 0; j < 128; j++) {		    // 遍历 128 列
			oled_display_buffer[i][j] ^= 0xFF;	    // 将显存数组数据全部取反
		}
	}
}

/**
  * @brief  图像上移指定高度 (像素)
  * @param  height 上移的高度
  */
void oled_scroll_up(uint8_t height) {
    // 高度大于整个屏幕时, 直接清屏操作
    if (height >= OLED_HEIGHT) {
        oled_clear();
    } else {                                            // 否则需要滚屏操作
        uint8_t shift_pages, shift_bits;
        shift_pages = height / 8;                       // 要移动的 page 数
        shift_bits = height % 8;                        // 要移动的 bit 数

        // 先移页操作
        for (uint8_t col = 0; col < OLED_WIDTH; col++) {
            for (uint8_t row = 0; row < (OLED_HEIGHT / 8 - shift_pages); row++) {
                oled_display_buffer[row][col] = oled_display_buffer[row + shift_pages][col];
            }
            for (uint8_t row = OLED_HEIGHT / 8 - shift_pages; row < OLED_HEIGHT / 8; row++) {           // 底部的 shift_pages 页填充空白
                oled_display_buffer[row][col] = 0x00;
            }
        }
        // 再移位, 遍历每行的所有列
        for (uint8_t col = 0; col < OLED_WIDTH; col++) {
            for (uint8_t row = 0; row < (OLED_HEIGHT / 8 - 1); row++) {
                // 需要右移来实现滚屏, 数据模型参考上方数据格式 (高位在下)
                // 将当前行右移 height 位, 最高位 "或" 上下一行的低 height 位
                uint8_t next_page_bit = oled_display_buffer[row + 1][col] << (8 - shift_bits);
                oled_display_buffer[row][col] = (oled_display_buffer[row][col] >> shift_bits) | next_page_bit;
            }
            oled_display_buffer[7][col] >>= shift_bits;
        }
    }
    oled_update();
}
	
/**
  * @brief	将OLED显存数组部分取反
  * @param	x 指定区域左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	y 指定区域左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	width 指定区域的宽度, 范围: 0~128
  * @param	height 指定区域的高度, 范围: 0~64
  * 返 回 值: 无
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_reverse_area(int16_t x, int16_t y, uint8_t width, uint8_t height) {
	for (int16_t i = y; i < y + height; i++) {		                // 遍历指定页
		for (int16_t j = x; j < x + width; j++) {	                // 遍历指定列
			if (j >= 0 && j <= 127 && i >=0 && i <= 63) {			// 超出屏幕的内容不显示
				oled_display_buffer[i / 8][j] ^= 0x01 << (i % 8);	// 将显存数组指定数据取反
			}
		}
	}
}

/**
  * @brief  将指定图像显示到指定位置
  * @param  x - 列坐标
  * @param  y - 行坐标
  * @param  width - 图像宽度
  * @param  height - 图像高度
  * @param  img - 图像数据指针
  */
void oled_show_img(int16_t x, int16_t y, uint8_t width, uint8_t height, const uint8_t * img) {
    int16_t page, shift;

    // 将图像所在区域清空
    oled_clear_area(x, y, width, height);

    // 遍历图像占用的所有行 (page)
    for (uint8_t row = 0; row < (height - 1) / 8 + 1; row++) {
        // 一次外层循环将遍历图像在当前行 (page) 占用的所有列
        for (uint8_t col = 0; col < width; col++) {
            if (x + col >= 0 && x + col <= 127) {                       // 超出屏幕的部分不显示
                // 计算图像起始坐标 (x, y) 对应的页地址和页内偏移
                page = y / 8;                                           // 页地址 (0~7, 可能为负数）
                shift = y % 8;                                          // 页内偏移 (C 语言对负数取模可能为负)

                // 修正负数坐标: 将负偏移转换成正偏移, 同时页号减 1
                // 原理: 显示位置向上移动 n 行, 相当于从上一个页的底部开始显示
                if (y < 0) {
                    page -= 1;                                          // 回退到上一个页
                    shift += 8;                                         // 偏移加 8, 使 shift 范围变为 0~15(实际有效的是 0~7)
                }

                // 显示图像在当前页的内容
                if (page + row >= 0 && page + row <= 7) {               // 超出屏幕的部分不显示
                    oled_display_buffer[page + row][x + col] |= img[row * width + col] << shift;
                }

                // 显示图像在下一页的内容
                if (page + row + 1 >= 0 && page + row + 1 <= 7) {
                    oled_display_buffer[page + row + 1][x + col] |= img[row * width + col] >> (8 - shift);
                }
            }
        }
    }
}

/**
  * @brief  OLED 显示一个字符
  * @pram   x 指定字符左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @pram   y 指定字符左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @pram   ch 指定要显示的字符, 范围: ASCII 码可见字符
  * @pram   font_size 指定字体大小
  *           范围: F8X16		宽 8 像素, 高 16 像素
  *                 F6X8		宽 6 像素, 高 8 像素
  * @note   调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_char(uint8_t x, uint8_t y, char ch, FONTSIZE font_size) {
    if (font_size == F8X16) {
        oled_show_img(x, y, 8, 16, oled_f8x16[ch - ' ']);
    } else if (font_size == F6X8) {
        oled_show_img(x, y, 6, 8, oled_f6x8[ch - ' ']);
    }
}

/**
  * @brief  OLED 显示字符串 (支持 ASCII 码和中文混合写入)
  * @pram   x 指定字符串左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @pram   y 指定字符串左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @pram   str 指定要显示的字符串, 范围: ASCII 码可见字符或中文字符组成的字符串
  * @pram   font_size 指定字体大小
  *           范围: F8X16		宽 8 像素, 高 16 像素
  *                 F6X8		宽 6 像素, 高 8 像素
  * @note   显示的中文字符需要在 oled-data.c 里的 oled_cf16x16 数组定义
  *           未找到指定中文字符时, 会显示默认图形 (一个方框, 内部一个问号)
  *           当字体大小为 F8X16 时, 中文字符以 16*16 点阵正常显示
  *           当字体大小为 F6X8 时, 中文字符以 6*8 点阵显示 '?'
  * @note   调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_string(uint16_t x, uint16_t y, char * str, FONTSIZE font_size) {
    uint16_t i = 0;
    uint8_t char_len;                                               // 字符占的字节数
    char single_char[5];                                            // 用于保存一个中文字符
    uint16_t x_off = 0, p_idx;

    while (str[i] != '\0') {
        // 提取 UTF-8 字符串中的一个字符, 转存到 single_char 子字符串中
        // 判断 UTF-8 编码第一个字节的标志位
        if ((str[i] & 0x80) == 0x00) {                              // 第一个字节为 0b0xxxxxxx 则表示
            // 最高位为 0, 则表示字符占 1 字节
            char_len = 1;
            single_char[0] = str[i++];                              // 将第一个字节保存到 0, 并指向下一个字节
            single_char[1] = '\0';                                  // 添加字符串结束标志
        } else if ((str[i] & 0xE0) == 0xC0) {                       // 第一个字节为 0b110xxxxx & 0b11100000 = 0b11000000
            char_len = 2;
            single_char[0] = str[i++];
            single_char[1] = str[i++];
            single_char[2] = '\0';
        } else if ((str[i] & 0xF0) == 0xE0) {                       // 第一个字节为 0b1110xxxx
            char_len = 3;
            single_char[0] = str[i++];
            single_char[1] = str[i++];
            single_char[2] = str[i++];
            single_char[3] = '\0';
        } else if ((str[i] & 0xF8) == 0xF0) {                       // 第一个字节为 0b11110xxx
            char_len = 4;
            single_char[0] = str[i++];
            single_char[1] = str[i++];
            single_char[2] = str[i++];
            single_char[3] = str[i++];
            single_char[4] = '\0';
        } else {                                                    // 异常情况, 忽略当前字节, i 指向下一个字节, 继续判断
            i++;
            continue;
        }

        // 显示提取到的中文字符 (在 UTF-8 下, 一个中文字符至少占 2-4 字节)
		if (char_len == 1) {                                        // 如果是单字节字符 (非中文)
			/* 非中文直接使用 oled_show_char 显示此字符 */
			oled_show_char(x + x_off, y, single_char[0], font_size);
			x_off += font_size;
		} else {					                                // 否则, 即多字节字符
			/* 遍历整个字模库, 从字模库中寻找此字符的数据 */
			/* 如果找到最后一个字符 (定义为空字符串), 则表示字符未在字模库定义, 停止寻找 */
			for (p_idx = 0; strcmp(oled_cf16x16[p_idx].index, "") != 0; p_idx++) {
				/* 找到匹配的字符 */
				if (strcmp(oled_cf16x16[p_idx].index, single_char) == 0) {
                    // 跳出循环, 此时 p_idx 的值为指定字符的索引
					break;		
				}
			}
			if (font_size == F8X16) {	                        // 给定字体为 8*16 点阵
				/* 将字模库 oled_cf16x16 的指定数据以 16*16 的图像格式显示 */
				oled_show_img(x + x_off, y, 16, 16, oled_cf16x16[p_idx].data);
				x_off += 16;
			} else if (font_size == F6X8) {	                    // 给定字体为 6*8 点阵
				/* 空间不足, 此位置显示'?' */
				oled_show_char(x + x_off, y, '?', F6X8);
				x_off += F6X8;
			}
		}
    }
}

/**
  * @brief  OLED 显示字符串（自动换行版本）
  * @param  x 起始横坐标
  * @param  y 起始纵坐标
  * @param  str 字符串
  * @param  font_size 字体大小
  * @note   自动处理换行, 遇到 '\n' 或超出右边界自动换行
  */
void oled_show_string_wrap(uint16_t x, uint16_t y, char * str, FONTSIZE font_size) {
    uint16_t i = 0;
    uint8_t char_len;
    char single_char[5];
    uint16_t current_x = x;
    uint16_t current_y = y;
    uint16_t line_height = (font_size == F8X16) ? 16 : 8;
    uint16_t start_x = x;
    
    while (str[i] != '\0') {
        // 提取字符（代码同上）
        if ((str[i] & 0x80) == 0x00) {
            char_len = 1;
            single_char[0] = str[i++];
            single_char[1] = '\0';
        } else if ((str[i] & 0xE0) == 0xC0) {
            char_len = 2;
            single_char[0] = str[i++];
            single_char[1] = str[i++];
            single_char[2] = '\0';
        } else if ((str[i] & 0xF0) == 0xE0) {
            char_len = 3;
            single_char[0] = str[i++];
            single_char[1] = str[i++];
            single_char[2] = str[i++];
            single_char[3] = '\0';
        } else {
            i++;
            continue;
        }
        
        // 处理换行符
        if (char_len == 1 && single_char[0] == '\n') {
            current_x = start_x;
            current_y += line_height;
            continue;
        }
        
        // 获取字符宽度
        uint8_t char_width = (char_len == 1) ? font_size : 
                            ((font_size == F8X16) ? 16 : F6X8);
        
        // 检查是否需要换行
        if (current_x + char_width > OLED_WIDTH) {
            current_x = start_x;
            current_y += line_height;
            
            // 超出屏幕底部则停止
            if (current_y + line_height > OLED_HEIGHT) {
                break;
            }
        }
        
        // 显示字符（这里简化了，实际需要完整的中文显示代码）
        if (char_len == 1) {
            oled_show_char(current_x, current_y, single_char[0], font_size);
        } else {
            // 中文显示（复用原代码逻辑）
            uint16_t p_idx;
            for (p_idx = 0; strcmp(oled_cf16x16[p_idx].index, "") != 0; p_idx++) {
                if (strcmp(oled_cf16x16[p_idx].index, single_char) == 0) {
                    break;
                }
            }
            if (font_size == F8X16) {
                oled_show_img(current_x, current_y, 16, 16, oled_cf16x16[p_idx].data);
            } else {
                oled_show_char(current_x, current_y, '?', F6X8);
            }
        }
        
        current_x += char_width;
    }
}

/**
  * @brief  OLED 显示数字 (十进制, 正整数)
  * @param  x 指定数字左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定数字左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  number 指定要显示的数字, 范围: 0~4294967295
  * @param  length 指定数字的长度, 范围: 0~10
  * @param  font_size 指定字体大小
  *           范围: F8X16		宽 8 像素, 高 16 像素
  *                 F6X8		宽 6 像素, 高 8 像素
  * @note   调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_num(int16_t x, int16_t y, uint32_t number, uint8_t length, uint8_t font_size) {
	for (uint8_t i = 0; i < length; i++) {		// 遍历数字的每一位
		/* 调用 oled_show_char 函数, 依次显示每个数字 */
		/* Number / oled_pow(10, Length - i - 1) % 10 可以十进制提取数字的每一位 */
		/* + '0' 可将数字转换为字符格式 */
		oled_show_char(x + i * font_size, y, number / oled_pow(10, length - i - 1) % 10 + '0', font_size);
	}
}

/**
  * @brief	OLED 显示有符号数字 (十进制, 整数)
  * @param	x 指定数字左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	y 指定数字左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	number 指定要显示的数字, 范围: -2147483648~2147483647
  * @param	length 指定数字的长度, 范围: 0~10
  * @param	font_size 指定字体大小
  *           范围: F8X16		宽 8 像素, 高 16 像素
  *                 F6X8		宽 6 像素, 高 8 像素
  * 返 回 值: 无
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_signed_num(int16_t x, int16_t y, int32_t number, uint8_t length, FONTSIZE font_size) {
	uint8_t i;
	uint32_t number1;
	
	if (number >= 0) {						    // 数字大于等于 0
		oled_show_char(x, y, '+', font_size);	// 显示 + 号
		number1 = number;					    // number1 直接等于 number
	} else {								    // 数字小于 0
		oled_show_char(x, y, '-', font_size);	// 显示-号
		number1 = -number;					    // number1 等于 number 取负
	}
	
	for (i = 0; i < length; i++) {			    // 遍历数字的每一位
		/* 调用 oled_show_char 函数, 依次显示每个数字 */
		/* number1 / oled_pow(10, length - i - 1) % 10 可以十进制提取数字的每一位 */
		/* + '0' 可将数字转换为字符格式 */
		oled_show_char(x + (i + 1) * font_size, y, number1 / oled_pow(10, length - i - 1) % 10 + '0', font_size);
	}
}

/**
  * @brief  OLED 显示二进制数字（二进制, 正整数）
  * @param  x 指定数字左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定数字左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  number 指定要显示的数字, 范围: 0x00000000~0xFFFFFFFF
  * @param  length 指定数字的长度, 范围: 0~16
  * @param  font_size 指定字体大小
  *           范围: F8X16		宽 8 像素, 高 16 像素
  *                 F6X8		宽 6 像素, 高 8 像素
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_bin_num(int16_t X, int16_t Y, uint32_t number, uint8_t length, FONTSIZE font_size) {
	uint8_t i;
	for (i = 0; i < length; i++) {		                //遍历数字的每一位	
		/* 调用 oled_show_char 函数, 依次显示每个数字*/
		/* number / oled_pow(2, length - i - 1) % 2 可以二进制提取数字的每一位 */
		/* + '0' 可将数字转换为字符格式 */
		oled_show_char(X + i * font_size, Y, number / oled_pow(2, length - i - 1) % 2 + '0', font_size);
	}
}

/**
  * @brief  OLED 显示浮点数字 (十进制, 小数)
  * @param  x 指定数字左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定数字左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  number 指定要显示的数字, 范围: -4294967295.0~4294967295.0
  * @param  int_length 指定数字的整数位长度, 范围: 0~10
  * @param  fra_length 指定数字的小数位长度, 范围: 0~9, 小数进行四舍五入显示
  * @param  font_size 指定字体大小
  *           范围: OLED_8X16		宽 8 像素, 高 16 像素
  *                 OLED_6X8		宽 6 像素, 高 8 像素
  * 返 回 值: 无
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_float_num(int16_t x, int16_t y, double number, uint8_t int_length, uint8_t fra_length, FONTSIZE font_size) {
	uint32_t pow_num, int_num, fra_num;
	
	if (number >= 0) {						    // 数字大于等于 0
		oled_show_char(x, y, '+', font_size);	// 显示 + 号
	} else {								    // 数字小于 0
		oled_show_char(x, y, '-', font_size);	// 显示 - 号
		number = -number;					    // number 取负
	}
	
	/* 提取整数部分和小数部分 */
	int_num = number;						    // 直接赋值给整型变量, 提取整数
	number -= int_num;						    // 将 number 的整数减掉, 防止之后将小数乘到整数时因数过大造成错误
	pow_num = oled_pow(10, fra_length);		    // 根据指定小数的位数, 确定乘数
	fra_num = round(number * pow_num);		    // 将小数乘到整数, 同时四舍五入, 避免显示误差
	int_num += fra_num / pow_num;				// 若四舍五入造成了进位, 则需要再加给整数
	
	/* 显示整数部分 */
	oled_show_num(x + font_size, y, int_num, int_length, font_size);
	
	/* 显示小数点 */
	oled_show_char(x + (int_length + 1) * font_size, y, '.', font_size);
	
	/* 显示小数部分 */
	oled_show_num(x + (int_length + 2) * font_size, y, fra_num, fra_length, font_size);
}

/**
  * @brief  OLED 显示十六进制数字(十六进制, 正整数)
  * @param  x 指定数字左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定数字左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  number 指定要显示的数字, 范围: 0x00000000~0xFFFFFFFF
  * @param  length 指定数字的长度, 范围: 0~8
  * @param  font_size 指定字体大小
  *           范围: OLED_8X16		宽 8 像素, 高 16 像素
  *                 OLED_6X8		宽 6 像素, 高 8 像素
  * @note   调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_show_hex_num(int16_t x, int16_t y, uint32_t number, uint8_t length, FONTSIZE font_size) {
	uint8_t i, single_num;
	for (i = 0; i < length; i++) {		                // 遍历数字的每一位
		/* 以十六进制提取数字的每一位 */
		single_num = number / oled_pow(16, length - i - 1) % 16;
		
		if (single_num < 10) {			                // 单个数字小于 10
			/* 调用 oled_show_char 函数, 显示此数字 */
			/* + '0' 可将数字转换为字符格式 */
			oled_show_char(x + i * font_size, y, single_num + '0', font_size);
		} else {							            // 单个数字大于10
			/* 调用 oled_show_char 函数, 显示此数字 */
			/* + 'A' 可将数字转换为从 A 开始的十六进制字符 */
			oled_show_char(x + i * font_size, y, single_num - 10 + 'A', font_size);
		}
	}
}

/**
  * @brief  OLED 使用 printf 函数打印格式化字符串 (支持 ASCII 码和中文混合写入)
  * @param  x - 指定格式化字符串左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y - 指定格式化字符串左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param  font_Size 指定字体大小
  *           范围：F8X16		宽 8 像素, 高 16 像素
  *                 F6X8		宽 6 像素, 高 8 像素
  * @param  format 指定要显示的格式化字符串, 范围: ASCII 码可见字符或中文字符组成的字符串
  * @param  ... 格式化字符串参数列表
  * @note   显示的中文字符需要在 OLED_Data.c 里的 OLED_CF16x16 数组定义
  *           未找到指定中文字符时，会显示默认图形（一个方框，内部一个问号）
  *           当字体大小为 OLED_8X16 时, 中文字符以 16*16 点阵正常显示
  *           当字体大小为 OLED_6X8 时. 中文字符以 6*8 点阵显示 '?'
  * @note   调用此函数后，要想真正地呈现在屏幕上，还需调用更新函数
  */
void oled_printf(int16_t x, int16_t y, FONTSIZE font_size, char * format, ...) {
	char str[256];						                // 定义字符数组
	va_list arg;							            // 定义可变参数列表数据类型的变量 arg
	va_start(arg, format);					            // 从 format 开始, 接收参数列表到 arg 变量
	vsnprintf(str, sizeof(str), format, arg);		    // 使用 vsnprintf 打印格式化字符串和参数列表到字符数组中
	va_end(arg);							            // 结束变量 arg
	oled_show_string_wrap(x, y, str, font_size);             // OLED 显示字符数组 (字符串)
}

/**
  * @brief  OLED 在指定位置画一个点
  * @param  x 指定点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @note   调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_point(int16_t x, int16_t y) {
	if (x >= 0 && x <= 127 && y >=0 && y <= 63) {		// 超出屏幕的内容不显示
		/* 将显存数组指定位置的一个 bit 数据置 1 */
		oled_display_buffer[y / 8][x] |= 0x01 << (y % 8);
	}
}

/**
  * @brief  OLED 获取指定位置点的值
  * @param  x 指定点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param  y 指定点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @retval 指定位置点是否处于点亮状态: 1 - 点亮; 0 - 熄灭;
  */
uint8_t oled_get_point(int16_t x, int16_t y) {
	if (x >= 0 && x <= 127 && y >=0 && y <= 63) {		// 超出屏幕的内容不读取
		/* 判断指定位置的数据 */
		if (oled_display_buffer[y / 8][x] & 0x01 << (y % 8)) {
			return 1;	                                // 为 1, 返回 1
		}
	}
    // 否则, 返回0
	return 0;		                        
}


/**
  * @brief	OLED 画线
  * @param	X0 指定一个端点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y0 指定一个端点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	X1 指定另一个端点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y1 指定另一个端点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_line(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1) {
	int16_t x, y, dx, dy, d, incrE, incrNE, temp;
	int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
	uint8_t yflag = 0, xyflag = 0;
	
	if (y0 == y1) {		                        // 横线单独处理
		/* 0 号点 X 坐标大于 1 号点 X 坐标, 则交换两点 X 坐标 */
		if (x0 > x1) {
            temp = x0; x0 = x1; x1 = temp;
        }
		
		/*遍历X坐标*/
		for (x = x0; x <= x1; x ++) {
			oled_draw_point(x, y0);	            // 依次画点
		}
	} else if (x0 == x1) {	                    // 竖线单独处理
		/* 0 号点 Y 坐标大于 1 号点 Y 坐标, 则交换两点 Y 坐标 */
		if (y0 > y1) {
            temp = y0; y0 = y1; y1 = temp;
        }
		
		/* 遍历 Y 坐标 */
		for (y = y0; y <= y1; y ++) {
			oled_draw_point(x0, y);	            // 依次画点
		}
	} else {				                    // 斜线
		/* 使用 Bresenham 算法画直线, 可以避免耗时的浮点运算, 效率更高 */
		/* 参考文档: https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
		/* 参考教程: https://www.bilibili.com/video/BV1364y1d7Lo*/
		if (x0 > x1) {	                        // 0 号点 X 坐标大于 1 号点 X 坐标
			/* 交换两点坐标 */
			/* 交换后不影响画线, 但是画线方向由第一, 二, 三, 四象限变为第一, 四象限 */
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}
		
		if (y0 > y1) {	                        // 0 号点 Y 坐标大于 1 号点 Y 坐标
			/* 将 Y 坐标取负 */
			/* 取负后影响画线, 但是画线方向由第一, 四象限变为第一象限 */
			y0 = -y0;
			y1 = -y1;
			
			/* 置标志位 yflag, 记住当前变换, 在后续实际画线时, 再将坐标换回来 */
			yflag = 1;
		}
		
		if (y1 - y0 > x1 - x0) {	            // 画线斜率大于 1
			/* 将 X 坐标与 Y 坐标互换 */
			/* 互换后影响画线, 但是画线方向由第一象限 0~90 度范围变为第一象限 0~45 度范围 */
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;
			
			/* 置标志位 xyflag, 记住当前变换, 在后续实际画线时, 再将坐标换回来 */
			xyflag = 1;
		}
		
		/* 以下为 Bresenham 算法画直线 */
		/* 算法要求, 画线方向必须为第一象限 0~45 度范围 */
		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);
		d = 2 * dy - dx;
		x = x0;
		y = y0;
		
		/* 画起始点, 同时判断标志位, 将坐标换回来 */
		if (yflag && xyflag) {
            oled_draw_point(y, -x);
        } else if (yflag) {
            oled_draw_point(x, -y);
        } else if (xyflag){
            oled_draw_point(y, x);
        } else {
            oled_draw_point(x, y);
        }
		
		while (x < x1) {	                    // 遍历X轴的每个点
			x ++;
			if (d < 0) {		                // 下一个点在当前点东方
				d += incrE;
			} else {			                // 下一个点在当前点东北方
				y ++;
				d += incrNE;
			}
			
			/* 画每一个点, 同时判断标志位, 将坐标换回来 */
			if (yflag && xyflag) {
                oled_draw_point(y, -x);
            } else if (yflag) {
                oled_draw_point(x, -y);
            } else if (xyflag) {
                oled_draw_point(y, x);
            } else {
                oled_draw_point(x, y);
            }
		}	
	}
}

/**
  * @brief	OLED 矩形
  * @param	x 指定矩形左上角的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	y 指定矩形左上角的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	width 指定矩形的宽度, 范围: 0~128
  * @param	height 指定矩形的高度, 范围: 0~64
  * @param	is_filled 指定矩形是否填充
  *           范围: OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_rectangle(int16_t x, int16_t y, uint8_t width, uint8_t height, uint8_t is_filled) {
	int16_t i, j;
	if (!is_filled) {		                    // 指定矩形不填充
		/* 遍历上下 X 坐标, 画矩形上下两条线 */
		for (i = x; i < x + width; i++) {
			oled_draw_point(i, y);
			oled_draw_point(i, y + height - 1);
		}
		/* 遍历左右 Y 坐标, 画矩形左右两条线 */
		for (i = y; i < y + height; i++) {
			oled_draw_point(x, i);
			oled_draw_point(x + width - 1, i);
		}
	} else {				                    // 指定矩形填充
		/* 遍历 X 坐标 */
		for (i = x; i < x + width; i++) {
			/* 遍历 Y 坐标 */
			for (j = y; j < y + height; j++) {
				/* 在指定区域画点, 填充满矩形 */
				oled_draw_point(i, j);
			}
		}
	}
}

/**
  * @brief	OLED三角形
  * @param	X0 指定第一个端点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y0 指定第一个端点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	X1 指定第二个端点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y1 指定第二个端点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	X2 指定第三个端点的横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y2 指定第三个端点的纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	is_filled 指定三角形是否填充
  *           范围: OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * 返 回 值: 无
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_triangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t is_filled) {
	int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
	int16_t i, j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};
	
	if (!is_filled) {			                // 指定三角形不填充
		/* 调用画线函数, 将三个点用直线连接 */
		oled_draw_line(X0, Y0, X1, Y1);
		oled_draw_line(X0, Y0, X2, Y2);
		oled_draw_line(X1, Y1, X2, Y2);
	} else {					                // 指定三角形填充
		/* 找到三个点最小的 X, Y 坐标 */
		if (X1 < minx) {minx = X1;}
		if (X2 < minx) {minx = X2;}
		if (Y1 < miny) {miny = Y1;}
		if (Y2 < miny) {miny = Y2;}
		
		/* 找到三个点最大的 X, Y 坐标 */
		if (X1 > maxx) {maxx = X1;}
		if (X2 > maxx) {maxx = X2;}
		if (Y1 > maxy) {maxy = Y1;}
		if (Y2 > maxy) {maxy = Y2;}
		
		/* 最小最大坐标之间的矩形为可能需要填充的区域 */
		/* 遍历此区域中所有的点 */
		/* 遍历 X 坐标 */		
		for (i = minx; i <= maxx; i++) {
			/* 遍历 Y 坐标 */	
			for (j = miny; j <= maxy; j++) {
				/* 调用 OLED_pnpoly, 判断指定点是否在指定三角形之中 */
				/* 如果在, 则画点, 如果不在, 则不做处理 */
				if (oled_pnpoly(3, vx, vy, i, j)) {
                    oled_draw_point(i, j);
                }
			}
		}
	}
}

/**
  * @brief	OLED 画圆
  * @param	X 指定圆的圆心横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y 指定圆的圆心纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	radius 指定圆的半径, 范围: 0~255
  * @param	is_filled 指定圆是否填充
  *           范围: OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_circle(int16_t X, int16_t Y, uint8_t radius, uint8_t is_filled) {
	int16_t x, y, d, i;
	
	/* 使用 Bresenham 算法画圆, 可以避免耗时的浮点运算, 效率更高 */
	/* 参考文档: https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf */
	/* 参考教程: https://www.bilibili.com/video/BV1VM4y1u7wJ */
	
	d = 1 - radius;
	x = 0;
	y = radius;
	
	/* 画每个八分之一圆弧的起始点 */
	oled_draw_point(X + x, Y + y);
	oled_draw_point(X - x, Y - y);
	oled_draw_point(X + y, Y + x);
	oled_draw_point(X - y, Y - x);
	
	if (is_filled) {		                // 指定圆填充
		/* 遍历起始点 Y 坐标 */
		for (i = -y; i < y; i++) {
			/* 在指定区域画点, 填充部分圆 */
			oled_draw_point(X, Y + i);
		}
	}
	
	while (x < y) {		                    // 遍历 X 轴的每个点
		x ++;
		if (d < 0) {		                // 下一个点在当前点东方
			d += 2 * x + 1;
		} else {			                // 下一个点在当前点东南方
			y --;
			d += 2 * (x - y) + 1;
		}
		
		/* 画每个八分之一圆弧的点 */
		oled_draw_point(X + x, Y + y);
		oled_draw_point(X + y, Y + x);
		oled_draw_point(X - x, Y - y);
		oled_draw_point(X - y, Y - x);
		oled_draw_point(X + x, Y - y);
		oled_draw_point(X + y, Y - x);
		oled_draw_point(X - x, Y + y);
		oled_draw_point(X - y, Y + x);
		
		if (is_filled) {	                // 指定圆填充
			/* 遍历中间部分 */
			for (i = -y; i < y; i++) {
				/* 在指定区域画点, 填充部分圆 */
				oled_draw_point(X + x, Y + i);
				oled_draw_point(X - x, Y + i);
			}
			
			/* 遍历两侧部分 */
			for (i = -x; i < x; i++) {
				/* 在指定区域画点, 填充部分圆 */
				oled_draw_point(X - y, Y + i);
				oled_draw_point(X + y, Y + i);
			}
		}
	}
}

/**
  * @brief	OLED 画椭圆
  * @param	X 指定椭圆的圆心横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y 指定椭圆的圆心纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	A 指定椭圆的横向半轴长度, 范围: 0~255
  * @param	B 指定椭圆的纵向半轴长度, 范围: 0~255
  * @param	is_filled 指定椭圆是否填充
  *           范围: OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_ellipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t is_filled) {
	int16_t x, y, j;
	int16_t a = A, b = B;
	float d1, d2;
	
	/* 使用 Bresenham 算法画椭圆, 可以避免部分耗时的浮点运算, 效率更高 */
	/* 参考链接: https://blog.csdn.net/myf_666/article/details/128167392 */
	
	x = 0;
	y = b;
	d1 = b * b + a * a * (-b + 0.5);
	
	if (is_filled) {	                    //指定椭圆填充
		/* 遍历起始点 Y 坐标 */
		for (j = -y; j < y; j++) {
			/* 在指定区域画点, 填充部分椭圆 */
			oled_draw_point(X, Y + j);
			oled_draw_point(X, Y + j);
		}
	}
	
	/* 画椭圆弧的起始点 */
	oled_draw_point(X + x, Y + y);
	oled_draw_point(X - x, Y - y);
	oled_draw_point(X - x, Y + y);
	oled_draw_point(X + x, Y - y);
	
	/* 画椭圆中间部分 */
	while (b * b * (x + 1) < a * a * (y - 0.5)) {
		if (d1 <= 0) {		                // 下一个点在当前点东方
			d1 += b * b * (2 * x + 3);
		} else {				            // 下一个点在当前点东南方
			d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
			y --;
		}
		x ++;
		
		if (is_filled) {                    // 指定椭圆填充
			/* 遍历中间部分 */
			for (j = -y; j < y; j++) {
				/* 在指定区域画点, 填充部分椭圆 */
				oled_draw_point(X + x, Y + j);
				oled_draw_point(X - x, Y + j);
			}
		}
		
		/* 画椭圆中间部分圆弧 */
		oled_draw_point(X + x, Y + y);
		oled_draw_point(X - x, Y - y);
		oled_draw_point(X - x, Y + y);
		oled_draw_point(X + x, Y - y);
	}
	
	/* 画椭圆两侧部分 */
	d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;
	
	while (y > 0) {
		if (d2 <= 0) {		                // 下一个点在当前点东方
			d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
			x ++;
		} else {				            // 下一个点在当前点东南方
			d2 += a * a * (-2 * y + 3);
		}
		y --;
		
		if (is_filled) {	                // 指定椭圆填充
			/* 遍历两侧部分 */
			for (j = -y; j < y; j++) {
				/* 在指定区域画点, 填充部分椭圆 */
				oled_draw_point(X + x, Y + j);
				oled_draw_point(X - x, Y + j);
			}
		}
		
		/* 画椭圆两侧部分圆弧 */
		oled_draw_point(X + x, Y + y);
		oled_draw_point(X - x, Y - y);
		oled_draw_point(X - x, Y + y);
		oled_draw_point(X + x, Y - y);
	}
}

/**
  * @brief	OLED 画圆弧
  * @param	X 指定圆弧的圆心横坐标, 范围: -32768~32767, 屏幕区域: 0~127
  * @param	Y 指定圆弧的圆心纵坐标, 范围: -32768~32767, 屏幕区域: 0~63
  * @param	radius 指定圆弧的半径, 范围: 0~255
  * @param	start_angle 指定圆弧的起始角度, 范围: -180~180
  *           水平向右为0度, 水平向左为180度或-180度, 下方为正数, 上方为负数, 顺时针旋转
  * @param	end_angle 指定圆弧的终止角度, 范围: -180~180
  *           水平向右为0度, 水平向左为180度或-180度, 下方为正数, 上方为负数, 顺时针旋转
  * @param	is_filled 指定圆弧是否填充, 填充后为扇形
  *           范围: OLED_UNFILLED		不填充
  *                 OLED_FILLED			填充
  * @note	调用此函数后, 要想真正地呈现在屏幕上, 还需调用更新函数
  */
void oled_draw_arc(int16_t X, int16_t Y, uint8_t radius, int16_t start_angle, int16_t end_angle, uint8_t is_filled) {
	int16_t x, y, d, j;
	
	/* 此函数借用 Bresenham 算法画圆的方法 */
	
	d = 1 - radius;
	x = 0;
	y = radius;
	
	/* 在画圆的每个点时, 判断指定点是否在指定角度内, 在, 则画点, 不在, 则不做处理 */
	if (oled_is_in_angle(x, y, start_angle, end_angle))	    {oled_draw_point(X + x, Y + y);}
	if (oled_is_in_angle(-x, -y, start_angle, end_angle))   {oled_draw_point(X - x, Y - y);}
	if (oled_is_in_angle(y, x, start_angle, end_angle))     {oled_draw_point(X + y, Y + x);}
	if (oled_is_in_angle(-y, -x, start_angle, end_angle))   {oled_draw_point(X - y, Y - x);}
	
	if (is_filled) {	                //指定圆弧填充
		/* 遍历起始点 Y 坐标 */
		for (j = -y; j < y; j++) {
			/*在填充圆的每个点时, 判断指定点是否在指定角度内, 在, 则画点, 不在, 则不做处理*/
			if (oled_is_in_angle(0, j, start_angle, end_angle)) {oled_draw_point(X, Y + j);}
		}
	}
	
	while (x < y) {		                // 遍历 X 轴的每个点
		x ++;
		if (d < 0) {		            // 下一个点在当前点东方
			d += 2 * x + 1;
		} else {			            // 下一个点在当前点东南方
			y --;
			d += 2 * (x - y) + 1;
		}
		
		/* 在画圆的每个点时, 判断指定点是否在指定角度内, 在, 则画点, 不在, 则不做处理 */
		if (oled_is_in_angle(x, y, start_angle, end_angle)) {oled_draw_point(X + x, Y + y);}
		if (oled_is_in_angle(y, x, start_angle, end_angle)) {oled_draw_point(X + y, Y + x);}
		if (oled_is_in_angle(-x, -y, start_angle, end_angle)) {oled_draw_point(X - x, Y - y);}
		if (oled_is_in_angle(-y, -x, start_angle, end_angle)) {oled_draw_point(X - y, Y - x);}
		if (oled_is_in_angle(x, -y, start_angle, end_angle)) {oled_draw_point(X + x, Y - y);}
		if (oled_is_in_angle(y, -x, start_angle, end_angle)) {oled_draw_point(X + y, Y - x);}
		if (oled_is_in_angle(-x, y, start_angle, end_angle)) {oled_draw_point(X - x, Y + y);}
		if (oled_is_in_angle(-y, x, start_angle, end_angle)) {oled_draw_point(X - y, Y + x);}
		
		if (is_filled) {	            // 指定圆弧填充
			/* 遍历中间部分 */
			for (j = -y; j < y; j++) {
				/*在填充圆的每个点时, 判断指定点是否在指定角度内, 在, 则画点, 不在, 则不做处理*/
				if (oled_is_in_angle(x, j, start_angle, end_angle)) {oled_draw_point(X + x, Y + j);}
				if (oled_is_in_angle(-x, j, start_angle, end_angle)) {oled_draw_point(X - x, Y + j);}
			}
			
			/* 遍历两侧部分 */
			for (j = -x; j < x; j++) {
				/* 在填充圆的每个点时, 判断指定点是否在指定角度内, 在, 则画点, 不在, 则不做处理 */
				if (oled_is_in_angle(-y, j, start_angle, end_angle)) {oled_draw_point(X - y, Y + j);}
				if (oled_is_in_angle(y, j, start_angle, end_angle)) {oled_draw_point(X + y, Y + j);}
			}
		}
	}
}

/***************************** 功能函数结束 *****************************/
