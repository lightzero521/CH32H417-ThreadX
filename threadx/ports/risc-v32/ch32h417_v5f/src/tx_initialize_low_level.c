/**************************************************************************/
/*                                                                        */
/*    _tx_initialize_low_level             CH32H417 QingKe V5F            */
/*                                                                        */
/**************************************************************************/

#include "tx_api.h"
#include "tx_initialize.h"
#include "tx_thread.h"
#include "tx_qke_v5f.h"
#include "ch32h417.h"

extern UCHAR _end[];
extern UCHAR _heap_end[];

#ifndef TX_SYSTICK_IRQ_PRIORITY
#define TX_SYSTICK_IRQ_PRIORITY     ((uint8_t)0x80)
#endif

#ifndef TX_SOFTWARE_IRQ_PRIORITY
#define TX_SOFTWARE_IRQ_PRIORITY    ((uint8_t)0xF0)
#endif

#define PFIC_GISR_GACTSTA           (1u << 8)

ULONG _tx_v5f_in_isr = 0;

/* PFIC stand-in for Cortex-M IPSR: nonzero while any IRQ is active. */
ULONG _tx_v5f_irq_active(VOID)
{
    UINT i;

    if ((PFIC->GISR & PFIC_GISR_GACTSTA) != 0U)
    {
        return 1U;
    }

    for (i = 0U; i < 8U; i++)
    {
        if (NVIC->IACTR[i] != 0U)
        {
            return 1U;
        }
    }

    return 0U;
}

/* Cortex-M _tx_thread_system_return: pend PendSV. QingKe: Software_IRQn. */
VOID _tx_thread_system_return(VOID)
{
    NVIC_SetPendingIRQ(Software_IRQn);
    if (_tx_v5f_irq_active() == 0U)
    {
        __enable_irq();
    }
}

VOID _tx_v5f_wfi(VOID)
{
    __WFI();
}

VOID _tx_v5f_systick_start(VOID)
{
    ULONG ticks;
    ULONG hz;

    /* SysTick1 is clocked from HCLK, not V5F core clock. */
    hz = HCLKClock;
    if (hz == 0U)
    {
        hz = SystemCoreClock;
    }

    ticks = hz / TX_TIMER_TICKS_PER_SECOND;
    if (ticks == 0U)
    {
        ticks = 1U;
    }

    SysTick1->CTLR = 0;
    SysTick0->ISR &= ~(1u << 1);
    SysTick1->CNT = 0;
    SysTick1->CMP = ticks - 1U;
    SysTick1->CTLR = SYSTICK_CTLR_STRE | SYSTICK_CTLR_STCLK |
                     SYSTICK_CTLR_STIE | SYSTICK_CTLR_STE;

    NVIC_SetPriority(Software_IRQn, TX_SOFTWARE_IRQ_PRIORITY);
    NVIC_EnableIRQ(Software_IRQn);

    NVIC_SetPriority(SysTick1_IRQn, TX_SYSTICK_IRQ_PRIORITY);
    NVIC_EnableIRQ(SysTick1_IRQn);
}

VOID _tx_initialize_low_level(VOID)
{
    /* INTSYSCR: software stacking only. */
    __asm volatile ("csrw %0, zero" :: "i" (CSR_INTSYSCR));

    /* Keep IRQs off until the scheduler is ready. Enabling SysTick here
       races high-level init (system_state is still 0xF0F0F0F0). */
    __disable_irq();

    _tx_v5f_in_isr = 0;
    _tx_thread_system_stack_ptr = (VOID *)(ULONG)__get_SP();
    _tx_initialize_unused_memory = _end;

    (VOID)_heap_end;
}
