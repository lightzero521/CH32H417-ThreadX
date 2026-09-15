#include "lzport/spi.h"

#include "ch32h417_rcc.h"
#include "ch32h417_spi.h"

#define SPI_COUNT          4U
#define SPI_POLL_TIMEOUT   100000U

typedef struct
{
    SPI_TypeDef *reg;
    uint32_t clock;
    uint8_t hb2;
} spi_hw;

static const spi_hw g_hw[SPI_COUNT] = {
    {SPI1, RCC_HB2Periph_SPI1, 1U},
    {SPI2, RCC_HB1Periph_SPI2, 0U},
    {SPI3, RCC_HB1Periph_SPI3, 0U},
    {SPI4, RCC_HB1Periph_SPI4, 0U},
};

static uint8_t g_initialized;

static int pin_valid(const lzport_spi_pin *pin)
{
    return ((uint32_t)pin->port < LZPORT_GPIO_PORT_COUNT) &&
           ((uint32_t)pin->pin < LZPORT_GPIO_PIN_COUNT) &&
           ((uint32_t)pin->af < LZPORT_GPIO_AF_COUNT);
}

static uint16_t baud_prescaler(uint32_t input_hz, uint32_t max_hz)
{
    uint32_t divider = (input_hz + max_hz - 1U) / max_hz;

    if (divider <= 2U) return SPI_BaudRatePrescaler_Mode0;
    if (divider <= 4U) return SPI_BaudRatePrescaler_Mode1;
    if (divider <= 8U) return SPI_BaudRatePrescaler_Mode2;
    if (divider <= 16U) return SPI_BaudRatePrescaler_Mode3;
    if (divider <= 32U) return SPI_BaudRatePrescaler_Mode4;
    if (divider <= 64U) return SPI_BaudRatePrescaler_Mode5;
    if (divider <= 128U) return SPI_BaudRatePrescaler_Mode6;
    return SPI_BaudRatePrescaler_Mode7;
}

static int wait_flag(SPI_TypeDef *reg, uint16_t flag, FlagStatus state)
{
    uint32_t timeout = SPI_POLL_TIMEOUT;

    while (timeout-- != 0U) {
        if (SPI_I2S_GetFlagStatus(reg, flag) == state) {
            return 1;
        }
    }
    return 0;
}

lzport_status lzport_spi_init(uint8_t inst, const lzport_spi_config *cfg)
{
    RCC_ClocksTypeDef clocks;
    SPI_InitTypeDef init = {0};

    if ((inst >= SPI_COUNT) || (cfg == 0) || (cfg->max_hz == 0U) ||
        ((uint32_t)cfg->mode >= LZPORT_SPI_MODE_COUNT) ||
        !pin_valid(&cfg->sck) || !pin_valid(&cfg->miso) ||
        !pin_valid(&cfg->mosi)) {
        return LZPORT_EINVAL;
    }
    RCC_GetClocksFreq(&clocks);
    if (cfg->max_hz < ((clocks.HCLK_Frequency + 255U) / 256U)) {
        return LZPORT_EINVAL;
    }
    if (g_hw[inst].hb2 != 0U) {
        RCC_HB2PeriphClockCmd(g_hw[inst].clock, ENABLE);
    } else {
        RCC_HB1PeriphClockCmd(g_hw[inst].clock, ENABLE);
    }
    lzport_gpio_mode_af_output(cfg->sck.port, cfg->sck.pin, cfg->sck.af,
                               LZPORT_GPIO_SPEED_VERY_HIGH,
                               LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_mode_af_output(cfg->mosi.port, cfg->mosi.pin, cfg->mosi.af,
                               LZPORT_GPIO_SPEED_VERY_HIGH,
                               LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_mode_af_input(cfg->miso.port, cfg->miso.pin, cfg->miso.af,
                              LZPORT_GPIO_PULL_NONE);

    SPI_I2S_DeInit(g_hw[inst].reg);
    init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    init.SPI_Mode = SPI_Mode_Master;
    init.SPI_DataSize = SPI_DataSize_8b;
    init.SPI_CPOL = (cfg->mode >= LZPORT_SPI_MODE_2) ? SPI_CPOL_High
                                                     : SPI_CPOL_Low;
    init.SPI_CPHA = ((cfg->mode == LZPORT_SPI_MODE_1) ||
                     (cfg->mode == LZPORT_SPI_MODE_3)) ? SPI_CPHA_2Edge
                                                       : SPI_CPHA_1Edge;
    init.SPI_NSS = SPI_NSS_Soft;
    init.SPI_BaudRatePrescaler = baud_prescaler(clocks.HCLK_Frequency,
                                                 cfg->max_hz);
    init.SPI_FirstBit = SPI_FirstBit_MSB;
    init.SPI_CRCPolynomial = 7U;
    SPI_Init(g_hw[inst].reg, &init);
    SPI_NSSInternalSoftwareConfig(g_hw[inst].reg, SPI_NSSInternalSoft_Set);
    SPI_Cmd(g_hw[inst].reg, ENABLE);
    g_initialized |= (uint8_t)(1U << inst);
    return LZPORT_OK;
}

lzport_status lzport_spi_deinit(uint8_t inst)
{
    if ((inst >= SPI_COUNT) || ((g_initialized & (1U << inst)) == 0U)) {
        return LZPORT_EINVAL;
    }
    SPI_Cmd(g_hw[inst].reg, DISABLE);
    SPI_I2S_DeInit(g_hw[inst].reg);
    g_initialized &= (uint8_t)~(1U << inst);
    return LZPORT_OK;
}

lzport_status lzport_spi_xfer(uint8_t inst, const uint8_t *tx, uint8_t *rx,
                              uint32_t len)
{
    SPI_TypeDef *reg;
    uint32_t i;

    if ((inst >= SPI_COUNT) || ((g_initialized & (1U << inst)) == 0U)) {
        return LZPORT_EINVAL;
    }
    reg = g_hw[inst].reg;
    while (SPI_I2S_GetFlagStatus(reg, SPI_I2S_FLAG_RXNE) == SET) {
        (void)SPI_I2S_ReceiveData(reg);
    }
    if (SPI_I2S_GetFlagStatus(reg, SPI_I2S_FLAG_OVR) == SET) {
        (void)SPI_I2S_ReceiveData(reg);
        (void)reg->STATR;
    }
    for (i = 0U; i < len; ++i) {
        uint8_t value;

        if (!wait_flag(reg, SPI_I2S_FLAG_TXE, SET)) {
            return LZPORT_ETIMEOUT;
        }
        SPI_I2S_SendData(reg, (tx != 0) ? tx[i] : 0xFFU);
        if (!wait_flag(reg, SPI_I2S_FLAG_RXNE, SET)) {
            return LZPORT_ETIMEOUT;
        }
        value = (uint8_t)SPI_I2S_ReceiveData(reg);
        if (rx != 0) {
            rx[i] = value;
        }
    }
    if (!wait_flag(reg, SPI_I2S_FLAG_BSY, RESET)) {
        return LZPORT_ETIMEOUT;
    }
    return (SPI_I2S_GetFlagStatus(reg, SPI_I2S_FLAG_OVR) == RESET)
               ? LZPORT_OK
               : LZPORT_EIO;
}
