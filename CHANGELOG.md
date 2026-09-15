# Changelog

## Unreleased

- 外设 ISR 可按 ARM 方式写 C 并直接调用 ThreadX API：`Software_IRQn`/`SW_Handler` 充当 PendSV，中断退出后再调度。
- 抢占恢复改为在 ISR 内 `mret` 到新线程，避免 `jr t1` 把 t1 写成 PC 后改写 ITCM、表现为复位循环。
- C 外设 ISR 不使用浮点：QingKe HPE 只压整数寄存器，软件中断也不保存 `fcsr`；这是 RISC-V/QingKe 相对 Cortex-M FPU 的常见差异，不是 V5F 独有缺陷。选择 `ilp32d` 时编译失败。
- 修正 V5F 节拍与关中断：SysTick1 + HCLK、全局 IRQ 只走 GINTENR；ISR 必须 `mret` 否则 PFIC 卡死。
- 修复 V5F 上机后立刻 HardFault 复位：首次调度不再从线程态 `mret`，SysTick 延后到调度器启动，HardFault 打印 mcause/mepc。
- 新增 lzport（TIM6/TIM7、GPIO 等）以及 V5F soak：TIM7 10 kHz、FPU 线程、lifecycle、PC2=2 Hz / PC3=3 Hz。
- 移植目录改为官方布局 `threadx/ports/risc-v32/ch32h417_v5f`。
