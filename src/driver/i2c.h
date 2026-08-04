#ifndef __I2C_H__
#define __I2C_H__

#include <stm32f1xx.h>

typedef struct {
    I2C_TypeDef * instance;
    uint8_t scl_pin;
    uint8_t sda_pin;
} i2c_type;

typedef enum {
    I2C_OK,
    I2C_TIMEOUT,
    I2C_NACK,
    I2C_BUS_ERROR,
    I2C_OVERRUN
} i2c_status_t;

i2c_type * i2c_init(uint8_t num);
uint8_t i2c_start(i2c_type * i2c);
void i2c_stop(i2c_type * i2c);
uint8_t i2c_wait_event(i2c_type * i2c, uint32_t event_mask);

// 读写
uint8_t i2c_send_addr(i2c_type * i2c, uint8_t addr, uint8_t rw);
uint8_t i2c_send_data(i2c_type * i2c, uint8_t data);
uint8_t i2c_read_byte(i2c_type * i2c, uint8_t ack);


#endif
