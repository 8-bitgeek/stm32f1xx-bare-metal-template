#include "i2c.h"

/** 
  * I2C 的 6 个时序单元: 起始条件/终止条件/发送一个字节/接收一个字节/发送应答/接收应答
  * 每个时序单元一个函数, 方便上层应用组合拼装
  * PB6:    I2C1_SCL
  * PB7:    I2C1_SDA
  * PB10:   I2C2_SCL
  * PB11:   I2C2_SDA
  */


static i2c_type i2c1_conf = {
    .instance = I2C1,
    .sda_pin = 7,  // PB7
    .scl_pin = 6,  // PB6
};

static i2c_type * volatile i2c = &i2c1_conf;

void i2c_init(void) {
    I2C_TypeDef * instance = i2c->instance;

    // 1. Enable clock: I2C and GPIO
    if (i2c->sda_pin >= 8) {
        RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
    } else {
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    }
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    // 2. Config GPIO MODE and CNF: Alternate function output Open-drain ==> MODE: 11, CNF: 11
    // 两个引脚都设置为复用推挽输出: 复用推挽输出在输出高电压的时候并不会真正(在引脚上)输出高电压, 
    // 而是靠上拉电阻拉到高电压, 所以没有驱动能力 
    volatile uint32_t * CR;
    if (i2c->sda_pin >= 8) {
        CR = &(GPIOB->CRH);
    } else {
        CR = &(GPIOB->CRL);
    }
    // Config MODE/CNF bits
    *CR |= 0b1111 << ((i2c->sda_pin % 8) * 4);
    *CR |= 0b1111 << ((i2c->scl_pin % 8) * 4);

    // 3. Config periphrial register
    // 3.1 Config CR1 for SMBus mode = 0: I2C mode   1: SMBus mode
    instance->CR1 &= ~I2C_CR1_SMBUS;
    // 3.2 Config CR2 for I2C periphrial frequency: 36MHz
    instance->CR2 |= 36;                                // 设备 I2C 外设频率为 36MHz(AHB / 2)
    // 3.3 Config CCR
    // 3.3.1 Master mode: 0 - Standard mode; 1 - Fast mode
    instance->CCR &= ~I2C_CCR_FS;
    // 3.3.2 Config I2C clock frequency
    instance->CCR |= 180 << 0;                          // I2C 速率为标准模式 100KHz: CCR = PCLK1 / (2 * 目标频率) = 36MHz / (2 * 100K) = 180
    // 3.4 Config TRISE
    instance->TRISE = 37;

    // 确保关闭 I2C 中断
    instance->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);

    // 4. Enable periphrial
    instance->CR1 |= I2C_CR1_PE;
}

/**
  * @brief I2C 发送起始条件
  * @retval 1 - 成功; 0 - 失败;
  */
uint8_t i2c_start(void) {
    // 发送起始位, Start bit 置位后会自动发送一个起始位
    // repeated start generation: 重复起始位 -> 不发送结束位, 直接再发送一个起始位, 省去一个结束信号的发送
    i2c->instance->CR1 |= I2C_CR1_START;

    // 等待 Start Bit 置位
    uint8_t suc = i2c_wait_event(I2C_SR1_SB_Msk);
    uint32_t tmp = i2c->instance->SR1;
    (void) tmp;
    return suc;
}

/**
  * @brief 发送停止信号
  */
void i2c_stop(void) {
    // 发送停止信号
    i2c->instance->CR1 |= I2C_CR1_STOP;
}

/**
  * @brief I2C 发送地址
  * @param addr 7 位从机地址
  * @param rw 读写标志位(r - 1, w - 0)
  * @retval 1 - 成功; 0 - 失败;
  */
uint8_t i2c_send_addr(uint8_t addr, uint8_t rw) {
    // 发送从机地址加读写标志位
    i2c->instance->DR = (addr << 1) | rw;

    // 等待地址发送完成
    uint8_t ret = i2c_wait_event(I2C_SR1_ADDR_Msk);             // 地址发送完成标志

    // 读取 SR1 后再读取 SR2 清除 ADDR 标志
    volatile uint32_t tmp = i2c->instance->SR1;
    tmp = i2c->instance->SR2;
    (void) tmp;                                                 // 告诉编译器故意不用, 编译时不要警告(可以不加, 但是编译会有警告)

    return ret;
}

/**
  * @brief 发送 1 字节数据 (阻塞方式)
  * @note  批量发送应由上层通过循环调用本函数实现, 原因:
  *        1. I2C 协议要求连续发送时中间不能有 Stop 条件
  *        2. 发送每个字节后需要检查从机的 ACK 应答
  *        3. 上层需要灵活控制何时 Stop 以及是否重发 Start
  * @param data 要发送的数据
  * @retval 1 - 发送成功 (收到 ACK) 0 - 发送失败 (收到 NACK 或超时）
  */
uint8_t i2c_send_data(uint8_t data) {
    // 发送数据
    i2c->instance->DR = data;
    
    // 等待传输完成
    if (i2c_wait_event(I2C_SR1_BTF_Msk) == 0) {
        return 0;                               // 超时
    }
    
    // 检查是否收到 NACK
    if (i2c->instance->SR1 & I2C_SR1_AF_Msk) {
        i2c->instance->SR1 &= ~I2C_SR1_AF_Msk;  // 清除 AF 标志
        return 0;                               // NACK, 发送失败
    }
    
    return 1;                                   // 成功收到 ACK
}

/**
  * @brief 等待对应事件完成(状态标志置位)
  * @param event_mask 事件标志位掩码
  * @retval 1 - 成功; 0 - 失败;
  */
uint8_t i2c_wait_event(uint32_t event_mask) {
    uint32_t timeout = 100000;

    while ((i2c->instance->SR1 & event_mask) == 0) {
        if (--timeout == 0) {                               // 超时返回 0
            return 0;
        }
    }
    return 1;                                               // 正常返回 1
}

/**
  * @brief I2C 读取字节(ACK/NACK 控制)
  * @param ack 1 - 发送 NACK(最后一个字节); 0 - 发送 ACK(继续读)
  * @retval 读取到的数据
  */
uint8_t i2c_read_byte(uint8_t ack) {
    // 等待 RXNE (接收缓冲区非空)
    i2c_wait_event(I2C_SR1_RXNE);
    // 读取数据
    uint8_t data = i2c->instance->DR;

    // 配置下一个字节的 ACK 应答
    if (ack) {
        i2c->instance->CR1 &= ~I2C_CR1_ACK;
    } else {
        i2c->instance->CR1 |= I2C_CR1_ACK;
    }

    return data;
}
