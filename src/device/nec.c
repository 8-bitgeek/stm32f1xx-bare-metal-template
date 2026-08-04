#include "nec.h"
#include "../driver/gpio.h"
#include "../driver/tim2.h"

#define NEC_LEADER_LOW_MIN      8500
#define NEC_LEADER_LOW_MAX      10500
#define NEC_LEADER_HIGH_MIN     4000
#define NEC_LEADER_HIGH_MAX     5000
#define NEC_REPEAT_HIGH_MIN     2000
#define NEC_REPEAT_HIGH_MAX     2500
#define NEC_BIT0_HIGH_MIN       400
#define NEC_BIT0_HIGH_MAX       700
#define NEC_BIT1_HIGH_MIN       1500
#define NEC_BIT1_HIGH_MAX       1900
#define NEC_DATA_LOW_MIN        400
#define NEC_DATA_LOW_MAX        700

static nec_state_t nec_state = NEC_IDLE;
static volatile uint16_t last_tick = 0;
static nec_data_t nec_event[8];
static volatile uint8_t nec_head = 0;
static volatile uint8_t nec_tail = 0;
static nec_data_t nec_empty = {0};

static const nec_key_map_t key_maps[] = { 
    {0x19, NEC_KEY_NUM0}, {0x45, NEC_KEY_NUM1}, {0x46, NEC_KEY_NUM2}, {0x47, NEC_KEY_NUM3},
    {0x44, NEC_KEY_NUM4}, {0x40, NEC_KEY_NUM5}, {0x43, NEC_KEY_NUM6}, {0x07, NEC_KEY_NUM7},
    {0x15, NEC_KEY_NUM8}, {0x09, NEC_KEY_NUM9}, {0x16, NEC_KEY_STAR}, {0x0D, NEC_KEY_HASH},
    {0x18, NEC_KEY_UP},   {0x52, NEC_KEY_DOWN}, {0x08, NEC_KEY_LEFT}, {0x5A, NEC_KEY_RIGHT}, {0x1C, NEC_KEY_OK}
};

nec_key_t nec_get_key(uint8_t command) {
    for (uint8_t i = 0; i < sizeof(key_maps) / sizeof(key_maps[0]); i++) {
        if (key_maps[i].command == command) {
            return key_maps[i].key;
        }
    }
    return NEC_KEY_NONE;
}

/**
  * 初始化函数, 使用 GPIOB-15 引脚接收红外传感器数据
  */
void nec_init(void) {
    tim2_init();
    // 1. 初始化 GPIO
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    gpio_t nec_gpio = {
        GPIOB, 15, INPUT_UD, 1
    };
    gpio_init(nec_gpio);
    
    // 2. 开启 AFIO 时钟
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // 3. 配置引脚外部中断
    // 3.1 多路复用: 开启 PB15 中断
    AFIO->EXTICR[3] &= ~AFIO_EXTICR4_EXTI15;
    AFIO->EXTICR[3] |= AFIO_EXTICR4_EXTI15_PB;
    // 3.2 清空中断事件
    EXTI->PR |= EXTI_PR_PR15;
    // 3.3 下降沿/上升沿双边沿触发外部中断
    EXTI->FTSR |= EXTI_FTSR_FT15;
    EXTI->RTSR |= EXTI_RTSR_RT15;
    // 3.4 EXTI 15 中断使能
    EXTI->IMR |= EXTI_IMR_IM15;

    // 4. 配置 NVIC
    // NVIC_SetPriorityGrouping(3);
    NVIC_SetPriority(EXTI15_10_IRQn, 0);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}


nec_data_t nec_get_event() {
    if (nec_head == nec_tail) {
        return nec_empty;
    } else {
        nec_data_t event = nec_event[nec_head++];
        nec_head &= 0x07;
        return event;
    }
}

static nec_data_t nec_parse_data(uint32_t data) {
    nec_data_t nec_data = {0};
    nec_data.address = data & 0xFF;
    nec_data.address_inv = (data >> 8) & 0xFF;
    nec_data.command = (data >> 16) & 0xFF;
    nec_data.command_inv = (data >> 24) & 0xFF;
    if (((nec_data.address ^ nec_data.address_inv) == 0xff) && ((nec_data.command ^ nec_data.command_inv) == 0xff)) {
        nec_data.valid = 1;
    } else {
        nec_data.valid = 0;
    }
    return nec_data;
}

/**
  * NEC Protocol: 
  *     - Leader code: 9ms low + 4.5ms high
  *     - 32 bit data: 0 - 560us low + 560us high; 1 - 560us low + 1690us high;
  *         - Address 
  *         - Address inverse
  *         - Command
  *         - Command inverse
  */
void nec_decode(uint16_t dt) {
    static uint32_t data = 0;
    static uint8_t bit_cnt = 0;

    switch (nec_state) {
        case NEC_IDLE:              // 下降沿触发
            // 只有空闲时收到下降沿, 开始 9ms 低电平(数据接收结束后, 会有一个上升沿, 此时不应该触发接收)
            if (!(GPIOB->IDR & GPIO_IDR_IDR15)) {
                nec_state = NEC_LEADER_LOW;
                data = 0;           // 数据复位, 准备接收
                bit_cnt = 0;
            }
            break;
        case NEC_LEADER_LOW:        // 上升沿触发
            if (dt >= NEC_LEADER_LOW_MIN && dt <= NEC_LEADER_LOW_MAX) {
                nec_state = NEC_LEADER_HIGH;
            } else {
                nec_state = NEC_IDLE;
            }
            break;
        case NEC_LEADER_HIGH:       // 下降沿触发
            if (dt >= NEC_LEADER_HIGH_MIN && dt <= NEC_LEADER_HIGH_MAX) { 
                nec_state = NEC_DATA_LOW;
            } else {
                nec_state = NEC_IDLE;
            }
            break;
        case NEC_DATA_LOW:          // 上升沿触发
            if (dt >= NEC_DATA_LOW_MIN && dt <= NEC_DATA_LOW_MAX) {
                nec_state = NEC_DATA_HIGH;
            } else {
                nec_state = NEC_IDLE;
            }
            break;
        case NEC_DATA_HIGH:         // 下降沿触发
            // LSB first
            if (dt >= NEC_BIT0_HIGH_MIN && dt <= NEC_BIT0_HIGH_MAX) {      // bit 0
                // data &= ~(1UL << bit_cnt);   // 只有 bit 1 需要设置, 默认就是 0
            } else if (dt >= NEC_BIT1_HIGH_MIN && dt <= NEC_BIT1_HIGH_MAX) {
                data |= (1UL << bit_cnt);
            } else {                            // 非法状态
                nec_state = NEC_IDLE;
                break;
            }
            if (++bit_cnt >= 32) {              // 数据接收完成后, 恢复 idle 状态
                nec_state = NEC_IDLE;
                nec_data_t nec_data = nec_parse_data(data);
                if (nec_data.valid) {           // 只有有效的数据才入队
                    nec_event[nec_tail++] = nec_data;
                    nec_tail &= 0x07;
                }
            } else {                            // 不够 32 位时继续接收
                nec_state = NEC_DATA_LOW;
            }
            break;
        default: 
            nec_state = NEC_IDLE;
            break;
    }
}


void EXTI15_10_IRQHandler(void) {

    // 15 号引脚触发中断
    if (EXTI->PR & EXTI_PR_PR15) {
        // 清中断
        EXTI->PR |= EXTI_PR_PR15;

        uint16_t now = TIM2->CNT;
        // 计算时间差(无符号减法天然支持 16 位回绕)
        uint16_t dt = now - last_tick;

        last_tick = now;

        nec_decode(dt);
    }
}
