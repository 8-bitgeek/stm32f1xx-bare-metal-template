#ifndef __NEC_H__
#define __NEC_H__

#include <stm32f1xx.h>

typedef enum {
    NEC_IDLE,
    NEC_LEADER_LOW,
    NEC_LEADER_HIGH,
    NEC_DATA_LOW,
    NEC_DATA_HIGH
} nec_state_t;

/**  
  * Data:
  *    - Address 
  *    - Address inverse
  *    - Command
  *    - Command inverse
  */
typedef struct {
    uint8_t address;
    uint8_t address_inv;
    uint8_t command;
    uint8_t command_inv;
    uint8_t valid;
} nec_data_t;

typedef enum {
    NEC_KEY_NUM0 = 0,
    NEC_KEY_NUM1,
    NEC_KEY_NUM2,
    NEC_KEY_NUM3,
    NEC_KEY_NUM4,
    NEC_KEY_NUM5,
    NEC_KEY_NUM6,
    NEC_KEY_NUM7,
    NEC_KEY_NUM8,
    NEC_KEY_NUM9,

    NEC_KEY_STAR,
    NEC_KEY_HASH,

    NEC_KEY_UP,
    NEC_KEY_DOWN,
    NEC_KEY_LEFT,
    NEC_KEY_RIGHT,
    NEC_KEY_OK,

    NEC_KEY_NONE
} nec_key_t;

typedef struct {
    uint8_t command;
    nec_key_t key;
} nec_key_map_t;

void nec_init(void);
nec_data_t nec_get_event();
nec_key_t nec_get_key(uint8_t command);

#endif
