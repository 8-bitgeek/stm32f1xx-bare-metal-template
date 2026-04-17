#ifndef __I2C_H__
#define __I2C_H__

#include <stm32f1xx.h>

typedef struct {
    I2C_TypeDef * instance;
    uint8_t scl_pin;
    uint8_t sda_pin;
} i2c_type;

void i2c_init(void);
uint8_t i2c_start(void);
void i2c_stop(void);
uint8_t i2c_wait_event(uint32_t event_mask);

// 读写
uint8_t i2c_send_addr(uint8_t addr, uint8_t rw);
uint8_t i2c_send_data(uint8_t data);
uint8_t i2c_read_byte(uint8_t ack);


#endif
