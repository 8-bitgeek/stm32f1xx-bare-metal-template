#include "pwm.h"
#include "gpio.h"

#define PWM_CCR1_MAX 100
#define PWM_CCR1_MIN 0

/**
  * 使用 TIM3 定时器, CH1 - GPIOA-6
  * 外部时钟 -> PSC 预分频 -> CNT 计数器 -> ARR 自动重装载 -> 比较/更新事件 -> 中断/DMA/输出
  */
void pwm_init(void) {
    // 1. 开启时钟
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // 配置 GPIOA PIN-6 工作模式: 复用推挽输出, 下拉
    gpio_t led_port = {
        GPIOA, 6, OUTPUT_AL_PP_50M, 0
    };
    gpio_init(led_port);

    // 人耳能听到的频率为 20 - 20kHz, 但是频率越高, 发热量越大
    // 2. 配置预分频值 720, 分频后频率为 100K, 1/100000s, 即 1 个计数同期为 10us, 100 次计数 1000us -> 1ms -> 1kHz
    // 2. 配置预分频值 72, 分频后频率为 1000M, 1/1000000s, 即 1 个计数同期为 1us, 100 次计数 100us -> 0.1ms -> 10kHz
    // 2. 配置预分频值 36, 分频后频率为 2000M, 1/2000000s, 即 1 个计数同期为 0.5us, 100 次计数 50 -> 0.05ms -> 20kHz
    TIM3->PSC = 36 - 1;
    // 3. 设置自动重载寄存器
    TIM3->ARR = 100 - 1;        // 100 次计算为一个 pwm 周期

    // 4. 配置计数方向(不配置也行, 默认向上计数)
    TIM3->CR1 &= ~TIM_CR1_DIR;

    // 5. 配置通道 1 初始占空比
    TIM3->CCR1 = 0;

    // 6. 配置通道 1 为输出模式(CC1S = 0b00)
    TIM3->CCMR1 &= ~TIM_CCMR1_CC1S;                 // CCMR: Capture/Compare Mode Register

    /*
     * 7. 配置通道 1 输出为 PWM 模式 1: OC1M[2:0] = 0b110
     *
     * TIM3 向上计数时, CNT 从 0 递增到 ARR:
     *   CNT < CCR1  时, OC1REF 输出有效电平(高电平)
     *   CNT >= CCR1 时, OC1REF 输出无效电平(低电平)
     */
    TIM3->CCMR1 |= TIM_CCMR1_OC1M;
    TIM3->CCMR1 &= ~TIM_CCMR1_OC1M_0;

    // 8. 使能输出; CCER - Capture/Compare Enable Register
    TIM3->CCER |= TIM_CCER_CC1E;                    // CC1E: Capture/Compare 1 output Enable
}

void pwm_start(void) {
    TIM3->CR1 |= TIM_CR1_CEN;
}

void pwm_stop(void) {
    TIM3->CR1 &= ~TIM_CR1_CEN;
}

void pwm_set_duty_cycle(uint16_t duty_cycle) {
    if (duty_cycle <= PWM_CCR1_MAX) {
        TIM3->CCR1 = duty_cycle;
    }
}

uint8_t pwm_get_duty() {
    return TIM3->CCR1;
}
