#ifndef __DELAY_H__
#define __DELAY_H__

#include <stdint.h>
#include <stm32f1xx.h>

void SysTick_Init(void);
void delayMs(uint32_t ms);

#endif
