/**************************************************************************/
/*  SysTick0_Handler — ThreadX tick wrapper (no WCH-Interrupt-fast/HPE)   */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global SysTick0_Handler
    .type   SysTick0_Handler, @function
SysTick0_Handler:
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, -65*4
#else
    addi    sp, sp, -32*4
#endif
    sw      ra, 28*4(sp)
    call    _tx_thread_context_save

    li      t0, SYSTICK0_BASE
    sw      zero, 4(t0)

    call    _tx_timer_interrupt
    j       _tx_thread_context_restore
