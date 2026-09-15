/**************************************************************************/
/*  SW_Handler — QingKe PendSV                                            */
/*  Lowest-priority software IRQ. ThreadX APIs from a C ISR pend this;    */
/*  after the peripheral ISR mrets, we save/restore and switch if needed. */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global SW_Handler
    .type   SW_Handler, @function
SW_Handler:
    /* 64 words = 256 bytes, 16-byte SP for the following C/ABI calls.
       Slot 63 holds fcsr; single-precision fsw does not need more. */
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, -64*4
#else
    addi    sp, sp, -32*4
#endif
    sw      ra, 28*4(sp)
    call    _tx_thread_context_save
    j       _tx_thread_context_restore
