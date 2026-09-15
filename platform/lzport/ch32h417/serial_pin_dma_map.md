# CH32H417 USART Pin Mux 与 DMA 映射

> CH32H417 外设命名为 `USART1`～`USART8`；本工程 `port 0`～`port 7` 分别对应 `USART1`～`USART8`。

## TX/RX Pin Mux

| 工程 port | 外设 | TX 可选引脚 | RX 可选引脚 |
|---:|---|---|---|
| 0 | USART1 | PA9(AF7), PB6(AF7), PB14(AF4), PD13(AF14) | PA10(AF7), PB7(AF7), PB15(AF4), PD12(AF14) |
| 1 | USART2 | PA2(AF7), PD5(AF7) | PA3(AF7), PD6(AF7) |
| 2 | USART3 | PB10(AF7), PC10(AF7), PD8(AF7), PA13(AF4) | PB11(AF7), PC11(AF7), PD9(AF7), PA14(AF4) |
| 3 | USART4 | PF4(AF7), PC6(AF7) | PF3(AF7), PC7(AF7) |
| 4 | USART5 | PE3(AF11), PE0(AF4) | PE2(AF4), PF5(AF4) |
| 5 | USART6 | PA0(AF8), PA12(AF6), PB9(AF8), PC10(AF8), PD1(AF8) | PA1(AF8), PA11(AF6), PB8(AF8), PC11(AF8), PD0(AF8) |
| 6 | USART7 | PB6(AF14), PB13(AF14), PC12(AF8) | PB5(AF14), PB12(AF14), PD2(AF8) |
| 7 | USART8 | PA15(AF11), PB4(AF11), PE8(AF7), PF7(AF7) | PA8(AF11), PB3(AF11), PE7(AF7), PF6(AF7) |

## DMAMUX 请求

| 外设 | TX 请求号 | RX 请求号 | 可分配 DMA 通道 |
|---|---:|---:|---|
| USART1 | 85 | 86 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART2 | 87 | 88 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART3 | 89 | 90 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART4 | 91 | 92 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART5 | 93 | 94 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART6 | 95 | 96 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART7 | 97 | 98 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |
| USART8 | 99 | 100 | DMA1_CH1～CH8 或 DMA2_CH1～CH8 |

DMAMUX 通道与 DMA 通道的关系：

- DMAMUX_CH1～CH8 对应 DMA1_CH1～CH8。
- DMAMUX_CH9～CH16 对应 DMA2_CH1～CH8。
- 同一个 DMA 通道同一时间只能绑定一个请求。

## LZPort 绑定参数

LZPort 不预分配 DMA。上层根据本表传入 `controller`、`channel` 和 `request`：

- `controller`: `0` 表示 DMA1，`1` 表示 DMA2。
- `channel`: `0`～`7` 表示硬件 Channel 1～8。
- `request`: 使用上表对应的 DMAMUX 请求号。

## 来源

- `CH32H417DS0.PDF` V1.8，表 2-2-9，手册页 59～60。
- `CH32H417RM.PDF` V1.7，10.2.3、表 10-2，手册页 149～150。
