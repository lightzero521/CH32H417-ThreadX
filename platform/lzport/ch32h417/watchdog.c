#include "lzport/watchdog.h"

#include "ch32h417_iwdg.h"

#define WATCHDOG_LSI_HZ       40000U
#define WATCHDOG_RELOAD_MAX   4095U
#define WATCHDOG_POLL_TIMEOUT 100000U

static uint8_t g_started;

lzport_status lzport_watchdog_init(uint32_t timeout_ms)
{
    static const uint16_t divider[] = {4U, 8U, 16U, 32U, 64U, 128U, 256U};
    uint64_t ticks;
    uint32_t counts;
    uint32_t timeout;
    uint8_t prescaler;

    if ((timeout_ms == 0U) || (timeout_ms > 26214U) || (g_started != 0U)) {
        return LZPORT_EINVAL;
    }
    ticks = ((uint64_t)timeout_ms * WATCHDOG_LSI_HZ + 999U) / 1000U;
    for (prescaler = 0U; prescaler < 7U; ++prescaler) {
        counts = (uint32_t)((ticks + divider[prescaler] - 1U) /
                            divider[prescaler]);
        if (counts <= (WATCHDOG_RELOAD_MAX + 1U)) {
            break;
        }
    }
    if (prescaler >= 7U) {
        return LZPORT_EINVAL;
    }

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(prescaler);
    IWDG_SetReload((uint16_t)(counts - 1U));
    timeout = WATCHDOG_POLL_TIMEOUT;
    while (((IWDG_GetFlagStatus(IWDG_FLAG_PVU) != RESET) ||
            (IWDG_GetFlagStatus(IWDG_FLAG_RVU) != RESET)) &&
           (timeout-- != 0U)) {
    }
    if ((IWDG_GetFlagStatus(IWDG_FLAG_PVU) != RESET) ||
        (IWDG_GetFlagStatus(IWDG_FLAG_RVU) != RESET)) {
        return LZPORT_ETIMEOUT;
    }
    IWDG_ReloadCounter();
    IWDG_Enable();
    g_started = 1U;
    return LZPORT_OK;
}

lzport_status lzport_watchdog_feed(void)
{
    if (g_started == 0U) {
        return LZPORT_EINVAL;
    }
    IWDG_ReloadCounter();
    return LZPORT_OK;
}
