/**************************************************************************/
/*  _tx_thread_stack_build — solicited frame, first dispatch uses ret     */
/*  QingKe PFIC HardFaults if we mret from thread mode.                   */
/**************************************************************************/

    .section .text
    .align  2
    .global _tx_thread_stack_build
    .type   _tx_thread_stack_build, @function
_tx_thread_stack_build:

    lw      t0, 16(a0)
    li      t1, ~15
    and     t0, t0, t1

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    t0, t0, -29*4
#else
    addi    t0, t0, -16*4
#endif
    sw      zero, 0*4(t0)
    sw      zero, 1*4(t0)
    sw      zero, 2*4(t0)
    sw      zero, 3*4(t0)
    sw      zero, 4*4(t0)
    sw      zero, 5*4(t0)
    sw      zero, 6*4(t0)
    sw      zero, 7*4(t0)
    sw      zero, 8*4(t0)
    sw      zero, 9*4(t0)
    sw      zero, 10*4(t0)
    sw      zero, 11*4(t0)
    sw      zero, 12*4(t0)
    sw      a1, 13*4(t0)
    /* Slot is only an FPU-present hint. Never restored to mstatus. */
    li      t1, 0x6000
    sw      t1, 14*4(t0)
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    sw      zero, 15*4(t0)
    sw      zero, 16*4(t0)
    sw      zero, 17*4(t0)
    sw      zero, 18*4(t0)
    sw      zero, 19*4(t0)
    sw      zero, 20*4(t0)
    sw      zero, 21*4(t0)
    sw      zero, 22*4(t0)
    sw      zero, 23*4(t0)
    sw      zero, 24*4(t0)
    sw      zero, 25*4(t0)
    sw      zero, 26*4(t0)
    sw      zero, 27*4(t0)
    sw      zero, 28*4(t0)
#endif

    sw      t0, 8(a0)
    ret
