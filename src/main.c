#include <stdio.h>
#include <stm32f1xx.h>
#include "driver/delay.h"
#include "driver/uart.h"

int main(void) {
    SysTick_Init();
    uart1_init();

    while (1) {
        printf("hello world!\n");
        delayMs(200);
    }
}
