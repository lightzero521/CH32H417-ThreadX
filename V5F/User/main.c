/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2025/03/01
 * Description        : V5F ThreadX demo + context integrity test.
 *********************************************************************************
 * Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "debug.h"
#include "ch32h417_rcc.h"
#include "tx_api.h"
#include "tx_demo.h"

#define DEMO_STACK_SIZE     2048
#define CTX_ROUNDS          64

static TX_THREAD thread_ctx_a;
static TX_THREAD thread_ctx_b;
static TX_THREAD thread_preempt_hi;
static TX_THREAD thread_preempt_lo;
static UCHAR     thread_ctx_a_stack[DEMO_STACK_SIZE];
static UCHAR     thread_ctx_b_stack[DEMO_STACK_SIZE];
static UCHAR     thread_preempt_hi_stack[DEMO_STACK_SIZE];
static UCHAR     thread_preempt_lo_stack[DEMO_STACK_SIZE];

static volatile ULONG ctx_pass;
static volatile ULONG preempt_hits;

static VOID ctx_fail(const char *who, const char *reg, ULONG expect, ULONG got)
{
    printf("V5F CTX FAIL %s %s expect=0x%08lx got=0x%08lx\r\n",
           who, reg, expect, got);
    while (1)
    {
    }
}

/*
 * Fill callee-saved GP/FP, yield, then check. Compiler treats sleep/relinquish
 * as ABI-preserving, so it will not spill these register variables. If ThreadX
 * drops s0-s3 / f8-f9, the values change after the other thread runs.
 */
static VOID ctx_thread_entry(ULONG seed)
{
    const char *who = (seed == 1UL) ? "A" : "B";
    ULONG round;

    for (round = 0; round < CTX_ROUNDS; round++)
    {
        const uint32_t m0 = (uint32_t)seed * 0x01000193u + (uint32_t)round * 0x9E3779B9u;
        const uint32_t e0 = m0 ^ 0xA5A5A5A5u;
        const uint32_t e1 = m0 ^ 0x5A5A5A5Au;
        const uint32_t e2 = m0 + 1u;
        const uint32_t e3 = m0 + 2u;
        const float    ef0 = (float)m0 + 0.5f;
        const float    ef1 = (float)m0 + 1.25f;
        uint32_t       u0;
        uint32_t       u1;

        register uint32_t s0 asm("s0") = e0;
        register uint32_t s1 asm("s1") = e1;
        register uint32_t s2 asm("s2") = e2;
        register uint32_t s3 asm("s3") = e3;
        register float    f8 asm("f8") = ef0;
        register float    f9 asm("f9") = ef1;

        __asm__ volatile ("" :: "r"(s0), "r"(s1), "r"(s2), "r"(s3), "f"(f8), "f"(f9));

        if ((round & 1u) == 0u)
        {
            tx_thread_sleep(1);
        }
        else
        {
            tx_thread_relinquish();
        }

        __asm__ volatile ("" : "+r"(s0), "+r"(s1), "+r"(s2), "+r"(s3), "+f"(f8), "+f"(f9));

        if (s0 != e0)
        {
            ctx_fail(who, "s0", e0, s0);
        }
        if (s1 != e1)
        {
            ctx_fail(who, "s1", e1, s1);
        }
        if (s2 != e2)
        {
            ctx_fail(who, "s2", e2, s2);
        }
        if (s3 != e3)
        {
            ctx_fail(who, "s3", e3, s3);
        }

        __asm__ volatile ("fmv.x.w %0, %1" : "=r"(u0) : "f"(f8));
        __asm__ volatile ("fmv.x.w %0, %1" : "=r"(u1) : "f"(ef0));
        if (u0 != u1)
        {
            ctx_fail(who, "f8", u1, u0);
        }
        __asm__ volatile ("fmv.x.w %0, %1" : "=r"(u0) : "f"(f9));
        __asm__ volatile ("fmv.x.w %0, %1" : "=r"(u1) : "f"(ef1));
        if (u0 != u1)
        {
            ctx_fail(who, "f9", u1, u0);
        }

        ctx_pass++;
    }

    printf("V5F CTX PASS %s rounds=%lu total=%lu\r\n",
           who, (ULONG)CTX_ROUNDS, ctx_pass);

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID preempt_hi_entry(ULONG thread_input)
{
    (VOID)thread_input;
    while (preempt_hits < 40UL)
    {
        tx_thread_sleep(1);
        preempt_hits++;
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 10);
    }
}

/* Busy-loop so SysTick must preempt via the interrupt frame, not sleep.
   Expected values live in memory: the jr-resume path keeps PC in t1. */
static VOID preempt_lo_entry(ULONG thread_input)
{
    ULONG seen = 0;
    volatile uint32_t exp[4];
    volatile uint32_t got[4];
    volatile uint32_t expf[2];
    volatile uint32_t gotf[2];

    (VOID)thread_input;

    while (seen < 32UL)
    {
        exp[0] = 0x11111111u + (uint32_t)seen;
        exp[1] = 0x22222222u + (uint32_t)seen;
        exp[2] = 0x33333333u + (uint32_t)seen;
        exp[3] = 0x44444444u + (uint32_t)seen;
        expf[0] = 0x41040000u + (uint32_t)seen;
        expf[1] = 0x41180000u + (uint32_t)seen;

        __asm__ volatile (
            "lw s0, %0\n\t"
            "lw s1, %1\n\t"
            "lw s2, %2\n\t"
            "lw s3, %3\n\t"
            "fmv.w.x f8, %4\n\t"
            "fmv.w.x f9, %5"
            :
            : "m"(exp[0]), "m"(exp[1]), "m"(exp[2]), "m"(exp[3]),
              "r"(expf[0]), "r"(expf[1])
            : "s0", "s1", "s2", "s3", "f8", "f9");

        while (preempt_hits == seen)
        {
            __asm__ volatile ("" ::: "memory");
        }

        {
            uint32_t gf0;
            uint32_t gf1;

            __asm__ volatile (
                "sw s0, %0\n\t"
                "sw s1, %1\n\t"
                "sw s2, %2\n\t"
                "sw s3, %3\n\t"
                "fmv.x.w %4, f8\n\t"
                "fmv.x.w %5, f9"
                : "=m"(got[0]), "=m"(got[1]), "=m"(got[2]), "=m"(got[3]),
                  "=r"(gf0), "=r"(gf1)
                :
                : "s0", "s1", "s2", "s3", "f8", "f9");
            gotf[0] = gf0;
            gotf[1] = gf1;
        }

        if (got[0] != exp[0])
        {
            ctx_fail("P", "s0", exp[0], got[0]);
        }
        if (got[1] != exp[1])
        {
            ctx_fail("P", "s1", exp[1], got[1]);
        }
        if (got[2] != exp[2])
        {
            ctx_fail("P", "s2", exp[2], got[2]);
        }
        if (got[3] != exp[3])
        {
            ctx_fail("P", "s3", exp[3], got[3]);
        }
        if (gotf[0] != expf[0])
        {
            ctx_fail("P", "f8", expf[0], gotf[0]);
        }
        if (gotf[1] != expf[1])
        {
            ctx_fail("P", "f9", expf[1], gotf[1]);
        }

        seen = preempt_hits;
    }

    printf("V5F PREEMPT PASS hits=%lu\r\n", seen);

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID stack_error_handler(TX_THREAD *thread_ptr)
{
    printf("V5F STACK overflow thread=%s\r\n",
           thread_ptr ? thread_ptr->tx_thread_name : "?");
    while (1)
    {
    }
}

VOID tx_application_define(VOID *first_unused_memory)
{
    (VOID)first_unused_memory;

    tx_thread_stack_error_notify(stack_error_handler);

    tx_thread_create(&thread_ctx_a, "ctx_a", ctx_thread_entry, 1,
                     thread_ctx_a_stack, DEMO_STACK_SIZE,
                     6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_ctx_b, "ctx_b", ctx_thread_entry, 2,
                     thread_ctx_b_stack, DEMO_STACK_SIZE,
                     6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_preempt_hi, "phi", preempt_hi_entry, 0,
                     thread_preempt_hi_stack, DEMO_STACK_SIZE,
                     7, 7, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_preempt_lo, "plo", preempt_lo_entry, 0,
                     thread_preempt_lo_stack, DEMO_STACK_SIZE,
                     9, 9, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_demo_define();
}

int main(void)
{
    SystemAndCoreClockUpdate();
    USART_Printf_Init(460800);
    printf("V5F RST pin=%u por=%u sft=%u iwdg=%u wwdg=%u lkup=%u\r\n",
           (unsigned)RCC_GetFlagStatus(RCC_FLAG_PINRST),
           (unsigned)RCC_GetFlagStatus(RCC_FLAG_PORRST),
           (unsigned)RCC_GetFlagStatus(RCC_FLAG_SFTRST),
           (unsigned)RCC_GetFlagStatus(RCC_FLAG_IWDGRST),
           (unsigned)RCC_GetFlagStatus(RCC_FLAG_WWDGRST),
           (unsigned)RCC_GetFlagStatus(RCC_FLAG_LKUPRSTF));
    RCC_ClearFlag();
    printf("V5F ThreadX SystemCoreClk:%d HCLK:%d\r\n",
           SystemCoreClock, HCLKClock);

#if (Run_Core == Run_Core_V3FandV5F)

#endif

    tx_kernel_enter();

    while (1)
    {
    }
}
