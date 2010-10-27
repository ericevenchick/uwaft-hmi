#ifndef OSIF_KERNEL_H_
#define OSIF_KERNEL_H_

/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

//###############################################################

#include <linux/types.h>
#include <linux/tty.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/tqueue.h>
#include <pcmcia/driver_ops.h>
#include <pcmcia/bus_ops.h>

// DEFINES
#define OS_IF_TIMEOUT 0
#define OS_IF_TICK_COUNT jiffies
#define OS_IF_MOD_INC_USE_COUNT MOD_INC_USE_COUNT;
#define OS_IF_MOD_DEC_USE_COUNT MOD_DEC_USE_COUNT;
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)

// TYPEDEFS
typedef struct timeval        OS_IF_TIME_VAL;
typedef off_t                 OS_IF_OFFSET;
typedef unsigned long         OS_IF_SIZE;
typedef int                   OS_IF_THREAD;
typedef wait_queue_head_t     OS_IF_WAITQUEUE_HEAD;
typedef wait_queue_t          OS_IF_WAITQUEUE;
typedef spinlock_t            OS_IF_LOCK;

typedef struct tq_struct      OS_IF_TASK_QUEUE_HANDLE;
typedef int                   OS_IF_WQUEUE;
typedef struct semaphore      OS_IF_SEMAPHORE;

#endif //OSIF_KERNEL_H_
