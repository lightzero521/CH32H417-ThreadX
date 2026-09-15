#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/gpio.h"
#include "lzport/status.h"

typedef enum
{
    LZPORT_PWM_TIM1 = 0,
    LZPORT_PWM_TIM2,
    LZPORT_PWM_TIM3,
    LZPORT_PWM_TIM4,
    LZPORT_PWM_TIM5,
    LZPORT_PWM_TIM8,
    LZPORT_PWM_TIM9,
    LZPORT_PWM_TIM10,
    LZPORT_PWM_TIM11,
    LZPORT_PWM_TIM12,
    LZPORT_PWM_TIMER_COUNT
} lzport_pwm_timer;

typedef enum
{
    LZPORT_PWM_CHANNEL_1 = 0,
    LZPORT_PWM_CHANNEL_2,
    LZPORT_PWM_CHANNEL_3,
    LZPORT_PWM_CHANNEL_4,
    LZPORT_PWM_CHANNEL_COUNT
} lzport_pwm_channel;

typedef struct
{
    uint32_t frequency_hz;
    uint16_t duty_permille;
    lzport_gpio_port port;
    lzport_gpio_pin pin;
    lzport_gpio_af af;
} lzport_pwm_config;

lzport_status lzport_pwm_init(lzport_pwm_timer timer,
                              lzport_pwm_channel channel,
                              const lzport_pwm_config *cfg);
lzport_status lzport_pwm_deinit(lzport_pwm_timer timer,
                                lzport_pwm_channel channel);
lzport_status lzport_pwm_start(lzport_pwm_timer timer,
                               lzport_pwm_channel channel);
lzport_status lzport_pwm_stop(lzport_pwm_timer timer,
                              lzport_pwm_channel channel);
lzport_status lzport_pwm_set_duty(lzport_pwm_timer timer,
                                  lzport_pwm_channel channel,
                                  uint16_t duty_permille);

#ifdef __cplusplus
}
#endif
