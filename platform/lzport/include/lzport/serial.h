#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/gpio.h"
#include "lzport/status.h"

typedef enum
{
    LZPORT_SERIAL_DATA_BITS_8 = 0,
    LZPORT_SERIAL_DATA_BITS_9, /* Reserved: do not use with the current byte APIs. */
    LZPORT_SERIAL_DATA_BITS_COUNT
} lzport_serial_data_bits;

typedef enum
{
    LZPORT_SERIAL_PARITY_NONE = 0,
    LZPORT_SERIAL_PARITY_EVEN, /* Reserved: parity support is not complete yet. */
    LZPORT_SERIAL_PARITY_ODD,  /* Reserved: parity support is not complete yet. */
    LZPORT_SERIAL_PARITY_COUNT
} lzport_serial_parity;

typedef enum
{
    LZPORT_SERIAL_STOP_BITS_1 = 0,
    LZPORT_SERIAL_STOP_BITS_2,
    LZPORT_SERIAL_STOP_BITS_COUNT
} lzport_serial_stop_bits;

typedef struct
{
    lzport_gpio_port port;
    lzport_gpio_pin pin;
    lzport_gpio_af af;
} lzport_serial_pin;

typedef struct
{
    /* Current uint8_t APIs support 8 data bits without parity only.
     * 9-bit/parity modes require corrected word-length handling and uint16_t APIs. */
    uint32_t baud;
    lzport_serial_data_bits data_bits;
    lzport_serial_parity parity;
    lzport_serial_stop_bits stop_bits;
    lzport_serial_pin tx;
    lzport_serial_pin rx;
} lzport_serial_config;

/* done runs in ISR context. */
typedef void (*lzport_serial_done_cb)(uint8_t port, lzport_status status, uint32_t actual, void *user);

/* port 0..7 select USART1..USART8. Reinit aborts pending async transfers. */
lzport_status lzport_serial_init(uint8_t port, const lzport_serial_config *cfg);
/* Deinit aborts pending async transfers without invoking their callbacks. */
lzport_status lzport_serial_deinit(uint8_t port);

/* controller: 0=DMA1, 1=DMA2; channel: 0..7; request: DMAMUX request 1..123. */
lzport_status lzport_serial_dma_bind_tx(uint8_t port, uint8_t controller, uint8_t channel, uint8_t request);
lzport_status lzport_serial_dma_bind_rx(uint8_t port, uint8_t controller, uint8_t channel, uint8_t request);
/* Unbind aborts pending async transfers without invoking their callbacks. */
lzport_status lzport_serial_dma_unbind(uint8_t port);
lzport_status lzport_serial_write(uint8_t port, const uint8_t *buf, uint32_t len);
lzport_status lzport_serial_read(uint8_t port, uint8_t *buf, uint32_t len, uint32_t timeout_cycles);
lzport_status lzport_serial_write_async(uint8_t port, const uint8_t *buf, uint32_t len, lzport_serial_done_cb done, void *user);
lzport_status lzport_serial_read_async(uint8_t port, uint8_t *buf, uint32_t len, lzport_serial_done_cb done, void *user);
/** Best-effort abort for UART DMA; pending reception/interrupt may still fire. */
lzport_status lzport_serial_write_abort(uint8_t port);
lzport_status lzport_serial_read_abort(uint8_t port);

#ifdef __cplusplus
}
#endif
