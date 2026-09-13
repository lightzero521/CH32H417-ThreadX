/**************************************************************************/
/*  _tx_thread_system_return — solicited yield                            */
/*  Frame is still "solicited", but schedule returns via mret so PFIC     */
/*  interrupt level is always retired (Zephyr ALWAYS_SWITCH_THROUGH_ECALL */
/*  equivalent).                                                          */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global _tx_thread_system_return
    .type   _tx_thread_system_return, @function
_tx_thread_system_return:

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, -29*4
#else
    addi    sp, sp, -16*4
#endif

    sw      zero, 0(sp)

    csrr    t0, mstatus
    sw      t0, 14*4(sp)

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    srli    t1, t0, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_system_return_skip_fp
#if defined(__riscv_float_abi_single)
    fsw     f8, 15*4(sp)
    fsw     f9, 16*4(sp)
    fsw     f18, 17*4(sp)
    fsw     f19, 18*4(sp)
    fsw     f20, 19*4(sp)
    fsw     f21, 20*4(sp)
    fsw     f22, 21*4(sp)
    fsw     f23, 22*4(sp)
    fsw     f24, 23*4(sp)
    fsw     f25, 24*4(sp)
    fsw     f26, 25*4(sp)
    fsw     f27, 26*4(sp)
    csrr    t0, fcsr
    sw      t0, 27*4(sp)
#elif defined(__riscv_float_abi_double)
    fsd     f8, 15*4(sp)
    fsd     f9, 16*4(sp)
    fsd     f18, 17*4(sp)
    fsd     f19, 18*4(sp)
    fsd     f20, 19*4(sp)
    fsd     f21, 20*4(sp)
    fsd     f22, 21*4(sp)
    fsd     f23, 22*4(sp)
    fsd     f24, 23*4(sp)
    fsd     f25, 24*4(sp)
    fsd     f26, 25*4(sp)
    fsd     f27, 26*4(sp)
    csrr    t0, fcsr
    sw      t0, 27*4(sp)
#endif
_tx_thread_system_return_skip_fp:
#endif

    sw      ra, 13*4(sp)
    sw      s0, 12*4(sp)
    sw      s1, 11*4(sp)
    sw      s2, 10*4(sp)
    sw      s3, 9*4(sp)
    sw      s4, 8*4(sp)
    sw      s5, 7*4(sp)
    sw      s6, 6*4(sp)
    sw      s7, 5*4(sp)
    sw      s8, 4*4(sp)
    sw      s9, 3*4(sp)
    sw      s10, 2*4(sp)
    sw      s11, 1*4(sp)

    li      t0, GINTENR_IE
    csrc    CSR_GINTENR, t0
    csrci   mstatus, 0x08
    fence.i

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    call    _tx_execution_thread_exit
#endif

    la      t0, _tx_thread_current_ptr
    lw      t1, 0(t0)
    la      t2, _tx_thread_system_stack_ptr

    sw      sp, 8(t1)
    lw      sp, 0(t2)

    la      t4, _tx_timer_time_slice
    lw      t3, 0(t4)
    la      t2, _tx_thread_schedule
    beqz    t3, _tx_thread_dont_save_ts

    sw      t3, 24(t1)
    sw      zero, 0(t4)

_tx_thread_dont_save_ts:
    sw      x0, 0(t0)
    jr      t2
