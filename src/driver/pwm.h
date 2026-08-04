#ifndef __TIMER3_H__
#define __TIMER3_H__

#include <stm32f1xx.h>
#include <stdint.h>

void pwm_init(void);
void pwm_start(void);
void pwm_stop(void);
void pwm_set_duty_cycle(uint16_t duty_cycle);
uint8_t pwm_get_duty();
#endif
