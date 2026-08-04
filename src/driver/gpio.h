#ifndef __GPIO_H__
#define __GPIO_H__

#include <stm32f1xx.h>

typedef enum {
    INPUT_ANA = 0b0000,             // 模拟输入
    INPUT_FLOAT = 0b0100,           // 浮空输入
    INPUT_UD = 0b1000,              // 上/下拉输入
    OUTPUT_PP_10M = 0b0001,
    OUTPUT_PP_2M = 0b0010,          // 推挽输出
    OUTPUT_PP_50M = 0b0011,
    OUTPUT_OD_10M = 0b0101,         // 开漏输出
    OUTPUT_OD_2M = 0b0110,
    OUTPUT_OD_50M = 0b0111,
    OUTPUT_AL_PP_10M = 0b1001,      // 复用推挽输出
    OUTPUT_AL_PP_2M = 0b1010,
    OUTPUT_AL_PP_50M = 0b1011,
    OUTPUT_AL_OD_10M = 0b1101,      // 复用开漏输出
    OUTPUT_AL_OD_2M = 0b1110,
    OUTPUT_AL_OD_50M = 0b1111,
} GPIO_MODE;

typedef struct {
    GPIO_TypeDef * gpio;
    uint8_t pin;
    GPIO_MODE mode;
    uint8_t up_down;                // 1 - 上拉; 0 - 下拉(仅上下拉输入时有效)
} gpio_t;

/**
  * @brief  初始化 GPIO 端口
  *
  * @retval 1 - 成功; 0 - 失败;
  */
uint8_t gpio_init(gpio_t gpio);

#endif
