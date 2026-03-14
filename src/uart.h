#ifndef __UART_H__
#define __UART_H__

#include <stm32f1xx.h>
void uart1_init(void);
void uart1_write_char(uint8_t byte);
void uart1_write_str(uint8_t * str, uint8_t len);
uint8_t uart1_receive_char(void);

#endif
