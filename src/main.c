#include <stdio.h>
#include <stm32f1xx.h>
#include "delay.h"
#include "uart.h"

#define LED_PORT GPIOD
#define LED_PIN 15

int main(void) {
    SysTick_Init();
    uart1_init();
    while (1) {
        printf("hello world!\n");
        delayMs(200);
    }
}
