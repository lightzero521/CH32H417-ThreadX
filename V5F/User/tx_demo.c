#include "tx_demo.h"
#include "debug.h"
#include "ch32h417_tim.h"
#include "ch32h417_rcc.h"

#define DEMO_STACK_SIZE     1536
#define MUTEX_LOOPS         200
#define QUEUE_COUNT         32
#define RR_SLICE            5

static TX_THREAD thread_mutex_a;
static TX_THREAD thread_mutex_b;
static TX_THREAD thread_q_prod;
static TX_THREAD thread_q_cons;
static TX_THREAD thread_evt_set;
static TX_THREAD thread_evt_get;
static TX_THREAD thread_tim7;
static TX_THREAD thread_obj;
static TX_THREAD thread_rr_a;
static TX_THREAD thread_rr_b;
static TX_THREAD thread_rr_watch;

static UCHAR stack_mutex_a[DEMO_STACK_SIZE];
static UCHAR stack_mutex_b[DEMO_STACK_SIZE];
static UCHAR stack_q_prod[DEMO_STACK_SIZE];
static UCHAR stack_q_cons[DEMO_STACK_SIZE];
static UCHAR stack_evt_set[DEMO_STACK_SIZE];
static UCHAR stack_evt_get[DEMO_STACK_SIZE];
static UCHAR stack_tim7[DEMO_STACK_SIZE];
static UCHAR stack_obj[DEMO_STACK_SIZE];
static UCHAR stack_rr_a[DEMO_STACK_SIZE];
static UCHAR stack_rr_b[DEMO_STACK_SIZE];
static UCHAR stack_rr_watch[DEMO_STACK_SIZE];

static TX_MUTEX      mutex;
static TX_SEMAPHORE  sem_mutex_done;
static TX_SEMAPHORE  sem_tim7;
static TX_QUEUE      queue;
static ULONG         queue_storage[QUEUE_COUNT];
static TX_SEMAPHORE  sem_q_done;
static TX_EVENT_FLAGS_GROUP events;
static TX_TIMER      app_timer;

static volatile ULONG mutex_count;
static volatile ULONG queue_sum;
static volatile ULONG timer_hits;
static volatile ULONG rr_count_a;
static volatile ULONG rr_count_b;
static volatile ULONG tim7_isr_ticks;

static VOID app_timer_cb(ULONG input)
{
    (VOID)input;
    timer_hits++;
}

VOID tx_demo_tim7_isr(VOID)
{
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
        TIM_Cmd(TIM7, DISABLE);
        tim7_isr_ticks = tx_time_get();
        tx_semaphore_put(&sem_tim7);
    }
}

static VOID tim7_oneshot_start(VOID)
{
    TIM_TimeBaseInitTypeDef init;
    ULONG hz;
    ULONG psc;

    hz = HCLKClock;
    if (hz == 0U)
    {
        hz = SystemCoreClock;
    }

    /* 10 kHz tick, ARR=5000 -> 0.5 s one-shot. */
    psc = hz / 10000U;
    if (psc == 0U)
    {
        psc = 1U;
    }

    RCC_HB1PeriphClockCmd(RCC_HB1Periph_TIM7, ENABLE);
    TIM_DeInit(TIM7);

    TIM_TimeBaseStructInit(&init);
    init.TIM_Prescaler = (uint16_t)(psc - 1U);
    init.TIM_CounterMode = TIM_CounterMode_Up;
    init.TIM_Period = 5000U - 1U;
    init.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM7, &init);
    TIM_SelectOnePulseMode(TIM7, TIM_OPMode_Single);
    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
    TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

    if (TIM7_IRQn > 31)
    {
        NVIC_SetAllocateIRQ(TIM7_IRQn, Core_ID_V5F);
    }
    NVIC_SetPriority(TIM7_IRQn, 0x90);
    NVIC_EnableIRQ(TIM7_IRQn);

    TIM_Cmd(TIM7, ENABLE);
}

static VOID mutex_worker(ULONG id)
{
    ULONG i;

    (VOID)id;
    for (i = 0; i < MUTEX_LOOPS; i++)
    {
        tx_mutex_get(&mutex, TX_WAIT_FOREVER);
        mutex_count++;
        tx_mutex_put(&mutex);
    }
    tx_semaphore_put(&sem_mutex_done);
}

static VOID queue_prod(ULONG id)
{
    ULONG n;

    (VOID)id;
    for (n = 1; n <= QUEUE_COUNT; n++)
    {
        tx_queue_send(&queue, &n, TX_WAIT_FOREVER);
    }
}

static VOID queue_cons(ULONG id)
{
    ULONG n;
    ULONG i;

    (VOID)id;
    queue_sum = 0;
    for (i = 0; i < QUEUE_COUNT; i++)
    {
        tx_queue_receive(&queue, &n, TX_WAIT_FOREVER);
        queue_sum += n;
    }
    tx_semaphore_put(&sem_q_done);
}

static VOID evt_set_entry(ULONG id)
{
    (VOID)id;
    tx_thread_sleep(2);
    tx_event_flags_set(&events, 0x1, TX_OR);
    tx_thread_sleep(2);
    tx_event_flags_set(&events, 0x2, TX_OR);
}

static VOID evt_get_entry(ULONG id)
{
    ULONG actual = 0;

    (VOID)id;
    tx_event_flags_get(&events, 0x3, TX_AND, &actual, TX_WAIT_FOREVER);
    if (actual == 0x3)
    {
        printf("V5F EVENT PASS flags=0x%lx\r\n", actual);
    }
    else
    {
        printf("V5F EVENT FAIL flags=0x%lx\r\n", actual);
    }
}

static VOID tim7_wait_entry(ULONG id)
{
    ULONG start;
    ULONG delta;

    (VOID)id;
    start = tx_time_get();
    if (tx_semaphore_get(&sem_tim7, TX_TIMER_TICKS_PER_SECOND * 2) != TX_SUCCESS)
    {
        printf("V5F TIM7 FAIL timeout start=%lu\r\n", start);
        return;
    }
    delta = tim7_isr_ticks - start;
    if ((delta >= 400U) && (delta <= 700U))
    {
        printf("V5F TIM7 PASS oneshot ticks=%lu delta=%lu\r\n",
               tim7_isr_ticks, delta);
    }
    else
    {
        printf("V5F TIM7 FAIL oneshot ticks=%lu delta=%lu (want ~500)\r\n",
               tim7_isr_ticks, delta);
    }
}

static VOID rr_a_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        rr_count_a++;
    }
}

static VOID rr_b_entry(ULONG id)
{
    (VOID)id;
    while (1)
    {
        rr_count_b++;
    }
}

static VOID rr_watch_entry(ULONG id)
{
    ULONG a;
    ULONG b;

    (VOID)id;
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2);
    a = rr_count_a;
    b = rr_count_b;
    if ((a > 1000U) && (b > 1000U))
    {
        printf("V5F RR PASS a=%lu b=%lu\r\n", a, b);
    }
    else
    {
        printf("V5F RR FAIL a=%lu b=%lu\r\n", a, b);
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

static VOID object_entry(ULONG id)
{
    ULONG expect;

    (VOID)id;

    tx_semaphore_get(&sem_mutex_done, TX_WAIT_FOREVER);
    tx_semaphore_get(&sem_mutex_done, TX_WAIT_FOREVER);
    if (mutex_count == (MUTEX_LOOPS * 2UL))
    {
        printf("V5F MUTEX PASS count=%lu\r\n", mutex_count);
    }
    else
    {
        printf("V5F MUTEX FAIL count=%lu\r\n", mutex_count);
    }

    tx_semaphore_get(&sem_q_done, TX_WAIT_FOREVER);
    expect = (QUEUE_COUNT * (QUEUE_COUNT + 1U)) / 2U;
    if (queue_sum == expect)
    {
        printf("V5F QUEUE PASS sum=%lu\r\n", queue_sum);
    }
    else
    {
        printf("V5F QUEUE FAIL sum=%lu expect=%lu\r\n", queue_sum, expect);
    }

    tx_thread_sleep(60);
    if (timer_hits >= 5U)
    {
        printf("V5F TIMER PASS hits=%lu\r\n", timer_hits);
    }
    else
    {
        printf("V5F TIMER FAIL hits=%lu\r\n", timer_hits);
    }

    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

VOID tx_demo_define(VOID)
{
    tx_mutex_create(&mutex, "mutex", TX_NO_INHERIT);
    tx_semaphore_create(&sem_mutex_done, "mdone", 0);
    tx_semaphore_create(&sem_tim7, "tim7", 0);
    tx_semaphore_create(&sem_q_done, "qdone", 0);
    tx_queue_create(&queue, "q", TX_1_ULONG, queue_storage, sizeof(queue_storage));
    tx_event_flags_create(&events, "evt");
    tx_timer_create(&app_timer, "tapp", app_timer_cb, 0,
                    10, 10, TX_AUTO_ACTIVATE);

    tx_thread_create(&thread_tim7, "tim7", tim7_wait_entry, 0,
                     stack_tim7, DEMO_STACK_SIZE,
                     5, 5, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_mutex_a, "ma", mutex_worker, 0,
                     stack_mutex_a, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_mutex_b, "mb", mutex_worker, 1,
                     stack_mutex_b, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_q_prod, "qp", queue_prod, 0,
                     stack_q_prod, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_q_cons, "qc", queue_cons, 0,
                     stack_q_cons, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_evt_get, "eg", evt_get_entry, 0,
                     stack_evt_get, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_evt_set, "es", evt_set_entry, 0,
                     stack_evt_set, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_obj, "obj", object_entry, 0,
                     stack_obj, DEMO_STACK_SIZE,
                     11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&thread_rr_watch, "rrw", rr_watch_entry, 0,
                     stack_rr_watch, DEMO_STACK_SIZE,
                     13, 13, TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_rr_a, "rra", rr_a_entry, 0,
                     stack_rr_a, DEMO_STACK_SIZE,
                     14, 14, RR_SLICE, TX_AUTO_START);
    tx_thread_create(&thread_rr_b, "rrb", rr_b_entry, 0,
                     stack_rr_b, DEMO_STACK_SIZE,
                     14, 14, RR_SLICE, TX_AUTO_START);

    tim7_oneshot_start();
}
