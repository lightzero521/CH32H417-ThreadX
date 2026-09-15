#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/gpio.h"
#include "lzport/status.h"

typedef enum
{
    LZPORT_ADC_1 = 0,
    LZPORT_ADC_2,
    LZPORT_ADC_COUNT
} lzport_adc;

typedef enum
{
    LZPORT_ADC_SAMPLE_0 = 0,
    LZPORT_ADC_SAMPLE_1,
    LZPORT_ADC_SAMPLE_2,
    LZPORT_ADC_SAMPLE_3,
    LZPORT_ADC_SAMPLE_4,
    LZPORT_ADC_SAMPLE_5,
    LZPORT_ADC_SAMPLE_6,
    LZPORT_ADC_SAMPLE_7,
    LZPORT_ADC_SAMPLE_COUNT
} lzport_adc_sample_time;

typedef struct
{
    uint8_t channel; /* 0..15 external, 16 temperature, 17 VREFINT. */
    lzport_adc_sample_time sample_time;
    lzport_gpio_port port; /* Ignored for internal channels. */
    lzport_gpio_pin pin;   /* Ignored for internal channels. */
} lzport_adc_config;

lzport_status lzport_adc_init(lzport_adc adc, const lzport_adc_config *cfg);
lzport_status lzport_adc_deinit(lzport_adc adc);
lzport_status lzport_adc_read(lzport_adc adc, uint16_t *value,
                              uint32_t timeout_cycles);

#ifdef __cplusplus
}
#endif
