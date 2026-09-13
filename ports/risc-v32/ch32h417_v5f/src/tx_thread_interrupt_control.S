/**************************************************************************/
/*  _tx_thread_interrupt_control — QingKe V5F                             */
/*  Global IRQ posture is CSR 0x800 (GINTENR), matching WCH HAL.          */
/**************************************************************************/

#include "tx_qke_v5f.h"

    .section .text
    .align  2
    .global _tx_thread_interrupt_control
    .type   _tx_thread_interrupt_control, @function
_tx_thread_interrupt_control:

    csrr    a1, CSR_GINTENR
    andi    a1, a1, GINTENR_IE

    beqz    a0, _tx_thread_interrupt_disable

    li      t0, GINTENR_IE
    csrs    CSR_GINTENR, t0
    csrsi   mstatus, 0x08
    j       _tx_thread_interrupt_control_exit

_tx_thread_interrupt_disable:
    li      t0, GINTENR_IE
    csrc    CSR_GINTENR, t0
    csrci   mstatus, 0x08
    fence.i

_tx_thread_interrupt_control_exit:
    mv      a0, a1
    ret
