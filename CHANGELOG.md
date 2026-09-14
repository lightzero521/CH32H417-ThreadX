# Changelog

## Unreleased

- 外设 ISR 可按 ARM 方式写 C 并直接调用 ThreadX API：`Software_IRQn`/`SW_Handler` 充当 PendSV，中断退出后再调度。
- 修正 V5F 节拍与关中断：SysTick1 + HCLK、全局 IRQ 只走 GINTENR；ISR 必须 `mret` 否则 PFIC 卡死。
- 修复 V5F 上机后立刻 HardFault 复位：首次调度不再从线程态 `mret`，SysTick 延后到调度器启动，HardFault 打印 mcause/mepc。
- 新增 CH32H417 QingKe V5F ThreadX 移植（`ports/risc-v32/ch32h417_v5f`）。
