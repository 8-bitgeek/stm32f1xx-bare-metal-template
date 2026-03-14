#include "uart.h"
#include "stm32f103xe.h"
#include <stdint.h>

void uart1_init(void) {
    // 1. 启用 USART1 时钟
    // 1.1 开启串口 1 外设时钟
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    // 1.2 开启 GPIO 时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // 2. 配置引脚工作模式
    // 2.1 配置 PA9 为复用推挽输出
    GPIOA->CRH &= ~(0xF << 4);
    GPIOA->CRH |= (0xB << 4);               // 1011
    // 2.2 配置 PA10 为浮空输入
    GPIOA->CRH &= ~GPIO_CRH_MODE10;         // 清除 PA10 模式(MODE10 = 00)
    GPIOA->CRH &= ~GPIO_CRH_CNF10;          // 设置为输入模式(CNF10 = 01)
    GPIOA->CRH |= GPIO_CRH_CNF10_0;         // 设置为浮空输入


    // 3. 配置串口的参数
    // 3.1 配置波特率为 115200
    USART1->BRR = 0x271;                             // 72MHz APB2
    // USART1->BRR = 0x45;                                 // 8MHz APB2
    // 3.2 配置串口使能, 并使能接收与发送
    USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE);       // Transmit Enable & Receive Enable
    // 3.3 配置一个字长度为 8bit
    USART1->CR1 &= ~USART_CR1_M;
    // 3.4 配置不需要校验位
    USART1->CR1 &= ~USART_CR1_PCE;                      // Parity Control Enable
    // 3.5 配置停止位的长度: 0
    USART1->CR2 &= ~USART_CR2_STOP;
    // 3.6 USART enable
    USART1->CR1 |= USART_CR1_UE;                        // USART Enable
}


/**
 * @bref 发送一个字节
 */
void uart1_write_char(uint8_t byte) {
    // 等待发送完成
    while ((USART1->SR & USART_SR_TXE) == 0) {          // Transmit Data Register Empty 
    }

    // 写数据到数据寄存器
    USART1->DR = byte;
}

/**
 * 发送字符串
 */
void uart1_write_str(uint8_t * str, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        uart1_write_char(str[i]);
    }
}

/**
 * 接收一个字节
 */
uint8_t uart1_receive_char(void) {
    // 等待接收寄存器非空
    while ((USART1->SR & USART_SR_RXNE) == 0) {
    }

    // 返回接收到的数据
    return USART1->DR;
}

/**
 * 接收字符串
 * @param   (uint8_t *) buf 接收缓冲区
 * @param   (uint8_t *) 接收到的字节长度
 */
void uart1_receive_str(uint8_t * buf, uint8_t * len) {
    uint8_t i = 0;
    while (1) {
        // 等待接收寄存器非空
        while ((USART1->SR & USART_SR_RXNE) == 0) {
            // 接收过程中判断是否收到空闲帧
            if (USART1->SR & USART_SR_IDLE) {    // 接收到空闲帧 
                *len = i;
                return;
            }
        }

        // 接收到数据
        buf[i] = USART1->DR;
        i++;
    }
}

int _write(int file, char * ptr, int len) {
    uart1_write_str((uint8_t *) ptr, len);
    return len;
}


