#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "lzport/status.h"

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday; /* 1=Monday .. 7=Sunday */
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
} lzport_rtc_datetime;

lzport_status lzport_rtc_init(void);
bool lzport_rtc_is_configured(void);
lzport_status lzport_rtc_get(lzport_rtc_datetime *out);
lzport_status lzport_rtc_set(const lzport_rtc_datetime *value);

#ifdef __cplusplus
}
#endif
