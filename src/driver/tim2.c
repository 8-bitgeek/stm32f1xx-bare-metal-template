#include "tim2.h"


void tim2_init(void) {
    // 0. 开启时钟
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    // 1. 配置 PSC 预分频: 72 分频, 每 1/1 000 000 秒计数一次, 即 1us 一次
    TIM2->PSC = 72 - 1;
    // 2. 配置计数方向: 向上计数
    TIM2->CR1 &= ~TIM_CR1_DIR;
    // 3. 配置 ARR 计数器上限(计时功能不需要)
    TIM2->ARR = 0xFFFF;                 // 16 bit 寄存器

    // 打开计数功能
    TIM2->CR1 |= TIM_CR1_CEN;
}
