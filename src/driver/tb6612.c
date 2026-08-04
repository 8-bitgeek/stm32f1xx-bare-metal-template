#include "tb6612.h"
#include "gpio.h"
#include "pwm.h"

/**
  * 初始化 pwm;
  * 初始化控制 tb6612 的 gpio 引脚; 
  */
void tb6612_init(void) {
    // 0. 初始化 PWM 模块
    pwm_init();

    gpio_t pin4 = {
        GPIOA, 4, OUTPUT_PP_50M, 1
    };
    gpio_t pin5 = {
        GPIOA, 5, OUTPUT_PP_50M, 1
    };
    gpio_init(pin4);
    gpio_init(pin5);
}

static tb6612_state_t tb6612_state = TB6612_SW_STOP;
static tb6612_dir_t tb6612_dir = TB6612_DIR_FORWARD;

/**
  * @brief  刷新 TB6612 方向
  */
static void refresh_dir(void) {
    if (tb6612_dir == TB6612_DIR_FORWARD) {
        GPIOA->ODR |= GPIO_ODR_ODR4;
        GPIOA->ODR &= ~GPIO_ODR_ODR5;
    } else if (tb6612_dir == TB6612_DIR_REVERSE) {
        GPIOA->ODR &= ~GPIO_ODR_ODR4;
        GPIOA->ODR |= GPIO_ODR_ODR5;
    }
}

/**
  * @brief  设置 TB6612 方向
  * @param  方向
  */
void tb6612_set_dir(tb6612_dir_t dir) {
    tb6612_dir = dir;
    // 如果正在运行, 要先停止
    if (tb6612_state == TB6612_SW_RUN) {
        pwm_stop();
        refresh_dir();
        pwm_start();
    } else {
        refresh_dir();
    }
}

/**
  * @brief  开启 PWM 输出实现开启驱动板输出
  */
void tb6612_start(void) {
    if (tb6612_state != TB6612_SW_RUN) {
        tb6612_state = TB6612_SW_RUN;
        pwm_start();
    }
}

/**
  * @brief  关闭 Pwm 输出实现关闭驱动板输出
  */
void tb6612_stop(void) {
    if (tb6612_state == TB6612_SW_RUN) {
        pwm_stop();
        tb6612_state = TB6612_SW_STOP;
    }
}

/**
  * @brief  获取当前驱动板的输出方向
  * @retval 当前驱动板输出方向
  */
tb6612_dir_t tb6612_get_dir(void) {
    return tb6612_dir;
}

/**
  * @brief  获取当前驱动板的运行状态
  * @retval 当前驱动板运行状态
  */
tb6612_state_t tb6612_get_state(void) {
    return tb6612_state;
}

/**
  * @brief  设置占空比
  * @param  duty - 占空比  
  */
void tb6612_set_duty(uint8_t duty) {
    pwm_set_duty_cycle(duty);
}

/**
  * @brief 获取当前占空比
  * @retval 当前占空比
  */
uint8_t tb6612_get_duty(void) {
    return pwm_get_duty();
}
