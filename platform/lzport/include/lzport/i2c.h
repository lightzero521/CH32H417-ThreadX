#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/gpio.h"
#include "lzport/status.h"

typedef struct
{
    lzport_gpio_port port;
    lzport_gpio_pin pin;
    lzport_gpio_af af;
} lzport_i2c_pin;

typedef struct
{
    uint32_t clock_hz;
    lzport_i2c_pin scl;
    lzport_i2c_pin sda;
} lzport_i2c_config;

/* inst 0..3 select I2C1..I2C4. External pull-ups are required. */
lzport_status lzport_i2c_init(uint8_t inst, const lzport_i2c_config *cfg);
lzport_status lzport_i2c_deinit(uint8_t inst);
lzport_status lzport_i2c_write(uint8_t inst, uint8_t addr7, const uint8_t *buf, uint32_t len, uint32_t timeout_cycles);
lzport_status lzport_i2c_read(uint8_t inst, uint8_t addr7, uint8_t *buf, uint32_t len, uint32_t timeout_cycles);
/* Write followed by a repeated START and read, with no STOP in between. */
lzport_status lzport_i2c_write_read(uint8_t inst, uint8_t addr7,
                                    const uint8_t *tx, uint32_t tx_len,
                                    uint8_t *rx, uint32_t rx_len,
                                    uint32_t timeout_cycles);

#ifdef __cplusplus
}
#endif
