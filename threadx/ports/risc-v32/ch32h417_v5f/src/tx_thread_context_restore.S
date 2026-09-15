/**************************************************************************/
/*  _tx_thread_context_restore — always leaves via mret                   */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global _tx_thread_context_restore
    .type   _tx_thread_context_restore, @function
_tx_thread_context_restore:

    li      t0, GINTENR_IE
    csrc    CSR_GINTENR, t0
    fence.i

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    call    _tx_execution_isr_exit
#endif

    la      t0, _tx_thread_system_state
    lw      t1, 0(t0)
    addi    t1, t1, -1
    sw      t1, 0(t0)
    beqz    t1, _tx_thread_not_nested_restore

    j       _tx_thread_interrupt_return

_tx_thread_not_nested_restore:
    la      t0, _tx_thread_current_ptr
    lw      t1, 0(t0)
    beqz    t1, _tx_thread_idle_system_restore

    la      t0, _tx_thread_preempt_disable
    lw      t2, 0(t0)
    bgtz    t2, _tx_thread_no_preempt_restore

    la      t0, _tx_thread_execute_ptr
    lw      t2, 0(t0)
    bne     t1, t2, _tx_thread_preempt_restore

_tx_thread_no_preempt_restore:
    lw      sp, 8(t1)
    j       _tx_thread_interrupt_return

_tx_thread_preempt_restore:
    lw      t0, 8(t1)
    ori     t3, zero, 1
    sw      t3, 0(t0)

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    lw      t3, 29*4(t0)
    srli    t3, t3, 13
    andi    t3, t3, 0x3
    beqz    t3, _tx_thread_preempt_skip_fp
#endif
#if defined(__riscv_float_abi_single)
    fsw     f8, 39*4(t0)
    fsw     f9, 40*4(t0)
    fsw     f18, 49*4(t0)
    fsw     f19, 50*4(t0)
    fsw     f20, 51*4(t0)
    fsw     f21, 52*4(t0)
    fsw     f22, 53*4(t0)
    fsw     f23, 54*4(t0)
    fsw     f24, 55*4(t0)
    fsw     f25, 56*4(t0)
    fsw     f26, 57*4(t0)
    fsw     f27, 58*4(t0)
#elif defined(__riscv_float_abi_double)
    fsd     f8, 39*4(t0)
    fsd     f9, 40*4(t0)
    fsd     f18, 49*4(t0)
    fsd     f19, 50*4(t0)
    fsd     f20, 51*4(t0)
    fsd     f21, 52*4(t0)
    fsd     f22, 53*4(t0)
    fsd     f23, 54*4(t0)
    fsd     f24, 55*4(t0)
    fsd     f25, 56*4(t0)
    fsd     f26, 57*4(t0)
    fsd     f27, 58*4(t0)
#endif
_tx_thread_preempt_skip_fp:

    sw      x9, 11*4(t0)
    sw      x18, 10*4(t0)
    sw      x19, 9*4(t0)
    sw      x20, 8*4(t0)
    sw      x21, 7*4(t0)
    sw      x22, 6*4(t0)
    sw      x23, 5*4(t0)
    sw      x24, 4*4(t0)
    sw      x25, 3*4(t0)
    sw      x26, 2*4(t0)
    sw      x27, 1*4(t0)

    la      t0, _tx_timer_time_slice
    lw      t2, 0(t0)
    beqz    t2, _tx_thread_dont_save_ts
    sw      t2, 24(t1)
    sw      x0, 0(t0)

_tx_thread_dont_save_ts:
    /* Resume the next thread with mret while PFIC is still in the ISR.
       mret-to-schedule then jr t1 leaves t1 = resume PC; at 10 kHz that
       store-through-t1 corrupts ITCM and looks like a reset loop. */
    la      t0, _tx_thread_current_ptr
    sw      x0, 0(t0)

    la      t3, _tx_thread_execute_ptr
    lw      t3, 0(t3)
    beqz    t3, _tx_thread_preempt_to_schedule

    sw      t3, 0(t0)
    lw      t2, 4(t3)
    addi    t2, t2, 1
    sw      t2, 4(t3)
    lw      t2, 24(t3)
    la      t0, _tx_timer_time_slice
    sw      t2, 0(t0)

    lw      sp, 8(t3)
    lw      t0, 0(sp)
    beqz    t0, _tx_thread_preempt_solicited_resume
    j       _tx_thread_irq_mret_resume

_tx_thread_preempt_to_schedule:
    la      t0, _tx_v5f_in_isr
    sw      zero, 0(t0)
    la      t0, _tx_thread_schedule
    csrw    mepc, t0
    mret

_tx_thread_idle_system_restore:
    j       _tx_thread_interrupt_return

_tx_thread_preempt_solicited_resume:
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    lw      t1, 14*4(sp)
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_preempt_solicited_skip_fp
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
_tx_thread_preempt_solicited_skip_fp:
#endif
    la      t0, _tx_v5f_in_isr
    sw      zero, 0(t0)
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
    csrw    mepc, ra
    mret

_tx_thread_irq_mret_resume:
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    lw      t1, 29*4(sp)
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_irq_mret_skip_fp
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
_tx_thread_irq_mret_skip_fp:
#endif
    la      t0, _tx_v5f_in_isr
    sw      zero, 0(t0)
    lw      t1, 30*4(sp)
    csrw    mepc, t1
    li      t0, GINTENR_IE
    csrs    CSR_GINTENR, t0
    lw      ra, 28*4(sp)
    lw      t0, 19*4(sp)
    lw      t1, 18*4(sp)
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
    addi    sp, sp, 64*4
#else
    addi    sp, sp, 32*4
#endif
    mret

_tx_thread_interrupt_return:
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    lw      t1, 29*4(sp)
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_irq_skip_fp
#endif
#if defined(__riscv_float_abi_single)
    flw     f0, 31*4(sp)
    flw     f1, 32*4(sp)
    flw     f2, 33*4(sp)
    flw     f3, 34*4(sp)
    flw     f4, 35*4(sp)
    flw     f5, 36*4(sp)
    flw     f6, 37*4(sp)
    flw     f7, 38*4(sp)
    flw     f10, 41*4(sp)
    flw     f11, 42*4(sp)
    flw     f12, 43*4(sp)
    flw     f13, 44*4(sp)
    flw     f14, 45*4(sp)
    flw     f15, 46*4(sp)
    flw     f16, 47*4(sp)
    flw     f17, 48*4(sp)
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
    fld     f10, 41*4(sp)
    fld     f11, 42*4(sp)
    fld     f12, 43*4(sp)
    fld     f13, 44*4(sp)
    fld     f14, 45*4(sp)
    fld     f15, 46*4(sp)
    fld     f16, 47*4(sp)
    fld     f17, 48*4(sp)
    fld     f28, 59*4(sp)
    fld     f29, 60*4(sp)
    fld     f30, 61*4(sp)
    fld     f31, 62*4(sp)
    lw      t0, 63*4(sp)
    csrw    fcsr, t0
#endif
_tx_thread_irq_skip_fp:

    la      t0, _tx_v5f_in_isr
    sw      zero, 0(t0)

    lw      t6, 30*4(sp)
    csrw    mepc, t6

    li      t0, GINTENR_IE
    csrs    CSR_GINTENR, t0

    lw      ra, 28*4(sp)
    lw      t0, 19*4(sp)
    lw      t1, 18*4(sp)
    lw      t2, 17*4(sp)
    lw      s0, 12*4(sp)
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

#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    addi    sp, sp, 64*4
#else
    addi    sp, sp, 32*4
#endif
    mret
