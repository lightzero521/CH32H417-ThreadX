/**************************************************************************/
/*  SysTick1_Handler — ThreadX tick wrapper (software stack, then mret)   */
/*  V5F tick is SysTick1; compare flag is SysTick0->ISR bit1.             */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global SysTick1_Handler
    .type   SysTick1_Handler, @function
SysTick1_Handler:
    /* 64 words = 256 bytes so SP stays 16-byte aligned for C calls. */
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, -64*4
#else
    addi    sp, sp, -32*4
#endif
    sw      ra, 28*4(sp)
    call    _tx_thread_context_save

    li      t0, SYSTICK0_BASE
    lw      t1, 4(t0)
    li      t2, 2
    not     t2, t2
    and     t1, t1, t2
    sw      t1, 4(t0)

    call    _tx_timer_interrupt
    j       _tx_thread_context_restore
