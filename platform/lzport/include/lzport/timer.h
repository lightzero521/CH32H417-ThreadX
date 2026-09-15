#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "lzport/status.h"

typedef enum
{
    LZPORT_TIMER_0 = 0,
    LZPORT_TIMER_1,
    LZPORT_TIMER_COUNT
} lzport_timer;

typedef enum
{
    LZPORT_TIMER_SINGLE_SHOT = 0,
    LZPORT_TIMER_REPEATING,
    LZPORT_TIMER_MODE_COUNT
} lzport_timer_mode;

/* Callback runs in interrupt context. */
typedef void (*lzport_timer_cb)(lzport_timer timer, void *user);

typedef struct
{
    uint32_t interval_us;
    lzport_timer_mode mode;
    lzport_timer_cb callback;
    void *user;
} lzport_timer_config;

lzport_status lzport_timer_init(lzport_timer timer, const lzport_timer_config *cfg);
lzport_status lzport_timer_deinit(lzport_timer timer);
lzport_status lzport_timer_start(lzport_timer timer);
lzport_status lzport_timer_stop(lzport_timer timer);
bool lzport_timer_is_active(lzport_timer timer);

#ifdef __cplusplus
}
#endif
