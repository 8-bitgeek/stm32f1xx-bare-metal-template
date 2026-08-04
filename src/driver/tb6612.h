#ifndef __TB6612_H__
#define __TB6612_H__

#include <stm32f1xx.h>


typedef enum {
    TB6612_SW_STOP = 0,
    TB6612_SW_RUN,
    TB6612_SW_BRAKE                     // 刹车
} tb6612_state_t;

typedef enum {
    TB6612_DIR_FORWARD = 0,
    TB6612_DIR_REVERSE,
} tb6612_dir_t;

void tb6612_init(void);
void tb6612_set_dir(tb6612_dir_t dir);
void tb6612_start(void);
void tb6612_stop(void);
tb6612_dir_t tb6612_get_dir(void);
tb6612_state_t tb6612_get_state(void);
void tb6612_set_duty(uint8_t duty);
uint8_t tb6612_get_duty(void);

#endif
