#include "fan.h"
#include "../driver/tb6612.h"

void fan_init(void) {
    tb6612_init();
    tb6612_stop();
    tb6612_set_dir(TB6612_DIR_FORWARD);
}

/**
  * 当前转向
  */
static volatile int8_t fan_gear = 0;

/**
  * @brief  设置风扇转速
  *
  * @param  gear - 0 ~ 10 共 10 个档位
  */
void fan_set_gear(uint8_t gear) {
    if (gear <= 10) {
        fan_gear = gear;
        tb6612_set_duty(gear * 10);
    }
}

void fan_toggle_dir(void) {
    tb6612_dir_t dir = tb6612_get_dir();
    if (dir == TB6612_DIR_FORWARD) {
        tb6612_set_dir(TB6612_DIR_REVERSE);
    } else {
        tb6612_set_dir(TB6612_DIR_FORWARD);
    }
}

uint8_t fan_get_dir(void) {
    return tb6612_get_dir();
}

void fan_set_dir(uint8_t dir) {
    tb6612_set_dir(dir ? TB6612_DIR_FORWARD : TB6612_DIR_REVERSE);
}

void fan_speed_up(void) {
    if (++fan_gear > 10) {
        fan_gear = 10;
    }
    tb6612_set_duty(fan_gear * 10);
}

void fan_speed_down(void) {
    if (--fan_gear <= 0) {
        fan_gear = 0;
    }
    tb6612_set_duty(fan_gear * 10);
}

uint8_t fan_get_gear(void) {
    return fan_gear;
}


/**
  * @brief  开关风扇
  * @param  1 - 打开 0 - 关闭
  */
void fan_toggle(void) {
    tb6612_state_t state = tb6612_get_state();
    if (state != TB6612_SW_RUN) {
        tb6612_start();
    } else {
        tb6612_stop();
    }
}


uint8_t fan_get_switch_status() {
    return tb6612_get_state();
}

uint8_t fan_get_duty(void) {
    return tb6612_get_duty();
}
