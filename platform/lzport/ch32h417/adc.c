#include "lzport/adc.h"

lzport_status lzport_adc_init(lzport_adc adc, const lzport_adc_config *cfg)
{
    (void)adc;
    (void)cfg;
    return LZPORT_EIO;
}

lzport_status lzport_adc_deinit(lzport_adc adc)
{
    (void)adc;
    return LZPORT_EIO;
}

lzport_status lzport_adc_read(lzport_adc adc, uint16_t *value,
                              uint32_t timeout_cycles)
{
    (void)adc;
    (void)value;
    (void)timeout_cycles;
    return LZPORT_EIO;
}
