/**************************************************************************/
/*  _tx_thread_schedule                                                   */
/*  Always entered in thread mode. Solicited: ret. IRQ frame: jr mepc.    */
/*  ISR paths must mret here; never mret from this function.              */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global _tx_thread_schedule
    .type   _tx_thread_schedule, @function
_tx_thread_schedule:

    /* Match Cortex-M: POST_INITIALIZATION held this at 1. */
    la      t0, _tx_thread_preempt_disable
    sw      zero, 0(t0)

    li      t0, GINTENR_IE
    csrs    CSR_GINTENR, t0

_tx_thread_schedule_loop:
    la      t0, _tx_thread_execute_ptr
    lw      t1, 0(t0)
    bnez    t1, _tx_thread_ready_to_run
#ifndef TX_NO_WFI
    call    _tx_v5f_wfi
#endif
    j       _tx_thread_schedule_loop

_tx_thread_ready_to_run:
    li      t2, GINTENR_IE
    csrc    CSR_GINTENR, t2
    fence.i

    lw      t1, 0(t0)
    beqz    t1, _tx_thread_schedule_loop

    la      t0, _tx_thread_current_ptr
    sw      t1, 0(t0)

    lw      t2, 4(t1)
    addi    t2, t2, 1
    sw      t2, 4(t1)

    lw      t2, 24(t1)
    la      t3, _tx_timer_time_slice
    sw      t2, 0(t3)

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    call    _tx_execution_thread_enter
#endif

    lw      sp, 8(t1)

    lw      t0, 0(sp)
    beqz    t0, _tx_thread_solicited_return

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    lw      t1, 29*4(sp)
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_schedule_skip_fp
#if defined(__riscv_float_abi_single)
    flw     f0, 31*4(sp)
    flw     f1, 32*4(sp)
    flw     f2, 33*4(sp)
    flw     f3, 34*4(sp)
    flw     f4, 35*4(sp)
    flw     f5, 36*4(sp)
    flw     f6, 37*4(sp)
    flw     f7, 38*4(sp)
    flw     f8, 39*4(sp)
    flw     f9, 40*4(sp)
    flw     f10, 41*4(sp)
    flw     f11, 42*4(sp)
    flw     f12, 43*4(sp)
    flw     f13, 44*4(sp)
    flw     f14, 45*4(sp)
    flw     f15, 46*4(sp)
    flw     f16, 47*4(sp)
    flw     f17, 48*4(sp)
    flw     f18, 49*4(sp)
    flw     f19, 50*4(sp)
    flw     f20, 51*4(sp)
    flw     f21, 52*4(sp)
    flw     f22, 53*4(sp)
    flw     f23, 54*4(sp)
    flw     f24, 55*4(sp)
    flw     f25, 56*4(sp)
    flw     f26, 57*4(sp)
    flw     f27, 58*4(sp)
    flw     f28, 59*4(sp)
    flw     f29, 60*4(sp)
    flw     f30, 61*4(sp)
    flw     f31, 62*4(sp)
    lw      t0, 63*4(sp)
    csrw    fcsr, t0
#elif defined(__riscv_float_abi_double)
    fld     f0, 31*4(sp)
    fld     f1, 32*4(sp)
    fld     f2, 33*4(sp)
    fld     f3, 34*4(sp)
    fld     f4, 35*4(sp)
    fld     f5, 36*4(sp)
    fld     f6, 37*4(sp)
    fld     f7, 38*4(sp)
    fld     f8, 39*4(sp)
    fld     f9, 40*4(sp)
    fld     f10, 41*4(sp)
    fld     f11, 42*4(sp)
    fld     f12, 43*4(sp)
    fld     f13, 44*4(sp)
    fld     f14, 45*4(sp)
    fld     f15, 46*4(sp)
    fld     f16, 47*4(sp)
    fld     f17, 48*4(sp)
    fld     f18, 49*4(sp)
    fld     f19, 50*4(sp)
    fld     f20, 51*4(sp)
    fld     f21, 52*4(sp)
    fld     f22, 53*4(sp)
    fld     f23, 54*4(sp)
    fld     f24, 55*4(sp)
    fld     f25, 56*4(sp)
    fld     f26, 57*4(sp)
    fld     f27, 58*4(sp)
    fld     f28, 59*4(sp)
    fld     f29, 60*4(sp)
    fld     f30, 61*4(sp)
    fld     f31, 62*4(sp)
    lw      t0, 63*4(sp)
    csrw    fcsr, t0
#endif
_tx_thread_schedule_skip_fp:
#endif

    /* Thread-mode resume of an interrupt frame: jr, never mret/csrw mepc.
       t1 is the jump register (QingKe cannot csrw mepc here). */
    li      t2, GINTENR_IE
    csrs    CSR_GINTENR, t2
    lw      t1, 30*4(sp)
    lw      ra, 28*4(sp)
    lw      t0, 19*4(sp)
    lw      t2, 17*4(sp)
    lw      s0, 12*4(sp)
    lw      x9, 11*4(sp)
    lw      a0, 27*4(sp)
    lw      a1, 26*4(sp)
    lw      a2, 25*4(sp)
    lw      a3, 24*4(sp)
    lw      a4, 23*4(sp)
    lw      a5, 22*4(sp)
    lw      a6, 21*4(sp)
    lw      a7, 20*4(sp)
    lw      t3, 16*4(sp)
    lw      t4, 15*4(sp)
    lw      t5, 14*4(sp)
    lw      t6, 13*4(sp)
    lw      x18, 10*4(sp)
    lw      x19, 9*4(sp)
    lw      x20, 8*4(sp)
    lw      x21, 7*4(sp)
    lw      x22, 6*4(sp)
    lw      x23, 5*4(sp)
    lw      x24, 4*4(sp)
    lw      x25, 3*4(sp)
    lw      x26, 2*4(sp)
    lw      x27, 1*4(sp)
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, 65*4
#else
    addi    sp, sp, 32*4
#endif
    jr      t1

_tx_thread_solicited_return:
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    lw      t1, 14*4(sp)
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_schedule_solicited_skip_fp
#if defined(__riscv_float_abi_single)
    flw     f8, 15*4(sp)
    flw     f9, 16*4(sp)
    flw     f18, 17*4(sp)
    flw     f19, 18*4(sp)
    flw     f20, 19*4(sp)
    flw     f21, 20*4(sp)
    flw     f22, 21*4(sp)
    flw     f23, 22*4(sp)
    flw     f24, 23*4(sp)
    flw     f25, 24*4(sp)
    flw     f26, 25*4(sp)
    flw     f27, 26*4(sp)
    lw      t0, 27*4(sp)
    csrw    fcsr, t0
#elif defined(__riscv_float_abi_double)
    fld     f8, 15*4(sp)
    fld     f9, 16*4(sp)
    fld     f18, 17*4(sp)
    fld     f19, 18*4(sp)
    fld     f20, 19*4(sp)
    fld     f21, 20*4(sp)
    fld     f22, 21*4(sp)
    fld     f23, 22*4(sp)
    fld     f24, 23*4(sp)
    fld     f25, 24*4(sp)
    fld     f26, 25*4(sp)
    fld     f27, 26*4(sp)
    lw      t0, 27*4(sp)
    csrw    fcsr, t0
#endif
_tx_thread_schedule_solicited_skip_fp:
#endif

    li      t0, GINTENR_IE
    csrs    CSR_GINTENR, t0

    lw      ra, 13*4(sp)
    lw      s0, 12*4(sp)
    lw      s1, 11*4(sp)
    lw      x18, 10*4(sp)
    lw      x19, 9*4(sp)
    lw      x20, 8*4(sp)
    lw      x21, 7*4(sp)
    lw      x22, 6*4(sp)
    lw      x23, 5*4(sp)
    lw      x24, 4*4(sp)
    lw      x25, 3*4(sp)
    lw      x26, 2*4(sp)
    lw      x27, 1*4(sp)

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, 29*4
#else
    addi    sp, sp, 16*4
#endif
    ret
