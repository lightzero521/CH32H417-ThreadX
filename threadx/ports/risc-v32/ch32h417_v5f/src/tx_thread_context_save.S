/**************************************************************************/
/*  _tx_thread_context_save — software stacking (HPE is off)              */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global _tx_thread_context_save
    .type   _tx_thread_context_save, @function
_tx_thread_context_save:

    sw      t0, 19*4(sp)
    sw      t1, 18*4(sp)

    la      t0, _tx_thread_system_state
    lw      t1, 0(t0)
    addi    t1, t1, 1
    sw      t1, 0(t0)
    li      t0, 1
    bgt     t1, t0, _tx_thread_nested_save

    la      t0, _tx_v5f_in_isr
    li      t1, 1
    sw      t1, 0(t0)

    sw      t2, 17*4(sp)
    sw      s0, 12*4(sp)
    sw      a0, 27*4(sp)
    sw      a1, 26*4(sp)
    sw      a2, 25*4(sp)
    sw      a3, 24*4(sp)
    sw      a4, 23*4(sp)
    sw      a5, 22*4(sp)
    sw      a6, 21*4(sp)
    sw      a7, 20*4(sp)
    sw      t3, 16*4(sp)
    sw      t4, 15*4(sp)
    sw      t5, 14*4(sp)
    sw      t6, 13*4(sp)

    csrr    t0, mstatus
    sw      t0, 29*4(sp)
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    srli    t1, t0, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_skip_fpu_save
#endif

#if defined(__riscv_float_abi_single)
    fsw     f0, 31*4(sp)
    fsw     f1, 32*4(sp)
    fsw     f2, 33*4(sp)
    fsw     f3, 34*4(sp)
    fsw     f4, 35*4(sp)
    fsw     f5, 36*4(sp)
    fsw     f6, 37*4(sp)
    fsw     f7, 38*4(sp)
    fsw     f8, 39*4(sp)
    fsw     f9, 40*4(sp)
    fsw     f10, 41*4(sp)
    fsw     f11, 42*4(sp)
    fsw     f12, 43*4(sp)
    fsw     f13, 44*4(sp)
    fsw     f14, 45*4(sp)
    fsw     f15, 46*4(sp)
    fsw     f16, 47*4(sp)
    fsw     f17, 48*4(sp)
    fsw     f18, 49*4(sp)
    fsw     f19, 50*4(sp)
    fsw     f20, 51*4(sp)
    fsw     f21, 52*4(sp)
    fsw     f22, 53*4(sp)
    fsw     f23, 54*4(sp)
    fsw     f24, 55*4(sp)
    fsw     f25, 56*4(sp)
    fsw     f26, 57*4(sp)
    fsw     f27, 58*4(sp)
    fsw     f28, 59*4(sp)
    fsw     f29, 60*4(sp)
    fsw     f30, 61*4(sp)
    fsw     f31, 62*4(sp)
    csrr    t0, fcsr
    sw      t0, 63*4(sp)
#elif defined(__riscv_float_abi_double)
    fsd     f0, 31*4(sp)
    fsd     f1, 32*4(sp)
    fsd     f2, 33*4(sp)
    fsd     f3, 34*4(sp)
    fsd     f4, 35*4(sp)
    fsd     f5, 36*4(sp)
    fsd     f6, 37*4(sp)
    fsd     f7, 38*4(sp)
    fsd     f8, 39*4(sp)
    fsd     f9, 40*4(sp)
    fsd     f10, 41*4(sp)
    fsd     f11, 42*4(sp)
    fsd     f12, 43*4(sp)
    fsd     f13, 44*4(sp)
    fsd     f14, 45*4(sp)
    fsd     f15, 46*4(sp)
    fsd     f16, 47*4(sp)
    fsd     f17, 48*4(sp)
    fsd     f18, 49*4(sp)
    fsd     f19, 50*4(sp)
    fsd     f20, 51*4(sp)
    fsd     f21, 52*4(sp)
    fsd     f22, 53*4(sp)
    fsd     f23, 54*4(sp)
    fsd     f24, 55*4(sp)
    fsd     f25, 56*4(sp)
    fsd     f26, 57*4(sp)
    fsd     f27, 58*4(sp)
    fsd     f28, 59*4(sp)
    fsd     f29, 60*4(sp)
    fsd     f30, 61*4(sp)
    fsd     f31, 62*4(sp)
    csrr    t0, fcsr
    sw      t0, 63*4(sp)
#endif
_tx_thread_skip_fpu_save:

    csrr    t0, mepc
    sw      t0, 30*4(sp)

    la      t1, _tx_thread_current_ptr
    lw      t2, 0(t1)
    beqz    t2, _tx_thread_idle_system_save

    sw      sp, 8(t2)
    la      t0, _tx_thread_system_stack_ptr
    lw      sp, 0(t0)

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    call    _tx_execution_isr_enter
#endif
    ret

_tx_thread_nested_save:
    sw      t2, 17*4(sp)
    sw      s0, 12*4(sp)
    sw      a0, 27*4(sp)
    sw      a1, 26*4(sp)
    sw      a2, 25*4(sp)
    sw      a3, 24*4(sp)
    sw      a4, 23*4(sp)
    sw      a5, 22*4(sp)
    sw      a6, 21*4(sp)
    sw      a7, 20*4(sp)
    sw      t3, 16*4(sp)
    sw      t4, 15*4(sp)
    sw      t5, 14*4(sp)
    sw      t6, 13*4(sp)

    csrr    t0, mstatus
    sw      t0, 29*4(sp)
#if defined(__riscv_float_abi_single) || defined(__riscv_float_abi_double)
    srli    t1, t0, 13
    andi    t1, t1, 0x3
    beqz    t1, _tx_thread_skip_nested_fpu_save
#endif

#if defined(__riscv_float_abi_single)
    fsw     f0, 31*4(sp)
    fsw     f1, 32*4(sp)
    fsw     f2, 33*4(sp)
    fsw     f3, 34*4(sp)
    fsw     f4, 35*4(sp)
    fsw     f5, 36*4(sp)
    fsw     f6, 37*4(sp)
    fsw     f7, 38*4(sp)
    fsw     f8, 39*4(sp)
    fsw     f9, 40*4(sp)
    fsw     f10, 41*4(sp)
    fsw     f11, 42*4(sp)
    fsw     f12, 43*4(sp)
    fsw     f13, 44*4(sp)
    fsw     f14, 45*4(sp)
    fsw     f15, 46*4(sp)
    fsw     f16, 47*4(sp)
    fsw     f17, 48*4(sp)
    fsw     f18, 49*4(sp)
    fsw     f19, 50*4(sp)
    fsw     f20, 51*4(sp)
    fsw     f21, 52*4(sp)
    fsw     f22, 53*4(sp)
    fsw     f23, 54*4(sp)
    fsw     f24, 55*4(sp)
    fsw     f25, 56*4(sp)
    fsw     f26, 57*4(sp)
    fsw     f27, 58*4(sp)
    fsw     f28, 59*4(sp)
    fsw     f29, 60*4(sp)
    fsw     f30, 61*4(sp)
    fsw     f31, 62*4(sp)
    csrr    t0, fcsr
    sw      t0, 63*4(sp)
#elif defined(__riscv_float_abi_double)
    fsd     f0, 31*4(sp)
    fsd     f1, 32*4(sp)
    fsd     f2, 33*4(sp)
    fsd     f3, 34*4(sp)
    fsd     f4, 35*4(sp)
    fsd     f5, 36*4(sp)
    fsd     f6, 37*4(sp)
    fsd     f7, 38*4(sp)
    fsd     f8, 39*4(sp)
    fsd     f9, 40*4(sp)
    fsd     f10, 41*4(sp)
    fsd     f11, 42*4(sp)
    fsd     f12, 43*4(sp)
    fsd     f13, 44*4(sp)
    fsd     f14, 45*4(sp)
    fsd     f15, 46*4(sp)
    fsd     f16, 47*4(sp)
    fsd     f17, 48*4(sp)
    fsd     f18, 49*4(sp)
    fsd     f19, 50*4(sp)
    fsd     f20, 51*4(sp)
    fsd     f21, 52*4(sp)
    fsd     f22, 53*4(sp)
    fsd     f23, 54*4(sp)
    fsd     f24, 55*4(sp)
    fsd     f25, 56*4(sp)
    fsd     f26, 57*4(sp)
    fsd     f27, 58*4(sp)
    fsd     f28, 59*4(sp)
    fsd     f29, 60*4(sp)
    fsd     f30, 61*4(sp)
    fsd     f31, 62*4(sp)
    csrr    t0, fcsr
    sw      t0, 63*4(sp)
#endif
_tx_thread_skip_nested_fpu_save:

    csrr    t0, mepc
    sw      t0, 30*4(sp)

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    call    _tx_execution_isr_enter
#endif
    ret

_tx_thread_idle_system_save:
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    call    _tx_execution_isr_enter
#endif
    /* Keep the ISR frame on the system stack and mret back to the
       scheduler idle loop. Jumping to schedule without mret wedges PFIC. */
    ret
