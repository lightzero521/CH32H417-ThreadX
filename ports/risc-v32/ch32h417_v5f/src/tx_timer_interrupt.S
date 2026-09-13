/**************************************************************************/
/*  _tx_timer_interrupt — same algorithm as official RISC-V32 GNU port    */
/**************************************************************************/

    .section .text
    .align  2
    .global _tx_timer_interrupt
    .type   _tx_timer_interrupt, @function
_tx_timer_interrupt:

    la      t0, _tx_timer_system_clock
    lw      t1, 0(t0)
    la      t2, _tx_timer_time_slice
    lw      t3, 0(t2)
    addi    t1, t1, 1
    sw      t1, 0(t0)
    li      t6, 0

    beqz    t3, _tx_timer_no_time_slice
    addi    t3, t3, -1
    sw      t3, 0(t2)
    bgtz    t3, _tx_timer_no_time_slice
    li      t1, 1
    la      t4, _tx_timer_expired_time_slice
    sw      t1, 0(t4)
    ori     t6, t6, 1

_tx_timer_no_time_slice:

    la      t0, _tx_timer_current_ptr
    lw      t1, 0(t0)
    lw      t3, 0(t1)
    la      t2, _tx_timer_expired
    li      t4, 1
    beqz    t3, _tx_timer_no_timer

    ori     t6, t6, 2
    sw      t4, 0(t2)
    j       _tx_timer_done

_tx_timer_no_timer:
    la      t2, _tx_timer_list_end
    lw      t3, 0(t2)
    addi    t1, t1, 4
    sw      t1, 0(t0)
    bne     t1, t3, _tx_timer_skip_wrap

    la      t2, _tx_timer_list_start
    lw      t4, 0(t2)
    sw      t4, 0(t0)

_tx_timer_skip_wrap:
_tx_timer_done:

    beqz    t6, _tx_timer_nothing_expired
    addi    sp, sp, -16
    sw      t6, 0(sp)
    sw      ra, 4(sp)

    andi    t2, t6, 2
    beqz    t2, _tx_timer_dont_activate
    call    _tx_timer_expiration_process
    lw      t6, 0(sp)

_tx_timer_dont_activate:
    andi    t2, t6, 1
    beqz    t2, _tx_timer_not_ts_expiration
    call    _tx_thread_time_slice

_tx_timer_not_ts_expiration:
    lw      ra, 4(sp)
    addi    sp, sp, 16

_tx_timer_nothing_expired:
    ret
