#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/status.h"

typedef enum
{
    LZPORT_GPIO_A = 0,
    LZPORT_GPIO_B,
    LZPORT_GPIO_C,
    LZPORT_GPIO_D,
    LZPORT_GPIO_E,
    LZPORT_GPIO_F,
    LZPORT_GPIO_PORT_COUNT
} lzport_gpio_port;

typedef enum
{
    LZPORT_GPIO_PIN_0 = 0,
    LZPORT_GPIO_PIN_1,
    LZPORT_GPIO_PIN_2,
    LZPORT_GPIO_PIN_3,
    LZPORT_GPIO_PIN_4,
    LZPORT_GPIO_PIN_5,
    LZPORT_GPIO_PIN_6,
    LZPORT_GPIO_PIN_7,
    LZPORT_GPIO_PIN_8,
    LZPORT_GPIO_PIN_9,
    LZPORT_GPIO_PIN_10,
    LZPORT_GPIO_PIN_11,
    LZPORT_GPIO_PIN_12,
    LZPORT_GPIO_PIN_13,
    LZPORT_GPIO_PIN_14,
    LZPORT_GPIO_PIN_15,
    LZPORT_GPIO_PIN_COUNT
} lzport_gpio_pin;

typedef enum
{
    LZPORT_GPIO_AF_0 = 0,
    LZPORT_GPIO_AF_1,
    LZPORT_GPIO_AF_2,
    LZPORT_GPIO_AF_3,
    LZPORT_GPIO_AF_4,
    LZPORT_GPIO_AF_5,
    LZPORT_GPIO_AF_6,
    LZPORT_GPIO_AF_7,
    LZPORT_GPIO_AF_8,
    LZPORT_GPIO_AF_9,
    LZPORT_GPIO_AF_10,
    LZPORT_GPIO_AF_11,
    LZPORT_GPIO_AF_12,
    LZPORT_GPIO_AF_13,
    LZPORT_GPIO_AF_14,
    LZPORT_GPIO_AF_15,
    LZPORT_GPIO_AF_COUNT
} lzport_gpio_af;

typedef enum
{
    LZPORT_GPIO_PULL_NONE = 0,
    LZPORT_GPIO_PULL_UP,
    LZPORT_GPIO_PULL_DOWN,
    LZPORT_GPIO_PULL_COUNT
} lzport_gpio_pull;

typedef enum
{
    LZPORT_GPIO_SPEED_LOW = 0,
    LZPORT_GPIO_SPEED_MEDIUM,
    LZPORT_GPIO_SPEED_HIGH,
    LZPORT_GPIO_SPEED_VERY_HIGH,
    LZPORT_GPIO_SPEED_COUNT
} lzport_gpio_speed;

typedef enum
{
    LZPORT_GPIO_PUSH_PULL = 0,
    LZPORT_GPIO_OPEN_DRAIN,
    LZPORT_GPIO_OTYPE_COUNT
} lzport_gpio_otype;

typedef enum
{
    LZPORT_GPIO_LOW = 0,
    LZPORT_GPIO_HIGH,
    LZPORT_GPIO_LEVEL_COUNT
} lzport_gpio_level;

typedef enum
{
    LZPORT_GPIO_IRQ_RISING = 0,
    LZPORT_GPIO_IRQ_FALLING,
    LZPORT_GPIO_IRQ_BOTH,
    LZPORT_GPIO_IRQ_EDGE_COUNT
} lzport_gpio_irq_edge;

typedef void (*lzport_gpio_irq_cb)(lzport_gpio_port port,
                                   lzport_gpio_pin pin, void *user);

void lzport_gpio_mode_input(lzport_gpio_port port, lzport_gpio_pin pin, lzport_gpio_pull pull);
void lzport_gpio_mode_analog(lzport_gpio_port port, lzport_gpio_pin pin);
void lzport_gpio_mode_output(lzport_gpio_port port, lzport_gpio_pin pin, lzport_gpio_speed speed, lzport_gpio_otype otype);
void lzport_gpio_mode_af_input(lzport_gpio_port port, lzport_gpio_pin pin, lzport_gpio_af af, lzport_gpio_pull pull);
void lzport_gpio_mode_af_output(lzport_gpio_port port, lzport_gpio_pin pin, lzport_gpio_af af, lzport_gpio_speed speed, lzport_gpio_otype otype);
void lzport_gpio_set(lzport_gpio_port port, lzport_gpio_pin pin);
void lzport_gpio_reset(lzport_gpio_port port, lzport_gpio_pin pin);
void lzport_gpio_toggle(lzport_gpio_port port, lzport_gpio_pin pin);
void lzport_gpio_write(lzport_gpio_port port, lzport_gpio_pin pin, lzport_gpio_level level);
lzport_gpio_level lzport_gpio_read(lzport_gpio_port port, lzport_gpio_pin pin);
lzport_status lzport_gpio_irq_attach(lzport_gpio_port port, lzport_gpio_pin pin, lzport_gpio_irq_edge edge, lzport_gpio_irq_cb callback, void *user);
lzport_status lzport_gpio_irq_detach(lzport_gpio_port port, lzport_gpio_pin pin);

#ifdef __cplusplus
}
#endif
