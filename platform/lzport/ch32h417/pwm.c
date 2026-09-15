#include "lzport/pwm.h"

lzport_status lzport_pwm_init(lzport_pwm_timer timer,
                              lzport_pwm_channel channel,
                              const lzport_pwm_config *cfg)
{
    (void)timer;
    (void)channel;
    (void)cfg;
    return LZPORT_EIO;
}

lzport_status lzport_pwm_deinit(lzport_pwm_timer timer,
                                lzport_pwm_channel channel)
{
    (void)timer;
    (void)channel;
    return LZPORT_EIO;
}

lzport_status lzport_pwm_start(lzport_pwm_timer timer,
                               lzport_pwm_channel channel)
{
    (void)timer;
    (void)channel;
    return LZPORT_EIO;
}

lzport_status lzport_pwm_stop(lzport_pwm_timer timer,
                              lzport_pwm_channel channel)
{
    (void)timer;
    (void)channel;
    return LZPORT_EIO;
}

lzport_status lzport_pwm_set_duty(lzport_pwm_timer timer,
                                  lzport_pwm_channel channel,
                                  uint16_t duty_permille)
{
    (void)timer;
    (void)channel;
    (void)duty_permille;
    return LZPORT_EIO;
}
