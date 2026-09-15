/**************************************************************************/
/*                                                                        */
/*  QingKe V5F (CH32H417) helpers for the ThreadX port                    */
/*                                                                        */
/*  V5F is not a standard RV32 CLINT/PLIC core:                           */
/*    - Global IRQ mask is CSR 0x800 (GINTENR), matching WCH HAL          */
/*    - Thread-mode mstatus CSR ops are illegal (csrci/csrw both trap)    */
/*    - Startup writes 0x6088 then mret; do not write MPP 0x1880/0x1888   */
/*    - PFIC + optional HPE (INTSYSCR 0x804)                              */
/*    - Tick is SysTick1 @ HCLKClock; flag is SysTick0->ISR bit1          */
/*    - ISR must always leave via mret; jumping to schedule wedges PFIC   */
/*    - C peripheral ISRs pend Software_IRQn (PendSV); SW_Handler switches  */
/*    - FPU is single-precision F only (ilp32f). ilp32d / D is #error'd     */
/*                                                                        */
/*  Same approach as Zephyr's WCH/QingKe support: software stacking and   */
/*  always return from a switch via mret so PFIC interrupt level is       */
/*  retired (RISCV_ALWAYS_SWITCH_THROUGH_ECALL equivalent).               */
/*                                                                        */
/**************************************************************************/

#ifndef TX_QKE_V5F_H
#define TX_QKE_V5F_H

#if defined(__riscv_float_abi_double) || (defined(__riscv_flen) && (__riscv_flen == 64))
#error "QingKe V5F has single-precision F only (no D). Use ilp32f, not ilp32d."
#endif

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
