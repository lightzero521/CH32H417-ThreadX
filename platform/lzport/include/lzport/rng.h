#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "lzport/status.h"

lzport_status lzport_rng_init(void);
lzport_status lzport_rng_deinit(void);
lzport_status lzport_rng_get(uint32_t *value);

#ifdef __cplusplus
}
#endif
