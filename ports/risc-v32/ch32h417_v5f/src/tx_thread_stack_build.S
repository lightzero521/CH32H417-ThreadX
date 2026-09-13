/**************************************************************************/
/*  _tx_thread_stack_build — interrupt frame (always mret)                */
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
    addi    t0, t0, -65*4
#else
    addi    t0, t0, -32*4
#endif
    li      t1, 1
    sw      t1, 0*4(t0)
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
    sw      zero, 13*4(t0)
    sw      zero, 14*4(t0)
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
    sw      a1, 30*4(t0)
    li      t1, 0x1880
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    li      t2, 0x6000
    or      t1, t1, t2
#endif
    sw      t1, 29*4(t0)
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    sw      zero, 31*4(t0)
    sw      zero, 32*4(t0)
    sw      zero, 33*4(t0)
    sw      zero, 34*4(t0)
    sw      zero, 35*4(t0)
    sw      zero, 36*4(t0)
    sw      zero, 37*4(t0)
    sw      zero, 38*4(t0)
    sw      zero, 39*4(t0)
    sw      zero, 40*4(t0)
    sw      zero, 41*4(t0)
    sw      zero, 42*4(t0)
    sw      zero, 43*4(t0)
    sw      zero, 44*4(t0)
    sw      zero, 45*4(t0)
    sw      zero, 46*4(t0)
    sw      zero, 47*4(t0)
    sw      zero, 48*4(t0)
    sw      zero, 49*4(t0)
    sw      zero, 50*4(t0)
    sw      zero, 51*4(t0)
    sw      zero, 52*4(t0)
    sw      zero, 53*4(t0)
    sw      zero, 54*4(t0)
    sw      zero, 55*4(t0)
    sw      zero, 56*4(t0)
    sw      zero, 57*4(t0)
    sw      zero, 58*4(t0)
    sw      zero, 59*4(t0)
    sw      zero, 60*4(t0)
    sw      zero, 61*4(t0)
    sw      zero, 62*4(t0)
    sw      zero, 63*4(t0)
    sw      zero, 64*4(t0)
#else
    sw      zero, 31*4(t0)
#endif

    sw      t0, 8(a0)
    ret
