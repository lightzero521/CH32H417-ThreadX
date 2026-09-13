/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2025/03/01
 * Description        : V5F ThreadX demo.
 *********************************************************************************
 * Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
    microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "debug.h"
#include "tx_api.h"

#define DEMO_STACK_SIZE     1024

static TX_THREAD thread_led;
static TX_THREAD thread_tick;
static UCHAR     thread_led_stack[DEMO_STACK_SIZE];
static UCHAR     thread_tick_stack[DEMO_STACK_SIZE];

static VOID thread_led_entry(ULONG thread_input)
{
    (VOID)thread_input;
    while (1)
    {
        printf("V5F ThreadX thread_led\r\n");
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID thread_tick_entry(ULONG thread_input)
{
    ULONG count = 0;

    (VOID)thread_input;
    while (1)
    {
        count++;
        printf("V5F ThreadX ticks=%lu count=%lu\r\n",
               tx_time_get(), count);
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2);
    }
}

VOID tx_application_define(VOID *first_unused_memory)
{
    (VOID)first_unused_memory;

    tx_thread_create(&thread_led, "led", thread_led_entry, 0,
                     thread_led_stack, DEMO_STACK_SIZE,
                     8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_tick, "tick", thread_tick_entry, 0,
                     thread_tick_stack, DEMO_STACK_SIZE,
                     10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
}

int main(void)
{
    SystemAndCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("V5F ThreadX SystemCoreClk:%d\r\n", SystemCoreClock);

#if (Run_Core == Run_Core_V3FandV5F)
    HSEM_FastTake(HSEM_ID0);
    HSEM_ReleaseOneSem(HSEM_ID0, 0);
#endif

    tx_kernel_enter();

    while (1)
    {
    }
}
