ThreadX port: CH32H417 QingKe V5F (GNU / MRS)

This is a V5F-only ThreadX port. V3F is left as bare metal.

Why this is not the stock RISC-V32 port
---------------------------------------
QingKe V5F is closer to Zephyr's wch,qingke-v5f + wch,pfic than to a
standard RV32 CLINT/PLIC core.

  1. Global IRQ mask is CSR 0x800 (GINTENR), same as WCH __enable_irq /
     __disable_irq. Thread-mode csrci/csrw mstatus is illegal (mcause=2).
  2. PFIC, not CLINT/PLIC. An ISR must leave via mret. Jumping to the
     scheduler without mret wedges further interrupts. Thread-mode resume
     of an interrupt frame uses jr (t1 holds PC); do not csrw mepc.
  3. Hardware stacking (HPE, INTSYSCR 0x804) is OFF. ThreadX uses software
     stacking. HPE plus a switched SP pops the wrong frame on mret.
  4. Tick is SysTick1 at 0xE000F080, IRQ 13. Compare flag is SysTick0->ISR
     bit1. Reload uses HCLKClock (often 100 MHz), not SystemCoreClock
     (400 MHz).
  5. Vector table is WCH mtvec mode 3 (startup_ch32h417_v5f.S).

Files
-----
  inc/tx_port.h
  inc/tx_qke_v5f.h
  src/tx_initialize_low_level.c
  src/tx_systick_handler.S
  src/tx_thread_*.S
  src/tx_timer_interrupt.S

Application ISR (ARM-style)
---------------------------
Write a normal C handler with __attribute__((interrupt)), not
WCH-Interrupt-fast (HPE stays off). Call ThreadX APIs as usual
(tx_semaphore_put, tx_queue_send, ...). If a higher-priority thread
becomes ready, _tx_thread_system_return pends Software_IRQn (the
QingKe PendSV). After the peripheral ISR mrets, SW_Handler runs
context_save/restore and either switches or returns to the
interrupted thread.

Do not use __attribute__((interrupt("WCH-Interrupt-fast"))).

Shared interrupts (IRQn > 31) must be allocated to V5F:

  NVIC_SetAllocateIRQ(IRQn, Core_ID_V5F);

Tick rate
---------
TX_TIMER_TICKS_PER_SECOND is 1000 (V5F/User/tx_user.h).

On-target checks in the V5F demo
--------------------------------
  CTX PASS A/B     solicited switch (sleep / relinquish)
  PREEMPT PASS     SysTick preemption of a busy thread
  TIM7 PASS        0.5 s one-shot, ISR puts a semaphore
  MUTEX/QUEUE/EVENT/TIMER
                   ThreadX objects
  RR PASS          two equal-priority threads with time-slice
