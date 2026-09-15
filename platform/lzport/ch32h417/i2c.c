#include "lzport/i2c.h"

#include "ch32h417_i2c.h"
#include "ch32h417_rcc.h"

#define I2C_COUNT 4U
#define I2C_ERROR_FLAGS (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_AF | I2C_FLAG_OVR)
#define I2C_ERROR_MASK  (I2C_ERROR_FLAGS & 0xFFFFU)

typedef struct
{
    I2C_TypeDef *reg;
    uint32_t clock;
    uint8_t hb2;
} i2c_hw;

static const i2c_hw g_hw[I2C_COUNT] = {
    {I2C1, RCC_HB1Periph_I2C1, 0U},
    {I2C2, RCC_HB1Periph_I2C2, 0U},
    {I2C3, RCC_HB1Periph_I2C3, 0U},
    {I2C4, RCC_HB2Periph_I2C4, 1U},
};

static uint8_t g_initialized;

static int pin_valid(const lzport_i2c_pin *pin)
{
    return ((uint32_t)pin->port < LZPORT_GPIO_PORT_COUNT) &&
           ((uint32_t)pin->pin < LZPORT_GPIO_PIN_COUNT) &&
           ((uint32_t)pin->af < LZPORT_GPIO_AF_COUNT);
}

static lzport_status wait_flag(I2C_TypeDef *reg, uint32_t flag, FlagStatus state, uint32_t timeout)
{
    while (timeout-- != 0U) {
        if ((reg->STAR1 & I2C_ERROR_MASK) != 0U) {
            return LZPORT_EIO;
        }
        if (I2C_GetFlagStatus(reg, flag) == state) {
            return LZPORT_OK;
        }
    }
    return LZPORT_ETIMEOUT;
}

static void clear_addr(I2C_TypeDef *reg)
{
    volatile uint32_t unused;

    unused = reg->STAR1;
    unused = reg->STAR2;
    (void)unused;
}

static void receive_restore(I2C_TypeDef *reg)
{
    I2C_NACKPositionConfig(reg, I2C_NACKPosition_Current);
    I2C_AcknowledgeConfig(reg, ENABLE);
}

static void transfer_abort(I2C_TypeDef *reg)
{
    if ((reg->STAR2 & I2C_STAR2_MSL) != 0U) {
        I2C_GenerateSTOP(reg, ENABLE);
    }
    I2C_ClearFlag(reg, I2C_ERROR_FLAGS);
    receive_restore(reg);
}

static lzport_status receive_data(I2C_TypeDef *reg, uint8_t *buf, uint32_t len, uint32_t timeout_cycles)
{
    uint32_t remaining = len;
    lzport_status status;

    if (remaining == 1U) {
        I2C_AcknowledgeConfig(reg, DISABLE);
        clear_addr(reg);
        I2C_GenerateSTOP(reg, ENABLE);
        status = wait_flag(reg, I2C_FLAG_RXNE, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
        *buf = I2C_ReceiveData(reg);
    } else if (remaining == 2U) {
        I2C_NACKPositionConfig(reg, I2C_NACKPosition_Next);
        I2C_AcknowledgeConfig(reg, DISABLE);
        clear_addr(reg);
        status = wait_flag(reg, I2C_FLAG_BTF, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
        I2C_GenerateSTOP(reg, ENABLE);
        *buf++ = I2C_ReceiveData(reg);
        *buf = I2C_ReceiveData(reg);
    } else {
        clear_addr(reg);
        while (remaining > 3U) {
            status = wait_flag(reg, I2C_FLAG_RXNE, SET, timeout_cycles);
            if (status != LZPORT_OK) {
                transfer_abort(reg);
                return status;
            }
            *buf++ = I2C_ReceiveData(reg);
            --remaining;
        }
        status = wait_flag(reg, I2C_FLAG_BTF, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
        I2C_AcknowledgeConfig(reg, DISABLE);
        *buf++ = I2C_ReceiveData(reg);
        status = wait_flag(reg, I2C_FLAG_BTF, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
        I2C_GenerateSTOP(reg, ENABLE);
        *buf++ = I2C_ReceiveData(reg);
        *buf = I2C_ReceiveData(reg);
    }
    receive_restore(reg);
    return LZPORT_OK;
}

lzport_status lzport_i2c_init(uint8_t inst, const lzport_i2c_config *cfg)
{
    I2C_InitTypeDef init = {0};

    if ((inst >= I2C_COUNT) || (cfg == 0) || (cfg->clock_hz == 0U) ||
        (cfg->clock_hz > 400000U) || !pin_valid(&cfg->scl) ||
        !pin_valid(&cfg->sda)) {
        return LZPORT_EINVAL;
    }
    if (g_hw[inst].hb2 != 0U) {
        RCC_HB2PeriphClockCmd(g_hw[inst].clock, ENABLE);
    } else {
        RCC_HB1PeriphClockCmd(g_hw[inst].clock, ENABLE);
    }
    lzport_gpio_mode_af_output(cfg->scl.port, cfg->scl.pin, cfg->scl.af,
                               LZPORT_GPIO_SPEED_VERY_HIGH,
                               LZPORT_GPIO_OPEN_DRAIN);
    lzport_gpio_mode_af_output(cfg->sda.port, cfg->sda.pin, cfg->sda.af,
                               LZPORT_GPIO_SPEED_VERY_HIGH,
                               LZPORT_GPIO_OPEN_DRAIN);

    I2C_DeInit(g_hw[inst].reg);
    init.I2C_ClockSpeed = cfg->clock_hz;
    init.I2C_Mode = I2C_Mode_I2C;
    init.I2C_DutyCycle = I2C_DutyCycle_2;
    init.I2C_OwnAddress1 = 0U;
    init.I2C_Ack = I2C_Ack_Enable;
    init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(g_hw[inst].reg, &init);
    I2C_Cmd(g_hw[inst].reg, ENABLE);
    g_initialized |= (uint8_t)(1U << inst);
    return LZPORT_OK;
}

lzport_status lzport_i2c_deinit(uint8_t inst)
{
    if ((inst >= I2C_COUNT) || ((g_initialized & (1U << inst)) == 0U)) {
        return LZPORT_EINVAL;
    }
    I2C_Cmd(g_hw[inst].reg, DISABLE);
    I2C_DeInit(g_hw[inst].reg);
    g_initialized &= (uint8_t)~(1U << inst);
    return LZPORT_OK;
}

lzport_status lzport_i2c_write(uint8_t inst, uint8_t addr7,
                               const uint8_t *buf, uint32_t len,
                               uint32_t timeout_cycles)
{
    I2C_TypeDef *reg;
    uint32_t i;
    lzport_status status;

    if ((inst >= I2C_COUNT) || ((g_initialized & (1U << inst)) == 0U) ||
        (addr7 > 0x7FU) || ((buf == 0) && (len != 0U)) ||
        (timeout_cycles == 0U)) {
        return LZPORT_EINVAL;
    }
    reg = g_hw[inst].reg;
    I2C_ClearFlag(reg, I2C_ERROR_FLAGS);
    status = wait_flag(reg, I2C_FLAG_BUSY, RESET, timeout_cycles);
    if (status != LZPORT_OK) {
        return (status == LZPORT_ETIMEOUT) ? LZPORT_EBUSY : status;
    }
    I2C_GenerateSTART(reg, ENABLE);
    status = wait_flag(reg, I2C_FLAG_SB, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    I2C_Send7bitAddress(reg, (uint8_t)(addr7 << 1U), I2C_Direction_Transmitter);
    status = wait_flag(reg, I2C_FLAG_ADDR, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    clear_addr(reg);
    for (i = 0U; i < len; ++i) {
        status = wait_flag(reg, I2C_FLAG_TXE, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
        I2C_SendData(reg, buf[i]);
    }
    if (len != 0U) {
        status = wait_flag(reg, I2C_FLAG_BTF, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
    }
    I2C_GenerateSTOP(reg, ENABLE);
    return LZPORT_OK;
}

lzport_status lzport_i2c_read(uint8_t inst, uint8_t addr7, uint8_t *buf,
                              uint32_t len, uint32_t timeout_cycles)
{
    I2C_TypeDef *reg;
    lzport_status status;

    if ((inst >= I2C_COUNT) || ((g_initialized & (1U << inst)) == 0U) ||
        (addr7 > 0x7FU) || ((buf == 0) && (len != 0U)) ||
        (timeout_cycles == 0U)) {
        return LZPORT_EINVAL;
    }
    if (len == 0U) {
        return LZPORT_OK;
    }
    reg = g_hw[inst].reg;
    I2C_ClearFlag(reg, I2C_ERROR_FLAGS);
    status = wait_flag(reg, I2C_FLAG_BUSY, RESET, timeout_cycles);
    if (status != LZPORT_OK) {
        return (status == LZPORT_ETIMEOUT) ? LZPORT_EBUSY : status;
    }
    receive_restore(reg);
    I2C_GenerateSTART(reg, ENABLE);
    status = wait_flag(reg, I2C_FLAG_SB, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    I2C_Send7bitAddress(reg, (uint8_t)(addr7 << 1U), I2C_Direction_Receiver);
    status = wait_flag(reg, I2C_FLAG_ADDR, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }

    return receive_data(reg, buf, len, timeout_cycles);
}

lzport_status lzport_i2c_write_read(uint8_t inst, uint8_t addr7,
                                    const uint8_t *tx, uint32_t tx_len,
                                    uint8_t *rx, uint32_t rx_len,
                                    uint32_t timeout_cycles)
{
    I2C_TypeDef *reg;
    uint32_t i;
    lzport_status status;

    if ((inst >= I2C_COUNT) || ((g_initialized & (1U << inst)) == 0U) ||
        (addr7 > 0x7FU) || ((tx == 0) && (tx_len != 0U)) ||
        ((rx == 0) && (rx_len != 0U)) || (timeout_cycles == 0U)) {
        return LZPORT_EINVAL;
    }
    if (rx_len == 0U) {
        return lzport_i2c_write(inst, addr7, tx, tx_len, timeout_cycles);
    }
    if (tx_len == 0U) {
        return lzport_i2c_read(inst, addr7, rx, rx_len, timeout_cycles);
    }

    reg = g_hw[inst].reg;
    I2C_ClearFlag(reg, I2C_ERROR_FLAGS);
    status = wait_flag(reg, I2C_FLAG_BUSY, RESET, timeout_cycles);
    if (status != LZPORT_OK) {
        return (status == LZPORT_ETIMEOUT) ? LZPORT_EBUSY : status;
    }
    I2C_GenerateSTART(reg, ENABLE);
    status = wait_flag(reg, I2C_FLAG_SB, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    I2C_Send7bitAddress(reg, (uint8_t)(addr7 << 1U), I2C_Direction_Transmitter);
    status = wait_flag(reg, I2C_FLAG_ADDR, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    clear_addr(reg);
    for (i = 0U; i < tx_len; ++i) {
        status = wait_flag(reg, I2C_FLAG_TXE, SET, timeout_cycles);
        if (status != LZPORT_OK) {
            transfer_abort(reg);
            return status;
        }
        I2C_SendData(reg, tx[i]);
    }
    status = wait_flag(reg, I2C_FLAG_BTF, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }

    receive_restore(reg);
    I2C_GenerateSTART(reg, ENABLE);
    status = wait_flag(reg, I2C_FLAG_SB, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    I2C_Send7bitAddress(reg, (uint8_t)(addr7 << 1U), I2C_Direction_Receiver);
    status = wait_flag(reg, I2C_FLAG_ADDR, SET, timeout_cycles);
    if (status != LZPORT_OK) {
        transfer_abort(reg);
        return status;
    }
    return receive_data(reg, rx, rx_len, timeout_cycles);
}
