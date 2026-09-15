#include "lzport/rng.h"

#include "ch32h417_rcc.h"
#include "ch32h417_rng.h"

#define RNG_POLL_TIMEOUT 100000U

static uint8_t g_initialized;

lzport_status lzport_rng_init(void)
{
    RCC_HBPeriphClockCmd(RCC_HBPeriph_RNG, ENABLE);
    RNG_Cmd(DISABLE);
    RNG_ClearFlag(RNG_FLAG_CECS | RNG_FLAG_SECS);
    RNG_Cmd(ENABLE);
    g_initialized = 1U;
    return LZPORT_OK;
}

lzport_status lzport_rng_deinit(void)
{
    if (g_initialized == 0U) {
        return LZPORT_EINVAL;
    }
    RNG_Cmd(DISABLE);
    RCC_HBPeriphClockCmd(RCC_HBPeriph_RNG, DISABLE);
    g_initialized = 0U;
    return LZPORT_OK;
}

lzport_status lzport_rng_get(uint32_t *value)
{
    uint32_t timeout = RNG_POLL_TIMEOUT;
    uint32_t status;

    if ((g_initialized == 0U) || (value == 0)) {
        return LZPORT_EINVAL;
    }
    while (timeout-- != 0U) {
        status = RNG->SR;
        if ((status & (RNG_SR_CECS | RNG_SR_SECS)) != 0U) {
            RNG_ClearFlag(RNG_FLAG_CECS | RNG_FLAG_SECS);
            RNG_Cmd(DISABLE);
            RNG_Cmd(ENABLE);
            return LZPORT_EIO;
        }
        if ((status & RNG_SR_DRDY) != 0U) {
            *value = RNG_GetRandomNumber();
            return LZPORT_OK;
        }
    }
    return LZPORT_ETIMEOUT;
}
