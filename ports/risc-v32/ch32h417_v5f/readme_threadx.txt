ThreadX port: CH32H417 QingKe V5F (GNU / MRS)

Why this is not the stock RISC-V32 port
---------------------------------------
QingKe V5F is closer to Zephyr's wch,qingke-v5f + wch,pfic than to a
standard RV32 CLINT/PLIC core.

  1. Global IRQ enable is CSR 0x800 (GINTENR), same as WCH __enable_irq.
  2. PFIC (not CLINT/PLIC). Switching a thread with `ret` after an ISR
     leaves PFIC's interrupt level set, and further IRQs stop. Zephyr
     uses CONFIG_RISCV_ALWAYS_SWITCH_THROUGH_ECALL; this port always
     returns via `mret`.
  3. Hardware stack (HPE, INTSYSCR 0x804) is turned OFF. ThreadX uses
     software stacking. Leaving HPE on and then switching SP will
     hardware-pop the wrong frame on `mret`.
  4. Tick is SysTick0 at 0xE000F000, IRQ 12, not mtime/mtimecmp.
  5. Vector table is WCH mtvec mode 3 (startup_ch32h417_v5f.S).

Files
-----
  inc/tx_port.h
  inc/tx_qke_v5f.h
  src/tx_initialize_low_level.c
  src/tx_systick_handler.S
  src/tx_thread_*.S
  src/tx_timer_interrupt.S

Application ISR that must preempt
---------------------------------
Do not use __attribute__((interrupt("WCH-Interrupt-fast"))). Write a
naked wrapper that allocates a 32-word frame, saves ra, calls
_tx_thread_context_save, runs the C ISR, then jumps to
_tx_thread_context_restore (see tx_systick_handler.S).

Tick rate
---------
TX_TIMER_TICKS_PER_SECOND defaults to 1000 (tx_user.h).
