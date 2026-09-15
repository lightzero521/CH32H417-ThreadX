#include "lzport/timer.h"

#include "ch32h417_rcc.h"
#include "ch32h417_tim.h"

#define TIMER_IRQ_PRIORITY   0xA0U

typedef struct
{
    TIM_TypeDef *reg;
    uint32_t clock;
    IRQn_Type irq;
} timer_hw;

typedef struct
{
    lzport_timer_cb callback;
    void *user;
    uint8_t configured;
    uint8_t active;
    uint8_t repeating;
} timer_state;

/* Logical timers are deliberately platform-neutral; CH32H417 uses TIM6/TIM7. */
static const timer_hw g_hw[LZPORT_TIMER_COUNT] = {
    {TIM6, RCC_HB1Periph_TIM6, TIM6_IRQn},
    {TIM7, RCC_HB1Periph_TIM7, TIM7_IRQn},
};

static timer_state g_state[LZPORT_TIMER_COUNT];

static int timebase_calculate(uint32_t interval_us, uint16_t *prescaler,
                              uint16_t *period)
{
    RCC_ClocksTypeDef clocks;
    uint64_t ticks;
    uint64_t divider;
    uint64_t counts;

    RCC_GetClocksFreq(&clocks);
    ticks = (((uint64_t)clocks.HCLK_Frequency * interval_us) + 999999U) /
            1000000U;
    if (ticks == 0U) {
        ticks = 1U;
    }
    divider = (ticks + 65535U) / 65536U;
    if (divider > 65536U) {
        return 0;
    }
    counts = (ticks + divider - 1U) / divider;
    *prescaler = (uint16_t)(divider - 1U);
    *period = (uint16_t)(counts - 1U);
    return 1;
}

lzport_status lzport_timer_init(lzport_timer timer,
                                const lzport_timer_config *cfg)
{
    TIM_TimeBaseInitTypeDef init = {0};
    uint16_t prescaler;
    uint16_t period;

    if (((uint32_t)timer >= LZPORT_TIMER_COUNT) || (cfg == 0) ||
        (cfg->interval_us == 0U) ||
        ((uint32_t)cfg->mode >= LZPORT_TIMER_MODE_COUNT) ||
        (cfg->callback == 0) ||
        !timebase_calculate(cfg->interval_us, &prescaler, &period)) {
        return LZPORT_EINVAL;
    }

    g_state[timer].active = 0U;
    RCC_HB1PeriphClockCmd(g_hw[timer].clock, ENABLE);
    NVIC_DisableIRQ(g_hw[timer].irq);
    TIM_Cmd(g_hw[timer].reg, DISABLE);
    TIM_ITConfig(g_hw[timer].reg, TIM_IT_Update, DISABLE);
    TIM_DeInit(g_hw[timer].reg);

    init.TIM_Prescaler = prescaler;
    init.TIM_CounterMode = TIM_CounterMode_Up;
    init.TIM_Period = period;
    init.TIM_ClockDivision = TIM_CKD_DIV1;
    init.TIM_RepetitionCounter = 0U;
    TIM_TimeBaseInit(g_hw[timer].reg, &init);
    TIM_SelectOnePulseMode(g_hw[timer].reg,
                           (cfg->mode == LZPORT_TIMER_SINGLE_SHOT)
                               ? TIM_OPMode_Single
                               : TIM_OPMode_Repetitive);
    TIM_ClearITPendingBit(g_hw[timer].reg, TIM_IT_Update);
    TIM_ITConfig(g_hw[timer].reg, TIM_IT_Update, ENABLE);

    g_state[timer].callback = cfg->callback;
    g_state[timer].user = cfg->user;
    g_state[timer].repeating = (cfg->mode == LZPORT_TIMER_REPEATING) ? 1U : 0U;
    g_state[timer].active = 0U;
    g_state[timer].configured = 1U;
    NVIC_ClearPendingIRQ(g_hw[timer].irq);
    if ((int)g_hw[timer].irq > 31) {
        NVIC_SetAllocateIRQ(g_hw[timer].irq, Core_ID_V5F);
    }
    NVIC_SetPriority(g_hw[timer].irq, TIMER_IRQ_PRIORITY);
    NVIC_EnableIRQ(g_hw[timer].irq);
    return LZPORT_OK;
}

lzport_status lzport_timer_deinit(lzport_timer timer)
{
    if (((uint32_t)timer >= LZPORT_TIMER_COUNT) ||
        (g_state[timer].configured == 0U)) {
        return LZPORT_EINVAL;
    }
    g_state[timer].active = 0U;
    g_state[timer].configured = 0U;
    TIM_Cmd(g_hw[timer].reg, DISABLE);
    TIM_ITConfig(g_hw[timer].reg, TIM_IT_Update, DISABLE);
    NVIC_DisableIRQ(g_hw[timer].irq);
    TIM_ClearITPendingBit(g_hw[timer].reg, TIM_IT_Update);
    NVIC_ClearPendingIRQ(g_hw[timer].irq);
    TIM_DeInit(g_hw[timer].reg);
    RCC_HB1PeriphClockCmd(g_hw[timer].clock, DISABLE);
    g_state[timer].callback = 0;
    g_state[timer].user = 0;
    return LZPORT_OK;
}

lzport_status lzport_timer_start(lzport_timer timer)
{
    if (((uint32_t)timer >= LZPORT_TIMER_COUNT) ||
        (g_state[timer].configured == 0U)) {
        return LZPORT_EINVAL;
    }
    TIM_Cmd(g_hw[timer].reg, DISABLE);
    TIM_SetCounter(g_hw[timer].reg, 0U);
    TIM_ClearITPendingBit(g_hw[timer].reg, TIM_IT_Update);
    NVIC_ClearPendingIRQ(g_hw[timer].irq);
    g_state[timer].active = 1U;
    TIM_Cmd(g_hw[timer].reg, ENABLE);
    return LZPORT_OK;
}

lzport_status lzport_timer_stop(lzport_timer timer)
{
    if (((uint32_t)timer >= LZPORT_TIMER_COUNT) ||
        (g_state[timer].configured == 0U)) {
        return LZPORT_EINVAL;
    }
    g_state[timer].active = 0U;
    TIM_Cmd(g_hw[timer].reg, DISABLE);
    TIM_ClearITPendingBit(g_hw[timer].reg, TIM_IT_Update);
    NVIC_ClearPendingIRQ(g_hw[timer].irq);
    return LZPORT_OK;
}

bool lzport_timer_is_active(lzport_timer timer)
{
    return ((uint32_t)timer < LZPORT_TIMER_COUNT) &&
           (g_state[timer].active != 0U);
}

static void timer_irq(lzport_timer timer)
{
    lzport_timer_cb callback;
    void *user;

    if (((uint32_t)timer >= LZPORT_TIMER_COUNT) ||
        (TIM_GetITStatus(g_hw[timer].reg, TIM_IT_Update) == RESET)) {
        return;
    }
    TIM_ClearITPendingBit(g_hw[timer].reg, TIM_IT_Update);
    if ((g_state[timer].configured == 0U) ||
        (g_state[timer].active == 0U)) {
        return;
    }
    if (g_state[timer].repeating == 0U) {
        g_state[timer].active = 0U;
        TIM_Cmd(g_hw[timer].reg, DISABLE);
    }
    callback = g_state[timer].callback;
    user = g_state[timer].user;
    callback(timer, user);
}

void TIM6_IRQHandler(void) __attribute__((interrupt));
void TIM6_IRQHandler(void)
{
    timer_irq(LZPORT_TIMER_0);
}

void TIM7_IRQHandler(void) __attribute__((interrupt));
void TIM7_IRQHandler(void)
{
    timer_irq(LZPORT_TIMER_1);
}
