#include <stm32f1xx.h>
#include <stdio.h>

#define LED_PORT GPIOC
#define LED_PIN 7

void delayMs(uint16_t ms);

int main(void) {
  // Enable the port GPIO peripheral clock
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPCEN);

  // Set output mode, push-pull, 10MHz max.
  // Multiply pin by four as configuration field is 4 bits wide.
  // Need to subtract 8 from pin as the high control register is used.
  MODIFY_REG(LED_PORT->CRL, 0x03 << (LED_PIN * 4), 0x01 << (LED_PIN * 4));
  MODIFY_REG(LED_PORT->CRL, 0x0c << (LED_PIN * 4), 0x00 << (LED_PIN * 4));

  while (1) {

    // Set the LED pin and delay
    SET_BIT(LED_PORT->BSRR, 0x1 << LED_PIN);
    delayMs(500);
    printf("hello world!\n");

    // Clear the LED pin and delay
    SET_BIT(LED_PORT->BRR, 0x1 << LED_PIN);
    delayMs(500);
  }
}

// Millisecond delay is an estimate
void delayMs(uint16_t ms) {

  for (uint16_t i = 0; i < ms; i++) {
    for (uint16_t j = 0; j < 0x6FF; j++) {
      __ASM("NOP");
    }
  }
}
