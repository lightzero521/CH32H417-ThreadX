/********************************** (C) COPYRIGHT *******************************
* File Name          : ch32h417_it.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2025/03/01
* Description        : Main Interrupt Service Routines.
*********************************************************************************
* Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
    microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "ch32h417_it.h"
#include "debug.h"

void NMI_Handler(void) __attribute__((interrupt));
void HardFault_Handler(void) __attribute__((interrupt));

void NMI_Handler(void)
{
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  printf("V5F HardFault mcause=0x%08lx mepc=0x%08lx mtval=0x%08lx\r\n",
         (unsigned long)__get_MCAUSE(),
         (unsigned long)__get_MEPC(),
         (unsigned long)__get_MTVAL());
  while (1)
  {
  }
}
