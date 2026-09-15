#include "lzport/rtc.h"

#include "ch32h417_pwr.h"
#include "ch32h417_rcc.h"
#include "ch32h417_rtc.h"

#define RTC_LSE_TIMEOUT_COUNT 250U
#define RTC_LSI_TIMEOUT_COUNT 250U
#define RTC_SECONDS_DAY       86400U
#define RTC_DELAY_LOOPS_MS    25000U /* 100 MHz, about 4 cycles per loop. */

static uint8_t g_initialized;
static uint32_t g_prescaler;

static void delay_ms(uint32_t ms)
{
    while (ms-- != 0U) {
        uint32_t loops = RTC_DELAY_LOOPS_MS;

        while (loops-- != 0U) {
            __NOP();
        }
    }
}

static int is_leap(uint16_t year)
{
    return ((year % 4U) == 0U) &&
           (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

static uint8_t month_days(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U,
    };

    return (uint8_t)(days[month - 1U] +
                     (((month == 2U) && is_leap(year)) ? 1U : 0U));
}

static int datetime_valid(const lzport_rtc_datetime *value)
{
    return (value != 0) && (value->year >= 1970U) &&
           (value->year <= 2099U) && (value->month >= 1U) &&
           (value->month <= 12U) && (value->day >= 1U) &&
           (value->day <= month_days(value->year, value->month)) &&
           (value->hour < 24U) && (value->minute < 60U) &&
           (value->second < 60U);
}

static uint32_t datetime_to_seconds(const lzport_rtc_datetime *value)
{
    uint32_t days = 0U;
    uint16_t year;
    uint8_t month;

    for (year = 1970U; year < value->year; ++year) {
        days += is_leap(year) ? 366U : 365U;
    }
    for (month = 1U; month < value->month; ++month) {
        days += month_days(value->year, month);
    }
    days += (uint32_t)value->day - 1U;
    return (days * RTC_SECONDS_DAY) + ((uint32_t)value->hour * 3600U) +
           ((uint32_t)value->minute * 60U) + value->second;
}

static void seconds_to_datetime(uint32_t seconds, lzport_rtc_datetime *out)
{
    uint32_t days = seconds / RTC_SECONDS_DAY;
    uint32_t day_seconds = seconds % RTC_SECONDS_DAY;
    uint32_t epoch_days = days;
    uint16_t year = 1970U;
    uint8_t month = 1U;
    uint16_t days_this_year;

    while (1) {
        days_this_year = is_leap(year) ? 366U : 365U;
        if (days < days_this_year) {
            break;
        }
        days -= days_this_year;
        ++year;
    }
    while (days >= month_days(year, month)) {
        days -= month_days(year, month);
        ++month;
    }
    out->year = year;
    out->month = month;
    out->day = (uint8_t)(days + 1U);
    out->weekday = (uint8_t)(((epoch_days + 3U) % 7U) + 1U);
    out->hour = (uint8_t)(day_seconds / 3600U);
    out->minute = (uint8_t)((day_seconds % 3600U) / 60U);
    out->second = (uint8_t)(day_seconds % 60U);
}

bool lzport_rtc_is_configured(void)
{
    return ((RCC->BDCTLR & RCC_RTCSEL) != RCC_RTCSEL_NOCLOCK) &&
           ((RCC->BDCTLR & RCC_RTCEN) != 0U);
}

lzport_status lzport_rtc_init(void)
{
    uint16_t timeout = 0U;
    uint32_t source;
    lzport_rtc_datetime initial = {
        2026U, 1U, 1U, 4U, 0U, 0U, 0U, 0U,
    };

    g_initialized = 0U;
    RCC_HB1PeriphClockCmd(RCC_HB1Periph_PWR | RCC_HB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);
    RTC_ClearITPendingBit(RTC_IT_ALR);
    RTC_ClearITPendingBit(RTC_IT_SEC);

    source = RCC->BDCTLR & RCC_RTCSEL;
    if (lzport_rtc_is_configured()) {
        if (source == RCC_RTCSEL_LSE) {
            while ((RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) && (timeout++ < RTC_LSE_TIMEOUT_COUNT)) {
                delay_ms(10U);
            }
        } else if (source == RCC_RTCSEL_LSI) {
            RCC_LSICmd(ENABLE);
            while ((RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) && (timeout++ < RTC_LSI_TIMEOUT_COUNT)) {
                delay_ms(10U);
            }
        }
        if (((source == RCC_RTCSEL_LSE) && (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET)) ||
            ((source == RCC_RTCSEL_LSI) && (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == SET))) {
            RTC_WaitForSynchro();
            RTC_WaitForLastTask();
            g_prescaler = ((uint32_t)(RTC->PSCRH & 0x0FU) << 16) | RTC->PSCRL;
            g_initialized = 1U;
            return LZPORT_OK;
        }
    }

    RCC_BackupResetCmd(ENABLE);
    RCC_BackupResetCmd(DISABLE);
    if (source != RCC_RTCSEL_LSE) {
        timeout = 0U;
        RCC_LSEConfig(RCC_LSE_ON);
        while ((RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) && (timeout++ < RTC_LSE_TIMEOUT_COUNT)) {
            delay_ms(10U);
        }
    }

    if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET) {
        source = RCC_RTCCLKSource_LSE;
        g_prescaler = 32767U;
    } else {
        RCC_LSEConfig(RCC_LSE_OFF);
        RCC_LSICmd(ENABLE);
        timeout = 0U;
        while ((RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) && (timeout++ < RTC_LSI_TIMEOUT_COUNT)) {
            delay_ms(10U);
        }
        if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {
            return LZPORT_ETIMEOUT;
        }
        source = RCC_RTCCLKSource_LSI;
        g_prescaler = 40000U;
    }

    RCC_RTCCLKConfig(source);
#if defined(Core_V5F)
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
#endif
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForLastTask();
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();
    RTC_SetPrescaler(g_prescaler);
    RTC_WaitForLastTask();

    g_initialized = 1U;
    if (lzport_rtc_set(&initial) != LZPORT_OK) {
        g_initialized = 0U;
        return LZPORT_EIO;
    }
    return LZPORT_OK;
}

lzport_status lzport_rtc_get(lzport_rtc_datetime *out)
{
    uint32_t first;
    uint32_t second;
    uint32_t divider;

    if ((g_initialized == 0U) || (out == 0)) {
        return LZPORT_EINVAL;
    }
    do {
        first = RTC_GetCounter();
        divider = RTC_GetDivider();
        second = RTC_GetCounter();
    } while (first != second);

    seconds_to_datetime(first, out);
    if ((g_prescaler != 0U) && (divider <= g_prescaler)) {
        out->millisecond = (uint16_t)(((g_prescaler - divider) * 1000U) /
                                      (g_prescaler + 1U));
    } else {
        out->millisecond = 0U;
    }
    return LZPORT_OK;
}

lzport_status lzport_rtc_set(const lzport_rtc_datetime *value)
{
    if ((g_initialized == 0U) || !datetime_valid(value)) {
        return LZPORT_EINVAL;
    }
    RTC_WaitForLastTask();
    RTC_SetCounter(datetime_to_seconds(value));
    RTC_WaitForLastTask();
    return LZPORT_OK;
}
