/**************************************************************************/
/*                                                                        */
/*  QingKe V5F (CH32H417) helpers for the ThreadX port                    */
/*                                                                        */
/*  V5F is not a standard RV32 CLINT/PLIC core:                           */
/*    - Global IRQ mask is CSR 0x800 (GINTENR), not only mstatus.MIE      */
/*    - PFIC + optional HPE (INTSYSCR 0x804)                              */
/*    - Tick source is memory-mapped SysTick0, not mtime                  */
/*                                                                        */
/*  Same approach as Zephyr's WCH/QingKe support: software stacking and   */
/*  always return from a switch via mret so PFIC interrupt level is       */
/*  retired (RISCV_ALWAYS_SWITCH_THROUGH_ECALL equivalent).               */
/*                                                                        */
/**************************************************************************/

#ifndef TX_QKE_V5F_H
#define TX_QKE_V5F_H

#define CSR_GINTENR                 0x800
#define GINTENR_IE                  0x88

#define CSR_INTSYSCR                0x804

#define SYSTICK0_BASE               0xE000F000u
#define SYSTICK_CTLR_STE            (1u << 0)
#define SYSTICK_CTLR_STIE           (1u << 1)
#define SYSTICK_CTLR_STCLK          (1u << 2)
#define SYSTICK_CTLR_STRE           (1u << 3)

#ifndef TX_TIMER_TICKS_PER_SECOND
#define TX_TIMER_TICKS_PER_SECOND   1000
#endif

#endif
