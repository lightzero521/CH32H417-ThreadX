#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/status.h"

/* Uses the nominal 40 kHz LSI. Valid timeout range is 1..26214 ms. */
lzport_status lzport_watchdog_init(uint32_t timeout_ms);
lzport_status lzport_watchdog_feed(void);

#ifdef __cplusplus
}
#endif
