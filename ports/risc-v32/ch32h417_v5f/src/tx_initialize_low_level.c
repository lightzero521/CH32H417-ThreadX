/**************************************************************************/
/*                                                                        */
/*    _tx_initialize_low_level             CH32H417 QingKe V5F            */
/*                                                                        */
/*  Disable HPE (software stacking, Zephyr-style), save the system stack, */
/*  and start SysTick0 as the ThreadX tick.                               */
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

VOID _tx_initialize_low_level(VOID)
{
    ULONG ticks;

    /* INTSYSCR: turn off HPE and hardware nesting. ThreadX does its own
       software stacking. HPE + mret-from-switched-stack wedges PFIC. */
    __asm volatile ("csrw %0, zero" :: "i" (CSR_INTSYSCR));

    _tx_thread_system_stack_ptr = (VOID *)(ULONG)__get_SP();
    _tx_initialize_unused_memory = _end;

    ticks = SystemCoreClock / TX_TIMER_TICKS_PER_SECOND;
    if (ticks == 0U)
    {
        ticks = 1U;
    }

    SysTick0->CTLR = 0;
    SysTick0->ISR  = 0;
    SysTick0->CNT  = 0;
    SysTick0->CMP  = ticks - 1U;
    SysTick0->CTLR = SYSTICK_CTLR_STRE | SYSTICK_CTLR_STCLK |
                     SYSTICK_CTLR_STIE | SYSTICK_CTLR_STE;

    NVIC_SetPriority(SysTick0_IRQn, TX_SYSTICK_IRQ_PRIORITY);
    NVIC_EnableIRQ(SysTick0_IRQn);

    (VOID)_heap_end;
}
