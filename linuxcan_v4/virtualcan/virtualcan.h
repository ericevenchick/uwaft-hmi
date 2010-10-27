/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

/* Kvaser CAN driver virtual hardware specific parts                    
** virtual definitions                                                    
*/

#ifndef _VIRTUAL_HW_IF_H_
#define _VIRTUAL_HW_IF_H_

#include "osif_kernel.h"

/*****************************************************************************/
/* defines */
/*****************************************************************************/

#define DEVICE_NAME_STRING   "kvvirtualcan"

#define NR_CHANNELS          2
#define MAX_CHANNELS         NR_CHANNELS

#define NR_VIRTUAL_DEV       1
#define VIRTUAL_MAX_DEV      NR_VIRTUAL_DEV

#define MAX_ERROR_COUNT        128
#define ERROR_RATE           30000
#define VIRTUAL_BYTES_PER_CIRCUIT 0x20

#define VIRTUAL_MAX_OUTSTANDING 300




/* Channel specific data */
typedef struct virtualChanData
{
    VCanBusParams busparams;
    OS_IF_TASK_QUEUE_HANDLE txTaskQ;
    int silentmode;
    atomic_t outstanding_tx;
} virtualChanData;

/*  Cards specific data */
typedef struct virtualCardData {
    /* Ports and addresses */
    unsigned           pciIf;
} virtualCardData;

#endif  /* _VIRTUAL_HW_IF_H_ */
