/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

/* Kvaser CAN driver PCIcan hardware specific parts                    
** PCIcan definitions                                                    
*/

#ifndef _PCICAN_HW_IF_H_
#define _PCICAN_HW_IF_H_

#include <linux/list.h>
#include <linux/types.h>
#include <asm/atomic.h>

#include "VCanOsIf.h"
#include "osif_kernel.h"
#include "helios_cmds.h"
#include "helios_dpram.h"

/*****************************************************************************/
/* defines */
/*****************************************************************************/

#define DEVICE_NAME_STRING "pcicanII"

#define MAX_CHANNELS 2
#define PCICAN_MAX_DEV 16

#define PCICANII_VENDOR 0x10e8  // AMCC.
#define PCICANII_ID 0x8407      // Allocated to Kvaser by AMCC

#define KVASER_VENDOR 0x1a07    // KVASER
#define PC104PLUS_ID 0x6        // KVASER PC104+
#define PCICANXII_ID 0x7        // KVASER PCIcanx II
#define PCI104_ID    0x9        // KVASER PCI104

// Standard value: Pushpull  (OCTP1|OCTN1|OCTP0|OCTN0|OCM1)
#define OCR_DEFAULT_STD 0xDA
// For Galathea piggyback.
#define OCR_DEFAULT_GAL 0xDB

#define MAX_ERROR_COUNT 128
#define ERROR_RATE 30000
#define PCICAN_BYTES_PER_CIRCUIT 0x20


/*****************************************************************************/
/* Xilinx                                                                    */
/*****************************************************************************/

//
// These register values are valid for revision 14 of the Xilinx logic.
//
#define XILINX_OUTA         0   // Bit 7 used to write bits to serial memory DS2430.
#define XILINX_INA          1   // Bit 7 used to read back bits from serial memory DS2430.
#define XILINX_CTRLA        2   // Sets the function of the Xilinx pins normally set to zero.
#define XILINX_UNUSED       3
#define XILINX_OUTB         4   // Bit 7 used to reset the SJA-1000.
#define XILINX_INB          5   // Bit 7 used to read back the reset line value.
#define XILINX_CTRLB        6   // Sets the function of the Xilinx pins normally set to zero.
#define XILINX_VERINT       7   // Lower nibble simulate interrupts, high nibble version number.

#define XILINX_PRESUMED_VERSION     14

#define HELIOS_MAX_OUTSTANDING_TX   16
#define PCICANII_TICKS_PER_10US       1
#define PCICANII_CMD_RESP_WAIT_TIME 200

/* Channel specific data */
typedef struct PciCanIIChanData
{
    CAN_MSG current_tx_message[HELIOS_MAX_OUTSTANDING_TX];
    //unsigned int outstanding_tx;
    atomic_t outstanding_tx;
    int channel;
    OS_IF_TASK_QUEUE_HANDLE txTaskQ;

    unsigned long freq;
    unsigned char sjw;
    unsigned char tseg1;
    unsigned char tseg2;
    unsigned char samples;
    VCanChanData  *vChan;
} PciCanIIChanData;

/*  Cards specific data */
typedef struct PciCanIICardData {   
    //unsigned char        ean_code [16];
    unsigned int         hardware_revision_major;
    unsigned int         hardware_revision_minor;
    unsigned int         card_flags;
    int                  initDone;  // all init is done
    int                  isWaiting; // wait for interrupt
    int                  receivedHwInfo;
    int                  receivedSwInfo;
    int                  waitForChipState; // wait for chip state event
    void __iomem         *baseAddr;
    int                  irq;
    OS_IF_WAITQUEUE_HEAD waitHwInfo;
    OS_IF_WAITQUEUE_HEAD waitSwInfo;
    OS_IF_WAITQUEUE_HEAD waitResponse;
    OS_IF_WAITQUEUE_HEAD waitClockResp;
    OS_IF_LOCK           timeHi_lock;
    OS_IF_LOCK           memQLock;
    unsigned long        recClock;
    struct list_head     replyWaitList;
    rwlock_t             replyWaitListLock;
} PciCanIICardData;

#endif  /* _PCICAN_HW_IF_H_ */
