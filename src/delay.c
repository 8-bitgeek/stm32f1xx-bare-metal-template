#include "delay.h"

/* systick counter */
volatile uint32_t msTicks = 0;  

/**
 * SysTick interrupt handler
 */
void SysTick_Handler(void) {
    msTicks++;
}

/**
 * init SysTick, 1 interrupt per ms
 */
void SysTick_Init(void) {
    if (SysTick_Config(SystemCoreClock / 1000)) {
        // Configure failed
        while (1);
    }
}

void delayMs(uint32_t ms) {
    uint32_t start = msTicks;
    while ((msTicks - start) < ms);
}
