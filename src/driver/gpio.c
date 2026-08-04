/**
 *     MODE: (低 2 bit)
 *         - 00: Input mode (reset state)
 *         - 01: Output mode, max speed 10MHz
 *         - 10: Output mode, max speed 2MHz
 *         - 11: Output mode, max speed 50MHz
 *     CNF: (高 2 bit)
 *         - Input Mode (Mode[1:0] = 00)
 *             - 00: Analog mode
 *             - 01: Floating input
 *             - 10: Input with pull-up / pull-down
 *             - 11: Reserved
 *         - Output Mode (Mode[1:0] > 00)
 *             - 00: General purpose output push-pull
 *             - 01: General purpose output Open-drain
 *             - 10: Alternate function output Push-pull
 *             - 11: Alternate function output Open-drain
 */
#include "gpio.h"

uint8_t gpio_init(gpio_t gpio) {
    uint8_t pos;
    if (gpio.gpio == GPIOA) {
        pos = RCC_APB2ENR_IOPAEN_Pos;
    } else if (gpio.gpio == GPIOB) {
        pos = RCC_APB2ENR_IOPBEN_Pos;
    } else if (gpio.gpio == GPIOC) {
        pos = RCC_APB2ENR_IOPCEN_Pos;
    } else if (gpio.gpio == GPIOD) {
        pos = RCC_APB2ENR_IOPDEN_Pos;
    } else {
        return 0;
    }
    // 1. 时钟使能
    RCC->APB2ENR |= 0x01 << pos;
    // 2. 配置 GPIO 工作模式
    GPIO_MODE mode = gpio.mode;
    if (gpio.pin >= 8) {
        // 先全部复位, 再配置
        gpio.gpio->CRH &= ~(0b1111 << ((gpio.pin - 8) * 4));
        gpio.gpio->CRH |= mode << ((gpio.pin - 8) * 4);
    } else {
        gpio.gpio->CRL &= ~(0b1111 << (gpio.pin * 4));
        gpio.gpio->CRL |= mode << (gpio.pin * 4);
    }
    // 3. 初始化上下拉
    if (gpio.mode == INPUT_UD) {
        if (gpio.up_down) {
            gpio.gpio->ODR |= 1 << gpio.pin;        // 上拉
        } else {
            gpio.gpio->ODR &= ~(1 << gpio.pin);     // 下拉
        }
    }

    return 1;
}
