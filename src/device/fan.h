#ifndef __FAN_H__
#define __FAN_H__

#include <stdint.h>
#include <stm32f1xx.h>

void fan_init(void);
void fan_set_gear(uint8_t gear);
void fan_toggle(void);
void fan_speed_up(void);
void fan_speed_down(void);
uint8_t fan_get_gear(void);
void fan_set_dir(uint8_t dir);
void fan_toggle_dir(void);
uint8_t fan_get_dir(void);
uint8_t fan_get_switch_status(void);
uint8_t fan_get_duty(void);

#endif
