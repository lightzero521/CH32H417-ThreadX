#include "tx_demo.h"

#include <stdint.h>
#include "debug.h"
#include "lzport/gpio.h"
#include "lzport/timer.h"

#define DEMO_STACK              2048
#define FPU_STACK               2048
#define LIFE_STACK              2048
#define RR_SLICE                5
#define FPU_THREADS             4
#define TIM7_PERIOD_US          100U
#define SOAK_PERIOD_TICKS       (TX_TIMER_TICKS_PER_SECOND * 10U)
#define MUTEX_BURST             8
#define QUEUE_DEPTH             8

#define PIN_ISR                 LZPORT_GPIO_PIN_0
#define PIN_LATENCY             LZPORT_GPIO_PIN_1
#define PIN_LED_2HZ             LZPORT_GPIO_PIN_2
#define PIN_LED_3HZ             LZPORT_GPIO_PIN_3

static TX_THREAD thread_fpu[FPU_THREADS];
static TX_THREAD thread_latency;
static TX_THREAD thread_torture_lo;
static TX_THREAD thread_torture_hi;
static TX_THREAD thread_race;
static TX_THREAD thread_life;
static TX_THREAD thread_life_worker;
static TX_THREAD thread_led_2hz;
static TX_THREAD thread_led_3hz;
static TX_THREAD thread_rr_a;
static TX_THREAD thread_rr_b;
static TX_THREAD thread_soak;
static TX_THREAD thread_mutex_a;
static TX_THREAD thread_mutex_b;
static TX_THREAD thread_q_prod;
static TX_THREAD thread_q_cons;

static UCHAR stack_fpu[FPU_THREADS][FPU_STACK];
static UCHAR stack_latency[DEMO_STACK];
static UCHAR stack_torture_lo[DEMO_STACK];
static UCHAR stack_torture_hi[DEMO_STACK];
static UCHAR stack_race[DEMO_STACK];
static UCHAR stack_life[DEMO_STACK];
static UCHAR stack_life_worker[LIFE_STACK];
static UCHAR stack_led_2hz[DEMO_STACK];
static UCHAR stack_led_3hz[DEMO_STACK];
static UCHAR stack_rr_a[DEMO_STACK];
static UCHAR stack_rr_b[DEMO_STACK];
static UCHAR stack_soak[DEMO_STACK];
static UCHAR stack_mutex_a[DEMO_STACK];
static UCHAR stack_mutex_b[DEMO_STACK];
static UCHAR stack_q_prod[DEMO_STACK];
static UCHAR stack_q_cons[DEMO_STACK];

static TX_SEMAPHORE sem_latency;
static TX_SEMAPHORE sem_torture;
static TX_SEMAPHORE sem_race;
static TX_MUTEX     mutex_torture;
static TX_MUTEX     mutex_obj;
static TX_QUEUE     queue_obj;
static TX_QUEUE     queue_race;
static ULONG        queue_obj_store[QUEUE_DEPTH];
static ULONG        queue_race_store[QUEUE_DEPTH];
static TX_EVENT_FLAGS_GROUP evt_race;
static TX_TIMER     app_timer;

static volatile ULONG64 g_isr_count;
static volatile ULONG   g_isr_lo;
static volatile ULONG64 g_latency_count;
static volatile ULONG64 g_fpu_ok[FPU_THREADS];
static volatile ULONG64 g_torture_lo;
static volatile ULONG64 g_torture_hi;
static volatile ULONG64 g_race_isr;
static volatile ULONG64 g_race_timeout;
static volatile ULONG64 g_life_cycles;
static volatile ULONG64 g_life_runs;
static volatile ULONG64 g_rr_a;
static volatile ULONG64 g_rr_b;
static volatile ULONG64 g_mutex_ops;
static volatile ULONG64 g_queue_ok;
static volatile ULONG64 g_timer_hits;

static volatile UINT    g_fail;
static volatile const CHAR *g_fail_who;
static volatile const CHAR *g_fail_what;
static volatile ULONG   g_fail_expect;
static volatile ULONG   g_fail_got;

static VOID demo_fail(const CHAR *who, const CHAR *what, ULONG expect, ULONG got)
{
    if (g_fail == 0U)
    {
        g_fail = 1U;
        g_fail_who = who;
        g_fail_what = what;
        g_fail_expect = expect;
        g_fail_got = got;
        printf("V5F FAIL %s %s expect=0x%08lx got=0x%08lx\r\n",
               who, what, expect, got);
    }
    while (1)
    {
    }
}

static VOID app_timer_cb(ULONG input)
{
    (VOID)input;
    g_timer_hits++;
}

static VOID gpio_init(VOID)
{
    lzport_gpio_mode_output(LZPORT_GPIO_C, PIN_ISR, LZPORT_GPIO_SPEED_VERY_HIGH,
                            LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_mode_output(LZPORT_GPIO_C, PIN_LATENCY, LZPORT_GPIO_SPEED_VERY_HIGH,
                            LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_mode_output(LZPORT_GPIO_C, PIN_LED_2HZ, LZPORT_GPIO_SPEED_LOW,
                            LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_mode_output(LZPORT_GPIO_C, PIN_LED_3HZ, LZPORT_GPIO_SPEED_LOW,
                            LZPORT_GPIO_PUSH_PULL);
    lzport_gpio_reset(LZPORT_GPIO_C, PIN_ISR);
    lzport_gpio_reset(LZPORT_GPIO_C, PIN_LATENCY);
    lzport_gpio_reset(LZPORT_GPIO_C, PIN_LED_2HZ);
    lzport_gpio_reset(LZPORT_GPIO_C, PIN_LED_3HZ);
}

static inline __attribute__((always_inline)) VOID fpu_regs_load(const uint32_t *v)
{
    __asm__ volatile (
        "flw    f0,  0*4(%0)\n\t"
        "flw    f1,  1*4(%0)\n\t"
        "flw    f2,  2*4(%0)\n\t"
        "flw    f3,  3*4(%0)\n\t"
        "flw    f4,  4*4(%0)\n\t"
        "flw    f5,  5*4(%0)\n\t"
        "flw    f6,  6*4(%0)\n\t"
        "flw    f7,  7*4(%0)\n\t"
        "flw    f8,  8*4(%0)\n\t"
        "flw    f9,  9*4(%0)\n\t"
        "flw    f10, 10*4(%0)\n\t"
        "flw    f11, 11*4(%0)\n\t"
        "flw    f12, 12*4(%0)\n\t"
        "flw    f13, 13*4(%0)\n\t"
        "flw    f14, 14*4(%0)\n\t"
        "flw    f15, 15*4(%0)\n\t"
        "flw    f16, 16*4(%0)\n\t"
        "flw    f17, 17*4(%0)\n\t"
        "flw    f18, 18*4(%0)\n\t"
        "flw    f19, 19*4(%0)\n\t"
        "flw    f20, 20*4(%0)\n\t"
        "flw    f21, 21*4(%0)\n\t"
        "flw    f22, 22*4(%0)\n\t"
        "flw    f23, 23*4(%0)\n\t"
        "flw    f24, 24*4(%0)\n\t"
        "flw    f25, 25*4(%0)\n\t"
        "flw    f26, 26*4(%0)\n\t"
        "flw    f27, 27*4(%0)\n\t"
        "flw    f28, 28*4(%0)\n\t"
        "flw    f29, 29*4(%0)\n\t"
        "flw    f30, 30*4(%0)\n\t"
        "flw    f31, 31*4(%0)"
        :
        : "r"(v)
        : "memory");
}

static inline __attribute__((always_inline)) VOID fpu_regs_store(uint32_t *v)
{
    __asm__ volatile (
        "fsw    f0,  0*4(%0)\n\t"
        "fsw    f1,  1*4(%0)\n\t"
        "fsw    f2,  2*4(%0)\n\t"
        "fsw    f3,  3*4(%0)\n\t"
        "fsw    f4,  4*4(%0)\n\t"
        "fsw    f5,  5*4(%0)\n\t"
        "fsw    f6,  6*4(%0)\n\t"
        "fsw    f7,  7*4(%0)\n\t"
        "fsw    f8,  8*4(%0)\n\t"
        "fsw    f9,  9*4(%0)\n\t"
        "fsw    f10, 10*4(%0)\n\t"
        "fsw    f11, 11*4(%0)\n\t"
        "fsw    f12, 12*4(%0)\n\t"
        "fsw    f13, 13*4(%0)\n\t"
        "fsw    f14, 14*4(%0)\n\t"
        "fsw    f15, 15*4(%0)\n\t"
        "fsw    f16, 16*4(%0)\n\t"
        "fsw    f17, 17*4(%0)\n\t"
        "fsw    f18, 18*4(%0)\n\t"
        "fsw    f19, 19*4(%0)\n\t"
        "fsw    f20, 20*4(%0)\n\t"
        "fsw    f21, 21*4(%0)\n\t"
        "fsw    f22, 22*4(%0)\n\t"
        "fsw    f23, 23*4(%0)\n\t"
        "fsw    f24, 24*4(%0)\n\t"
        "fsw    f25, 25*4(%0)\n\t"
        "fsw    f26, 26*4(%0)\n\t"
        "fsw    f27, 27*4(%0)\n\t"
        "fsw    f28, 28*4(%0)\n\t"
        "fsw    f29, 29*4(%0)\n\t"
        "fsw    f30, 30*4(%0)\n\t"
        "fsw    f31, 31*4(%0)"
        :
        : "r"(v)
        : "memory");
}

static uint32_t fcsr_get(VOID)
{
    uint32_t v;

    __asm__ volatile ("csrr %0, fcsr" : "=r"(v));
    return v;
}

static VOID fcsr_set(uint32_t v)
{
    __asm__ volatile ("csrw fcsr, %0" :: "r"(v));
}

static VOID tim7_cb(lzport_timer timer, void *user)
{
    ULONG msg;

    (VOID)timer;
    (VOID)user;

    lzport_gpio_set(LZPORT_GPIO_C, PIN_ISR);

    g_isr_count++;
    g_isr_lo++;
    tx_semaphore_put(&sem_latency);

    if ((g_isr_count % 3U) == 0U)
    {
        tx_semaphore_put(&sem_torture);
    }

    if ((g_isr_count % 13U) == 0U)
    {
        msg = (ULONG)g_isr_count;
        tx_queue_send(&queue_race, &msg, TX_NO_WAIT);
        tx_event_flags_set(&evt_race, 0x1U, TX_OR);
        tx_semaphore_put(&sem_race);
    }

    lzport_gpio_reset(LZPORT_GPIO_C, PIN_ISR);
}

static VOID fpu_touch(ULONG id)
{
    static float acc[FPU_THREADS] = {1.0f, 2.0f, 3.0f, 4.0f};

    acc[id] = acc[id] * 1.00001f + (0.25f * (float)(id + 1U));
}

static VOID fpu_entry(ULONG id)
{
    uint32_t expect[32];
    uint32_t got[32];
    uint32_t fcsr_exp;
    uint32_t i;
    ULONG    isr_mark;
    CHAR     who[4];

    who[0] = 'F';
    who[1] = (CHAR)('0' + (CHAR)id);
    who[2] = 0;
    fcsr_exp = ((uint32_t)id & 3U) << 5;

    for (i = 0U; i < 32U; i++)
    {
        expect[i] = 0x3F800000U + ((uint32_t)id << 12) + (i << 3);
    }

    while (g_fail == 0U)
    {
        fcsr_set(fcsr_exp);
        fpu_regs_load(expect);

        if (id <= 1U)
        {
            isr_mark = g_isr_lo + 4U;
            while ((g_isr_lo < isr_mark) && (g_fail == 0U))
            {
                __asm__ volatile ("" ::: "memory");
            }
        }
        else if (id == 2U)
        {
            tx_thread_sleep(1);
        }
        else
        {
            tx_thread_relinquish();
        }

        fpu_regs_store(got);
        if (fcsr_get() != fcsr_exp)
        {
            demo_fail(who, "fcsr", fcsr_exp, fcsr_get());
        }
        for (i = 0U; i < 32U; i++)
        {
            if (got[i] != expect[i])
            {
                demo_fail(who, "freg", expect[i], got[i]);
            }
        }

        fpu_touch(id);
        g_fpu_ok[id]++;
        tx_thread_sleep(1);
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID latency_entry(ULONG id)
{
    (VOID)id;
    while (g_fail == 0U)
    {
        tx_semaphore_get(&sem_latency, TX_WAIT_FOREVER);
        lzport_gpio_toggle(LZPORT_GPIO_C, PIN_LATENCY);
        g_latency_count++;
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID torture_lo_entry(ULONG id)
{
    UINT i;

    (VOID)id;
    while (g_fail == 0U)
    {
        tx_mutex_get(&mutex_torture, TX_WAIT_FOREVER);
        for (i = 0U; i < MUTEX_BURST; i++)
        {
            g_torture_lo++;
        }
        tx_mutex_put(&mutex_torture);
        /* Same-priority relinquish does not let life/rr run; they are lower. */
        tx_thread_sleep(1);
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID torture_hi_entry(ULONG id)
{
    (VOID)id;
    while (g_fail == 0U)
    {
        tx_semaphore_get(&sem_torture, TX_WAIT_FOREVER);
        g_torture_hi++;
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID race_entry(ULONG id)
{
    ULONG msg;
    ULONG flags;
    UINT  status;

    (VOID)id;
    while (g_fail == 0U)
    {
        status = tx_semaphore_get(&sem_race, 1);
        if (status == TX_SUCCESS)
        {
            g_race_isr++;
        }
        else if (status == TX_NO_INSTANCE)
        {
            g_race_timeout++;
        }

        status = tx_queue_receive(&queue_race, &msg, 1);
        if ((status != TX_SUCCESS) && (status != TX_QUEUE_EMPTY))
        {
            demo_fail("race", "queue", TX_SUCCESS, status);
        }

        flags = 0;
        status = tx_event_flags_get(&evt_race, 0x1U, TX_OR_CLEAR, &flags, 1);
        if ((status != TX_SUCCESS) && (status != TX_NO_EVENTS))
        {
            demo_fail("race", "event", TX_SUCCESS, status);
        }
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID life_worker_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        g_life_runs++;
        tx_thread_sleep(1);
    }
}

static VOID life_entry(ULONG id)
{
    UINT old_priority;

    (VOID)id;
    while (g_fail == 0U)
    {
        if (tx_thread_create(&thread_life_worker, "life_w", life_worker_entry, 0,
                             stack_life_worker, LIFE_STACK,
                             12, 12, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
        {
            demo_fail("life", "create", TX_SUCCESS, 1);
        }

        tx_thread_sleep(2);
        tx_thread_suspend(&thread_life_worker);
        tx_thread_sleep(1);
        tx_thread_resume(&thread_life_worker);
        tx_thread_priority_change(&thread_life_worker, 13, &old_priority);
        tx_thread_priority_change(&thread_life_worker, 12, &old_priority);
        tx_thread_relinquish();
        tx_thread_terminate(&thread_life_worker);
        tx_thread_delete(&thread_life_worker);
        g_life_cycles++;
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID led_2hz_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        lzport_gpio_toggle(LZPORT_GPIO_C, PIN_LED_2HZ);
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 4);
    }
}

static VOID led_3hz_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        lzport_gpio_toggle(LZPORT_GPIO_C, PIN_LED_3HZ);
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 6);
    }
}

static VOID rr_a_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        g_rr_a++;
    }
}

static VOID rr_b_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        g_rr_b++;
    }
}

static VOID mutex_worker(ULONG id)
{
    (VOID)id;
    while (g_fail == 0U)
    {
        tx_mutex_get(&mutex_obj, TX_WAIT_FOREVER);
        g_mutex_ops++;
        tx_mutex_put(&mutex_obj);
        tx_thread_sleep(1);
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID queue_prod(ULONG id)
{
    ULONG n = 1;

    (VOID)id;
    while (g_fail == 0U)
    {
        tx_queue_send(&queue_obj, &n, TX_WAIT_FOREVER);
        n++;
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID queue_cons(ULONG id)
{
    ULONG n;

    (VOID)id;
    while (g_fail == 0U)
    {
        tx_queue_receive(&queue_obj, &n, TX_WAIT_FOREVER);
        g_queue_ok++;
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID soak_dump(ULONG round)
{
    printf("V5F SOAK r=%lu isr=%lu lat=%lu fpu=%lu/%lu/%lu/%lu "
           "tlo=%lu thi=%lu race=%lu/%lu life=%lu rr=%lu/%lu "
           "mtx=%lu q=%lu tim=%lu\r\n",
           round,
           (ULONG)g_isr_count,
           (ULONG)g_latency_count,
           (ULONG)g_fpu_ok[0], (ULONG)g_fpu_ok[1],
           (ULONG)g_fpu_ok[2], (ULONG)g_fpu_ok[3],
           (ULONG)g_torture_lo, (ULONG)g_torture_hi,
           (ULONG)g_race_isr, (ULONG)g_race_timeout,
           (ULONG)g_life_cycles,
           (ULONG)g_rr_a, (ULONG)g_rr_b,
           (ULONG)g_mutex_ops, (ULONG)g_queue_ok,
           (ULONG)g_timer_hits);
}

static VOID soak_entry(ULONG id)
{
    ULONG64 last_isr = 0;
    ULONG64 last_lat = 0;
    ULONG64 last_fpu = 0;
    ULONG64 last_thi = 0;
    ULONG64 last_life = 0;
    ULONG64 last_rr = 0;
    ULONG   round = 0;
    ULONG   i;
    ULONG64 fpu_sum;
    const CHAR *why;

    (VOID)id;
    tx_thread_sleep(SOAK_PERIOD_TICKS);

    while (1)
    {
        round++;
        fpu_sum = 0;
        for (i = 0U; i < FPU_THREADS; i++)
        {
            fpu_sum += g_fpu_ok[i];
        }

        soak_dump(round);

        why = 0;
        if (g_fail != 0U)
        {
            printf("V5F SOAK FAIL r=%lu who=%s %s exp=0x%08lx got=0x%08lx\r\n",
                   round, g_fail_who, g_fail_what, g_fail_expect, g_fail_got);
        }
        else if (round == 1U)
        {
            if (g_isr_count == 0U)
            {
                why = "isr0";
            }
            else if (g_latency_count == 0U)
            {
                why = "lat0";
            }
            else if (fpu_sum == 0U)
            {
                why = "fpu0";
            }
            else if (g_torture_hi == 0U)
            {
                why = "thi0";
            }
            else if (g_life_cycles == 0U)
            {
                why = "life0";
            }
            else if ((g_rr_a + g_rr_b) == 0U)
            {
                why = "rr0";
            }
        }
        else if (g_isr_count == last_isr)
        {
            why = "isr";
        }
        else if (g_latency_count == last_lat)
        {
            why = "lat";
        }
        else if (fpu_sum == last_fpu)
        {
            why = "fpu";
        }
        else if (g_torture_hi == last_thi)
        {
            why = "thi";
        }
        else if (g_life_cycles == last_life)
        {
            why = "life";
        }
        else if ((g_rr_a + g_rr_b) == last_rr)
        {
            why = "rr";
        }

        if (why != 0)
        {
            if ((why[0] == 'l') && (why[1] == 'i'))
            {
                demo_fail("soak", why, (ULONG)last_life, (ULONG)g_life_cycles);
            }
            else if ((why[0] == 'r') && (why[1] == 'r'))
            {
                demo_fail("soak", why, (ULONG)last_rr, (ULONG)(g_rr_a + g_rr_b));
            }
            else
            {
                demo_fail("soak", why, (ULONG)last_isr, (ULONG)g_isr_count);
            }
        }

        last_isr = g_isr_count;
        last_lat = g_latency_count;
        last_fpu = fpu_sum;
        last_thi = g_torture_hi;
        last_life = g_life_cycles;
        last_rr = g_rr_a + g_rr_b;
        tx_thread_sleep(SOAK_PERIOD_TICKS);
    }
}

VOID tx_demo_define(VOID)
{
    lzport_timer_config tim7;
    UINT i;

    gpio_init();

    tx_semaphore_create(&sem_latency, "lat", 0);
    tx_semaphore_create(&sem_torture, "tor", 0);
    tx_semaphore_create(&sem_race, "race", 0);
    tx_mutex_create(&mutex_torture, "tmtx", TX_NO_INHERIT);
    tx_mutex_create(&mutex_obj, "omtx", TX_NO_INHERIT);
    tx_queue_create(&queue_obj, "oq", TX_1_ULONG, queue_obj_store, sizeof(queue_obj_store));
    tx_queue_create(&queue_race, "rq", TX_1_ULONG, queue_race_store, sizeof(queue_race_store));
    tx_event_flags_create(&evt_race, "re");
    tx_timer_create(&app_timer, "tapp", app_timer_cb, 0, 10, 10, TX_AUTO_ACTIVATE);

    tx_thread_create(&thread_latency, "lat", latency_entry, 0,
                     stack_latency, DEMO_STACK,
                     2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_torture_hi, "thi", torture_hi_entry, 0,
                     stack_torture_hi, DEMO_STACK,
                     3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_race, "race", race_entry, 0,
                     stack_race, DEMO_STACK,
                     4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_soak, "soak", soak_entry, 0,
                     stack_soak, DEMO_STACK,
                     5, 5, TX_NO_TIME_SLICE, TX_AUTO_START);

    {
        static CHAR fpu_name[FPU_THREADS][5] = {
            "fpu0", "fpu1", "fpu2", "fpu3"
        };

        for (i = 0U; i < FPU_THREADS; i++)
        {
            tx_thread_create(&thread_fpu[i], fpu_name[i], fpu_entry, i,
                             stack_fpu[i], FPU_STACK,
                             8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);
        }
    }

    tx_thread_create(&thread_torture_lo, "tlo", torture_lo_entry, 0,
                     stack_torture_lo, DEMO_STACK,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_life, "life", life_entry, 0,
                     stack_life, DEMO_STACK,
                     12, 12, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_mutex_a, "ma", mutex_worker, 0,
                     stack_mutex_a, DEMO_STACK,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_mutex_b, "mb", mutex_worker, 1,
                     stack_mutex_b, DEMO_STACK,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_q_prod, "qp", queue_prod, 0,
                     stack_q_prod, DEMO_STACK,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_q_cons, "qc", queue_cons, 0,
                     stack_q_cons, DEMO_STACK,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_rr_a, "rra", rr_a_entry, 0,
                     stack_rr_a, DEMO_STACK,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_rr_b, "rrb", rr_b_entry, 0,
                     stack_rr_b, DEMO_STACK,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_led_2hz, "led2", led_2hz_entry, 0,
                     stack_led_2hz, DEMO_STACK,
                     10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_led_3hz, "led3", led_3hz_entry, 0,
                     stack_led_3hz, DEMO_STACK,
                     10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);

    tim7.interval_us = TIM7_PERIOD_US;
    tim7.mode = LZPORT_TIMER_REPEATING;
    tim7.callback = tim7_cb;
    tim7.user = 0;
    if (lzport_timer_init(LZPORT_TIMER_1, &tim7) != LZPORT_OK)
    {
        printf("V5F TIM7 lzport init FAIL\r\n");
        while (1)
        {
        }
    }
    lzport_timer_start(LZPORT_TIMER_1);

    printf("V5F RTOS soak: TIM7=lzport 10kHz  PC0=ISR  PC1=lat  PC2=2Hz  PC3=3Hz\r\n");
}
