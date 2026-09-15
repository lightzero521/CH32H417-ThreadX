/**************************************************************************/
/*                                                                        */
/*    tx_port.h                            CH32H417 QingKe V5F / GNU      */
/*                                                                        */
/*  ThreadX port header. Based on Eclipse ThreadX RISC-V32 common port,   */
/*  with QingKe V5F interrupt and timer adaptations.                      */
/*                                                                        */
/**************************************************************************/

#ifndef TX_PORT_H
#define TX_PORT_H

#ifndef __ASSEMBLER__
#include <string.h>
#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif
#endif

#include "tx_qke_v5f.h"

#define VOID                                    void

#ifndef __ASSEMBLER__
typedef char                                    CHAR;
typedef unsigned char                           UCHAR;
typedef int                                     INT;
typedef unsigned int                            UINT;
typedef long                                    LONG;
typedef unsigned long                           ULONG;
typedef unsigned long long                      ULONG64;
typedef short                                   SHORT;
typedef unsigned short                          USHORT;
#define ULONG64_DEFINED
#endif

#ifndef TX_MAX_PRIORITIES
#define TX_MAX_PRIORITIES                       32
#endif

#ifndef TX_MINIMUM_STACK
#define TX_MINIMUM_STACK                        1024
#endif

#ifndef TX_TIMER_THREAD_STACK_SIZE
#define TX_TIMER_THREAD_STACK_SIZE              1024
#endif

#ifndef TX_TIMER_THREAD_PRIORITY
#define TX_TIMER_THREAD_PRIORITY                0
#endif

#define TX_INT_DISABLE                          0x00000000
#define TX_INT_ENABLE                           GINTENR_IE

#ifndef TX_TRACE_TIME_SOURCE
#define TX_TRACE_TIME_SOURCE                    ++_tx_trace_simulated_time
#endif
#ifndef TX_TRACE_TIME_MASK
#define TX_TRACE_TIME_MASK                      0xFFFFFFFFUL
#endif

#define TX_PORT_SPECIFIC_BUILD_OPTIONS          0
#define TX_INLINE_INITIALIZATION

#ifdef TX_ENABLE_STACK_CHECKING
#undef TX_DISABLE_STACK_FILLING
#endif

#define TX_THREAD_EXTENSION_0
#define TX_THREAD_EXTENSION_1
#define TX_THREAD_EXTENSION_2
#define TX_THREAD_EXTENSION_3

#define TX_BLOCK_POOL_EXTENSION
#define TX_BYTE_POOL_EXTENSION
#define TX_EVENT_FLAGS_GROUP_EXTENSION
#define TX_MUTEX_EXTENSION
#define TX_QUEUE_EXTENSION
#define TX_SEMAPHORE_EXTENSION
#define TX_TIMER_EXTENSION

#ifndef TX_THREAD_USER_EXTENSION
#define TX_THREAD_USER_EXTENSION
#endif

#define TX_THREAD_CREATE_EXTENSION(thread_ptr)
#define TX_THREAD_DELETE_EXTENSION(thread_ptr)
#define TX_THREAD_COMPLETED_EXTENSION(thread_ptr)
#define TX_THREAD_TERMINATED_EXTENSION(thread_ptr)

#define TX_BLOCK_POOL_CREATE_EXTENSION(pool_ptr)
#define TX_BYTE_POOL_CREATE_EXTENSION(pool_ptr)
#define TX_EVENT_FLAGS_GROUP_CREATE_EXTENSION(group_ptr)
#define TX_MUTEX_CREATE_EXTENSION(mutex_ptr)
#define TX_QUEUE_CREATE_EXTENSION(queue_ptr)
#define TX_SEMAPHORE_CREATE_EXTENSION(semaphore_ptr)
#define TX_TIMER_CREATE_EXTENSION(timer_ptr)

#define TX_BLOCK_POOL_DELETE_EXTENSION(pool_ptr)
#define TX_BYTE_POOL_DELETE_EXTENSION(pool_ptr)
#define TX_EVENT_FLAGS_GROUP_DELETE_EXTENSION(group_ptr)
#define TX_MUTEX_DELETE_EXTENSION(mutex_ptr)
#define TX_QUEUE_DELETE_EXTENSION(queue_ptr)
#define TX_SEMAPHORE_DELETE_EXTENSION(semaphore_ptr)
#define TX_TIMER_DELETE_EXTENSION(timer_ptr)

#ifndef __ASSEMBLER__
UINT  _tx_thread_interrupt_control(UINT new_posture);
VOID  _tx_v5f_systick_start(VOID);
VOID  _tx_v5f_wfi(VOID);
ULONG _tx_v5f_irq_active(VOID);
extern ULONG _tx_v5f_in_isr;
#endif

/* Like Cortex-M IPSR: a C peripheral ISR is still "in the kernel"
   even when _tx_thread_system_state is 0. */
#ifndef TX_THREAD_GET_SYSTEM_STATE
#define TX_THREAD_GET_SYSTEM_STATE() \
    (_tx_thread_system_state | _tx_v5f_irq_active())
#endif

/* Cortex-M port: only preempt-disable gates _tx_thread_system_return.
   From a C ISR that return just pends Software_IRQn (PendSV). */
#ifndef TX_THREAD_SYSTEM_RETURN_CHECK
#define TX_THREAD_SYSTEM_RETURN_CHECK(c) \
    (c) = ((ULONG) _tx_thread_preempt_disable);
#endif

/* Hold off solicited switches until _tx_thread_schedule starts. */
#define TX_PORT_SPECIFIC_POST_INITIALIZATION            _tx_thread_preempt_disable++;
#define TX_PORT_SPECIFIC_PRE_SCHEDULER_INITIALIZATION   _tx_v5f_systick_start();

#define TX_INTERRUPT_SAVE_AREA                  register UINT interrupt_save;

#define TX_DISABLE                              interrupt_save = _tx_thread_interrupt_control(TX_INT_DISABLE);
#define TX_RESTORE                              _tx_thread_interrupt_control(interrupt_save);

#define TX_BLOCK_POOL_DISABLE                   TX_DISABLE
#define TX_BYTE_POOL_DISABLE                    TX_DISABLE
#define TX_EVENT_FLAGS_GROUP_DISABLE            TX_DISABLE
#define TX_MUTEX_DISABLE                        TX_DISABLE
#define TX_QUEUE_DISABLE                        TX_DISABLE
#define TX_SEMAPHORE_DISABLE                    TX_DISABLE
#define TX_TIMER_DISABLE                        TX_DISABLE

#ifndef __ASSEMBLER__
#ifdef TX_THREAD_INIT
CHAR  _tx_version_id[] =
    "Copyright (c) 2026 Eclipse ThreadX contributors. * ThreadX RISC-V32/CH32H417-V5F Version 6.5.1 *";
#else
extern CHAR  _tx_version_id[];
#endif
#endif

#endif
