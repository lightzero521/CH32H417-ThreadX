#include "lzport/serial.h"

#include "ch32h417_dma.h"
#include "ch32h417_rcc.h"
#include "ch32h417_usart.h"

#define SERIAL_PORT_COUNT          8U
#define SERIAL_DMA_COUNT           2U
#define SERIAL_DMA_CHANNEL_COUNT   8U
#define SERIAL_DMA_MAX_TRANSFER    65535U
#define SERIAL_DMA_UNBOUND         0xFFU
#define SERIAL_DMA_REQUEST_MAX     123U
#define SERIAL_POLL_TIMEOUT        100000U
#define LZPORT_SERIAL_IRQ_RX_PRIORITY    0x20U  /* bit[7:5] = 001b */
#define LZPORT_SERIAL_IRQ_TX_PRIORITY    0x80U  /* bit[7:5] = 100b */


typedef enum
{
    SERIAL_TX = 0,
    SERIAL_RX = 1
} serial_direction;

typedef struct
{
    USART_TypeDef *usart;
    uint32_t clock;
    IRQn_Type irq;
} serial_hw;

typedef struct
{
    uint32_t len;
    lzport_serial_done_cb done;
    void *user;
    uint8_t controller;
    uint8_t channel;
    volatile uint8_t busy;
} serial_dma_binding;

typedef struct
{
    serial_dma_binding tx;
    serial_dma_binding rx;
} serial_state;

static const serial_hw g_hw[SERIAL_PORT_COUNT] = {
    {USART1, RCC_HB2Periph_USART1, USART1_IRQn},
    {USART2, RCC_HB1Periph_USART2, USART2_IRQn},
    {USART3, RCC_HB1Periph_USART3, USART3_IRQn},
    {USART4, RCC_HB1Periph_USART4, USART4_IRQn},
    {USART5, RCC_HB1Periph_USART5, USART5_IRQn},
    {USART6, RCC_HB1Periph_USART6, USART6_IRQn},
    {USART7, RCC_HB1Periph_USART7, USART7_IRQn},
    {USART8, RCC_HB1Periph_USART8, USART8_IRQn}
};

static DMA_TypeDef *const g_dma[SERIAL_DMA_COUNT] = {DMA1, DMA2};
static DMA_Channel_TypeDef *const g_dma_channel[SERIAL_DMA_COUNT][SERIAL_DMA_CHANNEL_COUNT] = {
    {DMA1_Channel1, DMA1_Channel2, DMA1_Channel3, DMA1_Channel4,
     DMA1_Channel5, DMA1_Channel6, DMA1_Channel7, DMA1_Channel8},
    {DMA2_Channel1, DMA2_Channel2, DMA2_Channel3, DMA2_Channel4,
     DMA2_Channel5, DMA2_Channel6, DMA2_Channel7, DMA2_Channel8}
};
static const IRQn_Type g_dma_irq[SERIAL_DMA_COUNT][SERIAL_DMA_CHANNEL_COUNT] = {
    {DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn, DMA1_Channel4_IRQn,
     DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel7_IRQn, DMA1_Channel8_IRQn},
    {DMA2_Channel1_IRQn, DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn,
     DMA2_Channel5_IRQn, DMA2_Channel6_IRQn, DMA2_Channel7_IRQn, DMA2_Channel8_IRQn}
};

/* 0 means free; otherwise 1 + port * 2 + direction. */
static uint8_t g_dma_owner[SERIAL_DMA_COUNT][SERIAL_DMA_CHANNEL_COUNT];
static serial_state g_state[SERIAL_PORT_COUNT];
static uint8_t g_initialized;

static uint32_t serial_dma_gl_flag(uint8_t channel)
{
    return 1UL << (channel * 4U);
}

static uint32_t serial_dma_tc_flag(uint8_t channel)
{
    return 2UL << (channel * 4U);
}

static uint32_t serial_dma_te_flag(uint8_t channel)
{
    return 8UL << (channel * 4U);
}

static uint8_t serial_owner_value(uint8_t port, serial_direction direction)
{
    return (uint8_t)(1U + port * 2U + (uint8_t)direction);
}

static int serial_initialized(uint8_t port)
{
    return (port < SERIAL_PORT_COUNT) && ((g_initialized & (1U << port)) != 0U);
}

static serial_dma_binding *serial_binding(uint8_t port, serial_direction direction)
{
    return (direction == SERIAL_TX) ? &g_state[port].tx : &g_state[port].rx;
}

static void serial_binding_reset(serial_dma_binding *binding)
{
    binding->len = 0U;
    binding->done = 0;
    binding->user = 0;
    binding->controller = SERIAL_DMA_UNBOUND;
    binding->channel = SERIAL_DMA_UNBOUND;
    binding->busy = 0U;
}

static void serial_idle_clear(USART_TypeDef *usart)
{
    volatile uint16_t clear = usart->STATR;
    clear = usart->DATAR;
    (void)clear;
}

static int serial_wait_flag(USART_TypeDef *usart, uint16_t flag, uint32_t timeout)
{
    while (timeout-- != 0U) {
        if (USART_GetFlagStatus(usart, flag) != RESET) {
            return 1;
        }
    }
    return 0;
}

static void serial_finish(uint8_t port, serial_direction direction,
                          lzport_status status, uint32_t actual)
{
    serial_dma_binding *binding = serial_binding(port, direction);
    lzport_serial_done_cb done = binding->done;
    void *user = binding->user;

    binding->done = 0;
    binding->user = 0;
    binding->busy = 0U;
    if (done != 0) {
        done(port, status, actual, user);
    }
}

static void serial_dma_channel_setup(uint8_t port, serial_direction direction,
                                     uint8_t controller, uint8_t channel,
                                     uint8_t request)
{
    DMA_Channel_TypeDef *dma_channel = g_dma_channel[controller][channel];
    DMA_InitTypeDef init = {0};

    RCC_HBPeriphClockCmd((controller == 0U) ? RCC_HBPeriph_DMA1 : RCC_HBPeriph_DMA2,
                         ENABLE);
    DMA_DeInit(dma_channel);
    init.DMA_PeripheralBaseAddr = (uint32_t)&g_hw[port].usart->DATAR;
    init.DMA_DIR = (direction == SERIAL_TX) ? DMA_DIR_PeripheralDST : DMA_DIR_PeripheralSRC;
    init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    init.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    init.DMA_Mode = DMA_Mode_Normal;
    init.DMA_Priority = (direction == SERIAL_TX) ? DMA_Priority_Medium : DMA_Priority_High;
    init.DMA_M2M = DMA_M2M_Disable;
    init.DMA_BufferMode = DMA_SingleBufferMode;
    DMA_Init(dma_channel, &init);
    DMA_MuxChannelConfig((uint8_t)(controller * 8U + channel), request);
    DMA_ITConfig(dma_channel, DMA_IT_TC | DMA_IT_TE, ENABLE);
    NVIC_SetPriority(g_dma_irq[controller][channel], (direction == SERIAL_TX) ? LZPORT_SERIAL_IRQ_TX_PRIORITY 
                                                                            : LZPORT_SERIAL_IRQ_RX_PRIORITY);
    NVIC_EnableIRQ(g_dma_irq[controller][channel]);
}

static lzport_status serial_dma_bind(uint8_t port, serial_direction direction,
                                     uint8_t controller, uint8_t channel,
                                     uint8_t request)
{
    serial_dma_binding *binding;
    uint8_t owner;

    if (!serial_initialized(port) || (controller >= SERIAL_DMA_COUNT) ||
        (channel >= SERIAL_DMA_CHANNEL_COUNT) || (request == 0U) ||
        (request > SERIAL_DMA_REQUEST_MAX)) {
        return LZPORT_EINVAL;
    }

    binding = serial_binding(port, direction);
    owner = serial_owner_value(port, direction);

    /* Reject configuration if an ongoing DMA transfer is active on this binding */
    if (binding->busy != 0U) {
        return LZPORT_EBUSY;
    }

    /* Reject if target DMA channel is currently owned by another port/direction */
    if ((g_dma_owner[controller][channel] != 0U) &&
        (g_dma_owner[controller][channel] != owner)) {
        return LZPORT_EBUSY;
    }

    /* Unbind and disable any previously associated DMA channel for this port/direction */
    if (binding->controller != SERIAL_DMA_UNBOUND) {
        DMA_Cmd(g_dma_channel[binding->controller][binding->channel], DISABLE);
        g_dma_owner[binding->controller][binding->channel] = 0U;
    }

    serial_dma_channel_setup(port, direction, controller, channel, request);
    g_dma_owner[controller][channel] = owner;
    binding->controller = controller;
    binding->channel = channel;
    if (direction == SERIAL_TX) {
        USART_DMACmd(g_hw[port].usart, USART_DMAReq_Tx, ENABLE);
    } else {
        USART_DMACmd(g_hw[port].usart, USART_DMAReq_Rx, ENABLE);
        serial_idle_clear(g_hw[port].usart);
        USART_ITConfig(g_hw[port].usart, USART_IT_IDLE, ENABLE);
        NVIC_SetPriority(g_hw[port].irq, LZPORT_SERIAL_IRQ_RX_PRIORITY);
        NVIC_EnableIRQ(g_hw[port].irq);
    }
    return LZPORT_OK;
}

static lzport_status serial_start(uint8_t port, serial_direction direction,
                                  void *buffer, uint32_t len,
                                  lzport_serial_done_cb done, void *user)
{
    serial_dma_binding *binding;
    DMA_Channel_TypeDef *dma_channel;

    if (!serial_initialized(port) || (buffer == 0) || (len == 0U) ||
        (len > SERIAL_DMA_MAX_TRANSFER)) {
        return LZPORT_EINVAL;
    }
    binding = serial_binding(port, direction);
    if (binding->controller == SERIAL_DMA_UNBOUND) {
        return LZPORT_EINVAL;
    }
    if (binding->busy != 0U) {
        return LZPORT_EBUSY;
    }

    binding->busy = 1U;
    binding->len = len;
    binding->done = done;
    binding->user = user;
    dma_channel = g_dma_channel[binding->controller][binding->channel];
    DMA_Cmd(dma_channel, DISABLE);
    DMA_ClearITPendingBit(g_dma[binding->controller],
                          serial_dma_gl_flag(binding->channel));
    if (direction == SERIAL_RX) {
        serial_idle_clear(g_hw[port].usart);
    }
    dma_channel->MADDR = (uint32_t)buffer;
    DMA_SetCurrDataCounter(dma_channel, (uint16_t)len);
    DMA_Cmd(dma_channel, ENABLE);
    return LZPORT_OK;
}

lzport_status lzport_serial_init(uint8_t port, const lzport_serial_config *cfg)
{
    const serial_hw *hw;
    USART_InitTypeDef usart = {0};

    if ((port >= SERIAL_PORT_COUNT) || (cfg == 0) || (cfg->baud == 0U) ||
        ((uint32_t)cfg->tx.port >= LZPORT_GPIO_PORT_COUNT) ||
        ((uint32_t)cfg->rx.port >= LZPORT_GPIO_PORT_COUNT) ||
        ((uint32_t)cfg->tx.pin >= LZPORT_GPIO_PIN_COUNT) ||
        ((uint32_t)cfg->rx.pin >= LZPORT_GPIO_PIN_COUNT) ||
        ((uint32_t)cfg->tx.af >= LZPORT_GPIO_AF_COUNT) ||
        ((uint32_t)cfg->rx.af >= LZPORT_GPIO_AF_COUNT) ||
        ((uint32_t)cfg->data_bits >= LZPORT_SERIAL_DATA_BITS_COUNT) ||
        ((uint32_t)cfg->parity >= LZPORT_SERIAL_PARITY_COUNT) ||
        ((uint32_t)cfg->stop_bits >= LZPORT_SERIAL_STOP_BITS_COUNT)) {
        return LZPORT_EINVAL;
    }
    if (serial_initialized(port)) {
        (void)lzport_serial_dma_unbind(port);
    } else {
        serial_binding_reset(&g_state[port].tx);
        serial_binding_reset(&g_state[port].rx);
    }

    hw = &g_hw[port];
    if (port == 0U) {
        RCC_HB2PeriphClockCmd(hw->clock, ENABLE);
    } else {
        RCC_HB1PeriphClockCmd(hw->clock, ENABLE);
    }

    lzport_gpio_mode_af_output(cfg->tx.port, cfg->tx.pin, cfg->tx.af,
                               LZPORT_GPIO_SPEED_VERY_HIGH, LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_mode_af_input(cfg->rx.port, cfg->rx.pin, cfg->rx.af,
                              LZPORT_GPIO_PULL_NONE);

    USART_DeInit(hw->usart);
    usart.USART_BaudRate = cfg->baud;
    usart.USART_WordLength = (cfg->data_bits == LZPORT_SERIAL_DATA_BITS_9)
                                 ? USART_WordLength_9b
                                 : USART_WordLength_8b;
    usart.USART_StopBits = (cfg->stop_bits == LZPORT_SERIAL_STOP_BITS_2)
                              ? USART_StopBits_2
                              : USART_StopBits_1;
    usart.USART_Parity = (cfg->parity == LZPORT_SERIAL_PARITY_EVEN) 
                            ? USART_Parity_Even 
                            : (cfg->parity == LZPORT_SERIAL_PARITY_ODD)  
                            ? USART_Parity_Odd 
                            : USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(hw->usart, &usart);
    USART_Cmd(hw->usart, ENABLE);
    g_initialized |= (uint8_t)(1U << port);
    return LZPORT_OK;
}

lzport_status lzport_serial_deinit(uint8_t port)
{
    if (!serial_initialized(port)) {
        return LZPORT_EINVAL;
    }
    (void)lzport_serial_dma_unbind(port);
    USART_Cmd(g_hw[port].usart, DISABLE);
    USART_DeInit(g_hw[port].usart);
    g_initialized &= (uint8_t)~(1U << port);
    return LZPORT_OK;
}

lzport_status lzport_serial_dma_bind_tx(uint8_t port, uint8_t controller,
                                        uint8_t channel, uint8_t request)
{
    return serial_dma_bind(port, SERIAL_TX, controller, channel, request);
}

lzport_status lzport_serial_dma_bind_rx(uint8_t port, uint8_t controller,
                                        uint8_t channel, uint8_t request)
{
    return serial_dma_bind(port, SERIAL_RX, controller, channel, request);
}

lzport_status lzport_serial_dma_unbind(uint8_t port)
{
    serial_dma_binding *tx;
    serial_dma_binding *rx;

    if (!serial_initialized(port)) {
        return LZPORT_EINVAL;
    }
    tx = &g_state[port].tx;
    rx = &g_state[port].rx;
    USART_DMACmd(g_hw[port].usart, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);
    USART_ITConfig(g_hw[port].usart, USART_IT_IDLE, DISABLE);
    if (tx->controller != SERIAL_DMA_UNBOUND) {
        DMA_Cmd(g_dma_channel[tx->controller][tx->channel], DISABLE);
        DMA_ClearITPendingBit(g_dma[tx->controller], serial_dma_gl_flag(tx->channel));
        g_dma_owner[tx->controller][tx->channel] = 0U;
    }
    if (rx->controller != SERIAL_DMA_UNBOUND) {
        DMA_Cmd(g_dma_channel[rx->controller][rx->channel], DISABLE);
        DMA_ClearITPendingBit(g_dma[rx->controller], serial_dma_gl_flag(rx->channel));
        g_dma_owner[rx->controller][rx->channel] = 0U;
    }
    serial_binding_reset(tx);
    serial_binding_reset(rx);
    return LZPORT_OK;
}

lzport_status lzport_serial_write(uint8_t port, const uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if (!serial_initialized(port) || ((buf == 0) && (len != 0U))) {
        return LZPORT_EINVAL;
    }
    if ((g_state[port].tx.controller != SERIAL_DMA_UNBOUND) &&
        (g_state[port].tx.busy != 0U)) {
        return LZPORT_EBUSY;
    }
    for (i = 0U; i < len; ++i) {
        if (!serial_wait_flag(g_hw[port].usart, USART_FLAG_TXE, SERIAL_POLL_TIMEOUT)) {
            return LZPORT_ETIMEOUT;
        }
        USART_SendData(g_hw[port].usart, buf[i]);
    }
    return serial_wait_flag(g_hw[port].usart, USART_FLAG_TC, SERIAL_POLL_TIMEOUT)
               ? LZPORT_OK
               : LZPORT_ETIMEOUT;
}

lzport_status lzport_serial_read(uint8_t port, uint8_t *buf, uint32_t len,
                                 uint32_t timeout_cycles)
{
    uint32_t i;

    if (!serial_initialized(port) || ((buf == 0) && (len != 0U))) {
        return LZPORT_EINVAL;
    }
    if ((g_state[port].rx.controller != SERIAL_DMA_UNBOUND) &&
        (g_state[port].rx.busy != 0U)) {
        return LZPORT_EBUSY;
    }
    for (i = 0U; i < len; ++i) {
        if (!serial_wait_flag(g_hw[port].usart, USART_FLAG_RXNE, timeout_cycles)) {
            return LZPORT_ETIMEOUT;
        }
        buf[i] = (uint8_t)USART_ReceiveData(g_hw[port].usart);
    }
    return LZPORT_OK;
}

lzport_status lzport_serial_write_async(uint8_t port, const uint8_t *buf,
                                        uint32_t len, lzport_serial_done_cb done,
                                        void *user)
{
    return serial_start(port, SERIAL_TX, (void *)buf, len, done, user);
}

lzport_status lzport_serial_read_async(uint8_t port, uint8_t *buf,
                                       uint32_t len, lzport_serial_done_cb done,
                                       void *user)
{
    return serial_start(port, SERIAL_RX, buf, len, done, user);
}

lzport_status lzport_serial_write_abort(uint8_t port)
{
    serial_dma_binding *binding;

    if (!serial_initialized(port)) {
        return LZPORT_EINVAL;
    }

    binding = serial_binding(port, SERIAL_TX);
    if ((binding->busy == 0U) || (binding->controller == SERIAL_DMA_UNBOUND)) {
        return LZPORT_EINVAL;
    }
    binding->busy = 0;
    DMA_Cmd(g_dma_channel[binding->controller][binding->channel], DISABLE);
    binding->len = 0U;
    binding->done = 0;
    binding->user = 0;

    return LZPORT_OK;
}

lzport_status lzport_serial_read_abort(uint8_t port)
{
    serial_dma_binding *binding;

    if (!serial_initialized(port)) {
        return LZPORT_EINVAL;
    }

    binding = serial_binding(port, SERIAL_RX);
    if ((binding->busy == 0U) || (binding->controller == SERIAL_DMA_UNBOUND)) {
        return LZPORT_EINVAL;
    }
    binding->busy = 0;
    DMA_Cmd(g_dma_channel[binding->controller][binding->channel], DISABLE);
    binding->len = 0U;
    binding->done = 0;
    binding->user = 0;

    return LZPORT_OK;
}

void lzport_serial_dma_irq(uint8_t controller, uint8_t channel)
{
    uint8_t owner;
    uint8_t value;
    uint8_t port;
    serial_direction direction;
    serial_dma_binding *binding;
    DMA_Channel_TypeDef *dma_channel;
    lzport_status status;
    uint32_t remaining;

    if ((controller >= SERIAL_DMA_COUNT) || (channel >= SERIAL_DMA_CHANNEL_COUNT)) {
        return;
    }
    if ((DMA_GetITStatus(g_dma[controller], serial_dma_tc_flag(channel)) == RESET) &&
        (DMA_GetITStatus(g_dma[controller], serial_dma_te_flag(channel)) == RESET)) {
        return;
    }
    status = (DMA_GetITStatus(g_dma[controller], serial_dma_te_flag(channel)) != RESET)
                 ? LZPORT_EIO
                 : LZPORT_OK;
    DMA_ClearITPendingBit(g_dma[controller], serial_dma_gl_flag(channel));

    owner = g_dma_owner[controller][channel];
    if (owner == 0U) {
        return;
    }
    value = (uint8_t)(owner - 1U);
    port = value / 2U;
    direction = ((value & 1U) == 0U) ? SERIAL_TX : SERIAL_RX;
    binding = serial_binding(port, direction);
    if ((binding->busy == 0U) || (binding->controller != controller) ||
        (binding->channel != channel)) {
        return;
    }

    dma_channel = g_dma_channel[controller][channel];
    DMA_Cmd(dma_channel, DISABLE);
    remaining = DMA_GetCurrDataCounter(dma_channel);
    serial_finish(port, direction, status,
                  (binding->len >= remaining) ? binding->len - remaining : 0U);
}

void lzport_serial_usart_irq(uint8_t port)
{
    serial_dma_binding *rx;
    DMA_Channel_TypeDef *dma_channel;
    uint32_t remaining;

    if (port >= SERIAL_PORT_COUNT) {
        return;
    }
    if (USART_GetITStatus(g_hw[port].usart, USART_IT_IDLE) == RESET) {
        return;
    }

    rx = &g_state[port].rx;
    if ((rx->controller == SERIAL_DMA_UNBOUND) || (rx->busy == 0U)) {
        serial_idle_clear(g_hw[port].usart);
        return;
    }
    dma_channel = g_dma_channel[rx->controller][rx->channel];
    DMA_Cmd(dma_channel, DISABLE);
    serial_idle_clear(g_hw[port].usart);
    DMA_ClearITPendingBit(g_dma[rx->controller], serial_dma_gl_flag(rx->channel));
    remaining = DMA_GetCurrDataCounter(dma_channel);
    serial_finish(port, SERIAL_RX, LZPORT_OK,
                  (rx->len >= remaining) ? rx->len - remaining : 0U);
}

#ifndef LZPORT_SERIAL_EXTERNAL_IRQ_HANDLERS
#define SERIAL_DMA_ISR(name, controller, channel) \
    void name(void) __attribute__((interrupt("WCH-Interrupt-fast"))); \
    void name(void) { lzport_serial_dma_irq((controller), (channel)); }

#define SERIAL_USART_ISR(name, port) \
    void name(void) __attribute__((interrupt("WCH-Interrupt-fast"))); \
    void name(void) { lzport_serial_usart_irq((port)); }

SERIAL_DMA_ISR(DMA1_Channel1_IRQHandler, 0U, 0U)
SERIAL_DMA_ISR(DMA1_Channel2_IRQHandler, 0U, 1U)
SERIAL_DMA_ISR(DMA1_Channel3_IRQHandler, 0U, 2U)
SERIAL_DMA_ISR(DMA1_Channel4_IRQHandler, 0U, 3U)
SERIAL_DMA_ISR(DMA1_Channel5_IRQHandler, 0U, 4U)
SERIAL_DMA_ISR(DMA1_Channel6_IRQHandler, 0U, 5U)
SERIAL_DMA_ISR(DMA1_Channel7_IRQHandler, 0U, 6U)
SERIAL_DMA_ISR(DMA1_Channel8_IRQHandler, 0U, 7U)
SERIAL_DMA_ISR(DMA2_Channel1_IRQHandler, 1U, 0U)
SERIAL_DMA_ISR(DMA2_Channel2_IRQHandler, 1U, 1U)
SERIAL_DMA_ISR(DMA2_Channel3_IRQHandler, 1U, 2U)
SERIAL_DMA_ISR(DMA2_Channel4_IRQHandler, 1U, 3U)
SERIAL_DMA_ISR(DMA2_Channel5_IRQHandler, 1U, 4U)
SERIAL_DMA_ISR(DMA2_Channel6_IRQHandler, 1U, 5U)
SERIAL_DMA_ISR(DMA2_Channel7_IRQHandler, 1U, 6U)
SERIAL_DMA_ISR(DMA2_Channel8_IRQHandler, 1U, 7U)

SERIAL_USART_ISR(USART1_IRQHandler, 0U)
SERIAL_USART_ISR(USART2_IRQHandler, 1U)
SERIAL_USART_ISR(USART3_IRQHandler, 2U)
SERIAL_USART_ISR(USART4_IRQHandler, 3U)
SERIAL_USART_ISR(USART5_IRQHandler, 4U)
SERIAL_USART_ISR(USART6_IRQHandler, 5U)
SERIAL_USART_ISR(USART7_IRQHandler, 6U)
SERIAL_USART_ISR(USART8_IRQHandler, 7U)
#endif
