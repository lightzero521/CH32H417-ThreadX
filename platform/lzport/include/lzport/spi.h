#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/gpio.h"
#include "lzport/status.h"

typedef enum
{
    LZPORT_SPI_MODE_0 = 0,
    LZPORT_SPI_MODE_1,
    LZPORT_SPI_MODE_2,
    LZPORT_SPI_MODE_3,
    LZPORT_SPI_MODE_COUNT
} lzport_spi_mode;

typedef struct
{
    lzport_gpio_port port;
    lzport_gpio_pin pin;
    lzport_gpio_af af;
} lzport_spi_pin;

typedef struct
{
    uint32_t max_hz;
    lzport_spi_mode mode;
    lzport_spi_pin sck;
    lzport_spi_pin miso;
    lzport_spi_pin mosi;
} lzport_spi_config;

/* inst 0..3 select SPI1..SPI4. Chip select remains application-controlled. */
lzport_status lzport_spi_init(uint8_t inst, const lzport_spi_config *cfg);
lzport_status lzport_spi_deinit(uint8_t inst);
/* tx=NULL sends 0xFF; rx=NULL discards received data. */
lzport_status lzport_spi_xfer(uint8_t inst, const uint8_t *tx, uint8_t *rx, uint32_t len);

#ifdef __cplusplus
}
#endif
