/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

//--------------------------------------------------
// NOTE! module_versioning HAVE to be included first
#include "module_versioning.h"
//--------------------------------------------------

//
// Kvaser CAN driver PCIcan hardware specific parts
// PCIcan functions
//

#include <linux/version.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#if LINUX_2_6
#  include <linux/workqueue.h>
#else
#  include <linux/tqueue.h>
#endif
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/proc_fs.h>

#include <asm/io.h>

#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>


// Kvaser definitions
#include "helios_cmds.h"
#include "VCanOsIf.h"
#include "PciCanIIHwIf.h"
#include "osif_kernel.h"
#include "osif_functions_kernel.h"
#include "queue.h"
#include "memq.h"
#include "hwnames.h"
#include "vcan_ioctl.h"


MODULE_DESCRIPTION("PCIcanII CAN module.");

//
// If you do not define PCICANII_DEBUG at all, all the debug code will be
// left out.  If you compile with PCICAN_DEBUG=0, the debug code will
// be present but disabled -- but it can then be enabled for specific
// modules at load time with a 'debug_level=#' option to insmod.
// i.e. >insmod kvpcicanII debug_level=#
//

#ifdef PCICANII_DEBUG
static int debug_level = PCICANII_DEBUG;
#   if !LINUX_2_6
        MODULE_PARM(debug_level, "i");
#   else
        MODULE_PARM_DESC(debug_level, "PCIcanII debug level");
        module_param(debug_level, int, 0644);
#   endif
#   define DEBUGPRINT(n, args...) if (debug_level>=(n)) printk("<" #n ">" args)
#else
#   define DEBUGPRINT(n, args...) if ((n) == 1) printk("<" #n ">" args)
#endif

// Bits in the CxSTRH register in the M16C.
#define M16C_BUS_RESET    0x01    // Chip is in Reset state
#define M16C_BUS_ERROR    0x10    // Chip has seen a bus error
#define M16C_BUS_PASSIVE  0x20    // Chip is error passive
#define M16C_BUS_OFF      0x40    // Chip is bus off


//======================================================================
// HW function pointers
//======================================================================

static int INIT pciCanInitAllDevices(void);
static int pciCanSetBusParams (VCanChanData *vChd, VCanBusParams *par);
static int pciCanGetBusParams (VCanChanData *vChd, VCanBusParams *par);
static int pciCanSetOutputMode (VCanChanData *vChd, int silent);
static int pciCanSetTranceiverMode (VCanChanData *vChd, int linemode, int resnet);
static int pciCanBusOn (VCanChanData *vChd);
static int pciCanBusOff (VCanChanData *vChd);
static int pciCanGetTxErr(VCanChanData *vChd);
static int pciCanGetRxErr(VCanChanData *vChd);
static int pciCanInSync (VCanChanData *vChd);
static int EXIT pciCanCloseAllDevices(void);
static int pciCanProcRead (char *buf, char **start, off_t offset,
                           int count, int *eof, void *data);
static int pciCanRequestChipState (VCanChanData *vChd);
static unsigned long pciCanRxQLen(VCanChanData *vChd);
static unsigned long pciCanTxQLen(VCanChanData *vChd); 
static int pciCanRequestSend (VCanCardData *vCard, VCanChanData *vChan);
static unsigned long pciCanTime(VCanCardData *vCard); // new
static int pciCanFlushSendBuffer (VCanChanData *vChan); // ne

static int pciCanWaitResponse(VCanCardData *vCard, heliosCmd *cmd,
                              heliosCmd *replyPtr, unsigned char cmdNr,
                              unsigned char transId);
static int pciCanNoResponse(PciCanIICardData *hCard, heliosCmd *cmd);


#if LINUX
#  if LINUX_2_6
VCanHWInterface hwIf = {
    .initAllDevices   =  pciCanInitAllDevices,
    .setBusParams     =  pciCanSetBusParams,
    .getBusParams     =  pciCanGetBusParams,
    .setOutputMode    =  pciCanSetOutputMode,
    .setTranceiverMode=  pciCanSetTranceiverMode,
    .busOn            =  pciCanBusOn,
    .busOff           =  pciCanBusOff,
    .txAvailable      =  pciCanInSync,
    .procRead         =  pciCanProcRead,
    .closeAllDevices  =  pciCanCloseAllDevices,
    .getTime          =  pciCanTime,
    .flushSendBuffer  =  pciCanFlushSendBuffer,
    .getTxErr         =  pciCanGetTxErr,
    .getRxErr         =  pciCanGetRxErr,
    .rxQLen           =  pciCanRxQLen,
    .txQLen           =  pciCanTxQLen,
    .requestChipState =  pciCanRequestChipState,
    .requestSend      =  pciCanRequestSend
};
#  else
VCanHWInterface hwIf = {
     initAllDevices:     pciCanInitAllDevices,
     setBusParams:       pciCanSetBusParams,
     getBusParams:       pciCanGetBusParams,
     setOutputMode:      pciCanSetOutputMode,
     setTranceiverMode:  pciCanSetTranceiverMode,
     busOn:              pciCanBusOn,
     busOff:             pciCanBusOff,
     txAvailable:        pciCanInSync,
     procRead:           pciCanProcRead,
     closeAllDevices:    pciCanCloseAllDevices,
     getTime:            pciCanTime,
     flushSendBuffer:    pciCanFlushSendBuffer,
     getTxErr:           pciCanGetTxErr,
     getRxErr:           pciCanGetRxErr,
     rxQLen:             pciCanRxQLen,
     txQLen:             pciCanTxQLen,
     requestChipState:   pciCanRequestChipState,
     requestSend:        pciCanRequestSend
};
#  endif
#endif


static int DEVINIT pciCanInitOne(struct pci_dev *dev,
                                 const struct pci_device_id *id);
static void DEVEXIT pciCanRemoveOne(struct pci_dev *dev);

static struct pci_device_id id_table[] DEVINITDATA = {
  {
    .vendor    = PCICANII_VENDOR,
    .device    = PCICANII_ID,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID
  },
  {
    .vendor    = KVASER_VENDOR,
    .device    = PC104PLUS_ID,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID
  },
  {
    .vendor    = KVASER_VENDOR,
    .device    = PCI104_ID,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID
  },
  {
    .vendor    = KVASER_VENDOR,
    .device    = PCICANXII_ID,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID
  },
  {0,},
};

static struct pci_driver pcican_tbl = {
  .name     = "kvpcicanII",
  .id_table = id_table,
  .probe    = pciCanInitOne,
  .remove   = DEVEXITP(pciCanRemoveOne),
};

//MODULE_DEVICE_TABLE(pci, pcican_tbl);


//======================================================================
// Find out length of helios command
//======================================================================
static OS_IF_INLINE int getCmdLen (heliosCmd *cmd)
{
    return cmd->head.cmdLen;
}


//======================================================================
// Copy a helios command
//======================================================================
static OS_IF_INLINE void copyCmd (heliosCmd *cmd_to, heliosCmd *cmd_from)
{
    int cmdLen;
    unsigned char *from, *to;

    from = (unsigned char *)cmd_from;
    to   = (unsigned char *)cmd_to;

    for (cmdLen = getCmdLen(cmd_from); cmdLen > 0; cmdLen--) {
        *to++ = *from++;
    }

    return;
}


//======================================================================
//    getTransId
//======================================================================
static OS_IF_INLINE int getTransId (heliosCmd *cmd)
{
    if (cmd->head.cmdNo > CMD_TX_EXT_MESSAGE) {
        // Any of the commands
        return cmd->getBusparamsReq.transId;
    }
    else {
        DEBUGPRINT(1, "WARNING won't give a correct transid\n");
        return 0;
    }
}


//======================================================================
// /proc read function
//======================================================================
static int pciCanProcRead (char *buf, char **start, off_t offset,
                           int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(buf + len, "\ntotal channels %d\n", driverData.noOfDevices);
    *eof = 1;

    return len;
}


//======================================================================
//  All acks received?
//======================================================================
static int pciCanInSync (VCanChanData *vChd)
{
    PciCanIIChanData *hChd = vChd->hwChanData;

    return (atomic_read(&hChd->outstanding_tx) == 0);
} // pciCanInSync


//======================================================================
//  Can we send now?
//======================================================================
static int pciCanTxAvailable (VCanChanData *vChd)
{
    PciCanIIChanData *hChd = vChd->hwChanData;

    return (atomic_read(&hChd->outstanding_tx) < HELIOS_MAX_OUTSTANDING_TX);
} // pciCanTxAvailable


//======================================================================
// Find out some info about the H/W
//======================================================================
static int pciCanProbe (VCanCardData *vCd)
{
    PciCanIICardData *hCd = vCd->hwCardData;
    int chan, i;

    if (hCd->baseAddr == 0) {
        DEBUGPRINT(1, "pcicanProbe card_present = 0\n");
        vCd->cardPresent = 0;
        return VCAN_STAT_NO_DEVICE;
    }

    chan = 0;
    for (i = 0; i < MAX_CHANNELS; i++) {
        VCanChanData *vChd = vCd->chanData[i];

        if (vChd != NULL) {
            vChd->channel = i;
            chan++;
        }
    }

    if (chan == 0) {
        vCd->cardPresent = 0;
        return VCAN_STAT_NO_DEVICE;
    }
    vCd->nrChannels = chan;
    vCd->cardPresent = 1;

    return VCAN_STAT_OK;
} // pciCanProbe


//======================================================================
// Enable bus error interrupts, and reset the
// counters which keep track of the error rate
//======================================================================
#if 0
static void pciCanResetErrorCounter (VCanChanData *vChd)
{
    PciCanIICardData *hCard = vChd->vCard->hwCardData;
    heliosCmd cmd;

    cmd.head.cmdNo                = CMD_RESET_ERROR_COUNTER;
    cmd.resetErrorCounter.cmdLen  = sizeof(cmdResetErrorCounter);
    cmd.resetErrorCounter.channel = (unsigned char)vChd->channel;

    if (pciCanNoResponse(hCard, &cmd) != VCAN_STAT_OK) {
        DEBUGPRINT(1, "ERROR----- pciCanResetErrorCounter----------\n");
    }

    vChd->errorCount = 0;
    vChd->errorTime = hwIf.getTime(vChd->vCard); // 10us?
} // pciCanResetErrorCounter
#endif


//======================================================================
//  Set bit timing
//======================================================================
static int pciCanSetBusParams (VCanChanData *vChd, VCanBusParams *par)
{

    PciCanIICardData *hCard = vChd->vCard->hwCardData;
    PciCanIIChanData *hChd  = vChd->hwChanData;
    heliosCmd cmd;
    unsigned long tmp;
    int ret;

    cmd.setBusparamsReq.cmdNo   = CMD_SET_BUSPARAMS_REQ;
    cmd.setBusparamsReq.cmdLen  = sizeof(cmdSetBusparamsReq);
    cmd.setBusparamsReq.bitRate = (unsigned long)par->freq;
    cmd.setBusparamsReq.sjw     = (unsigned char)par->sjw;
    cmd.setBusparamsReq.tseg1   = (unsigned char)par->tseg1;
    cmd.setBusparamsReq.tseg2   = (unsigned char)par->tseg2;
    cmd.setBusparamsReq.channel = (unsigned char)vChd->channel;
    cmd.setBusparamsReq.transId = (unsigned char)vChd->channel;
    cmd.setBusparamsReq.noSamp  = 1; // Always 1

    // Check bus parameters
    tmp = par->freq * (par->tseg1 + par->tseg2 + 1);
    if (tmp == 0) {
        return VCAN_STAT_BAD_PARAMETER;
    }
    if ((8000000 / tmp) > 16) {
        return VCAN_STAT_BAD_PARAMETER;
    }

    // Store locally since getBusParams not correct
    hChd->freq    = (unsigned long)par->freq;
    hChd->sjw     = (unsigned char)par->sjw;
    hChd->tseg1   = (unsigned char)par->tseg1;
    hChd->tseg2   = (unsigned char)par->tseg2;
    hChd->samples = 1; // Always 1

   // DEBUGPRINT(1, "pciCanSetBusParams: chan = %d,
      //freq = %d, sjw = %d, tseg1 = %d, tseg2 = %d, samples = %d (always 1)\n"
      //,vChd->channel, par->freq, par->sjw, par->tseg1, par->tseg2, 1);

    ret = pciCanNoResponse(hCard, &cmd);
    if (ret != VCAN_STAT_OK) {
        DEBUGPRINT(1, "ERROR----- pciCanSetBusParams----------\n");
    }

    return ret;
} // pciCanSetBusParams


//======================================================================
//  Get bit timing
//======================================================================
static int pciCanGetBusParams (VCanChanData *vChd, VCanBusParams *par)
{
    // Do not send cmdGetBusparamsReq command
    // possible bug in firmware

    PciCanIIChanData *hChd  = vChd->hwChanData;
    par->sjw     = hChd->sjw;
    par->samp3   = hChd->samples; // Always 1
    par->tseg1   = hChd->tseg1;
    par->tseg2   = hChd->tseg2;
    par->freq    = hChd->freq;

    return VCAN_STAT_OK;
} // pciCanGetBusParams


//======================================================================
//  Set silent or normal mode
//======================================================================
static int pciCanSetOutputMode (VCanChanData *vChd, int silent)
{
    PciCanIICardData *hCard = vChd->vCard->hwCardData;
    heliosCmd cmd;
    int ret;

    cmd.setDrivermodeReq.cmdNo    = CMD_SET_DRIVERMODE_REQ;
    cmd.setDrivermodeReq.cmdLen   = sizeof(cmdSetDrivermodeReq);
    cmd.setDrivermodeReq.channel  = (unsigned char)vChd->channel;
    cmd.setDrivermodeReq.driverMode = silent? DRIVERMODE_SILENT : DRIVERMODE_NORMAL;

    ret = pciCanNoResponse(hCard, &cmd);
    if (ret != VCAN_STAT_OK) {
        DEBUGPRINT(1, "ERROR----- pciCanSetOutputMode----------\n");
    }

    return ret;
} // pciCanSetOutputMode


//======================================================================
//  Line mode
//======================================================================
static int pciCanSetTranceiverMode (VCanChanData *vChd, int linemode, int resnet)
{
    vChd->lineMode = linemode;
    // qqq
    return VCAN_STAT_OK;
} // pciCanSetTranceiverMode


//======================================================================
//  Query chip status
//======================================================================
static int pciCanRequestChipState (VCanChanData *vChd)
{
    heliosCmd cmd;
    heliosCmd resp;
    int ret;

    cmd.head.cmdNo              = CMD_GET_CHIP_STATE_REQ;
    cmd.getChipStateReq.cmdLen  = sizeof(cmdGetChipStateReq);
    cmd.getChipStateReq.channel = (unsigned char)vChd->channel;
    cmd.getChipStateReq.transId = (unsigned char)vChd->channel;

    ret = pciCanWaitResponse(vChd->vCard, (heliosCmd *)&cmd,
                             (heliosCmd *)&resp, CMD_CHIP_STATE_EVENT,
                             cmd.getChipStateReq.transId);

    return ret;
} // pciCanRequestChipState


//======================================================================
//  Go bus on
//  qqq Shouldn't this be atomic?
//======================================================================
static int pciCanBusOn (VCanChanData *vChd)
{
    heliosCmd cmd;
    heliosCmd resp;
    int ret = 0;
    PciCanIIChanData *hChd = vChd->hwChanData;

    cmd.head.cmdNo            = CMD_START_CHIP_REQ;
    cmd.startChipReq.cmdLen   = sizeof(cmdStartChipReq);
    cmd.startChipReq.channel  = (unsigned char)vChd->channel;
    cmd.startChipReq.transId  = (unsigned char)vChd->channel;

    ret = pciCanWaitResponse(vChd->vCard, (heliosCmd *)&cmd, (heliosCmd *)&resp,
                             CMD_START_CHIP_RESP, cmd.startChipReq.transId);
    if (ret == VCAN_STAT_OK) {
        atomic_set(&hChd->outstanding_tx, 0);
#if 0
        atomic_set(&vChd->transId, 1);
#endif
        memset(hChd->current_tx_message, 0, sizeof(hChd->current_tx_message));

        vChd->overrun = 0;         // qqq Overrun not used
        vChd->isOnBus = 1;

        pciCanRequestChipState(vChd);
    }

    return ret;
} // pciCanBusOn


//======================================================================
//  Go bus off
//  qqq Shouldn't this be atomic?
//======================================================================
static int pciCanBusOff (VCanChanData *vChd)
{
    PciCanIIChanData *hChd  = vChd->hwChanData;
    heliosCmd cmd;
    heliosCmd resp;
    int ret;

    cmd.head.cmdNo          = CMD_STOP_CHIP_REQ;
    cmd.stopChipReq.cmdLen  = sizeof(cmdStopChipReq);
    cmd.stopChipReq.channel = (unsigned char)vChd->channel;
    cmd.stopChipReq.transId = (unsigned char)vChd->channel;

    ret = pciCanWaitResponse(vChd->vCard, (heliosCmd *)&cmd, (heliosCmd *)&resp,
                             CMD_STOP_CHIP_RESP, cmd.stopChipReq.transId);
    if (ret == VCAN_STAT_OK) {
        atomic_set(&hChd->outstanding_tx, 0);
#if 0
        atomic_set(&vChd->transId, 1);
#endif
        memset(hChd->current_tx_message, 0, sizeof(hChd->current_tx_message));

        vChd->isOnBus = 0;
        vChd->chipState.state = CHIPSTAT_BUSOFF;
        vChd->overrun = 0;         // qqq overrun not used

        pciCanRequestChipState(vChd);
    }

    return ret;
} // pciCanBusOff


//======================================================================
//  Enable/disable interrupts on card
//======================================================================
static void pciCanInterrupts (VCanCardData *vCard, int enable)
{
    PciCanIICardData *hCard = vCard->hwCardData;
    unsigned long tmp;
    void __iomem  *addr;

    addr = hCard->baseAddr;
    tmp = ioread32(addr + DPRAM_INTERRUPT_REG);

    if (enable) {
        tmp &= ~(DPRAM_INTERRUPT_DISABLE | DPRAM_INTERRUPT_ACK);
    } else { 
        tmp |= DPRAM_INTERRUPT_DISABLE;
    }
    iowrite32(tmp, addr + DPRAM_INTERRUPT_REG);
}


//======================================================================
// Get time
//======================================================================
static unsigned long pciCanTime (VCanCardData *vCard)
{
    PciCanIICardData *hCard = vCard->hwCardData;
    heliosCmd cmd;
    heliosCmd resp;
    int ret;

    memset(&cmd, 0, sizeof(cmd));
    cmd.readClockReq.cmdNo      = CMD_READ_CLOCK_REQ;
    cmd.readClockReq.cmdLen     = sizeof(cmdReadClockReq);
    cmd.readClockReq.flags      = 0;
    cmd.readClockReq.transId    = 0;

    ret = pciCanWaitResponse(vCard, (heliosCmd *)&cmd, (heliosCmd *)&resp,
                             CMD_READ_CLOCK_RESP, cmd.readClockReq.transId);
  // qqq Unable to distinguish error from time!
    if (ret) {
        return ret;
    }

    return hCard->recClock / PCICANII_TICKS_PER_10US;
}


//======================================================================
// get timestamp
//======================================================================
static unsigned long pciCanTimeStamp (VCanCardData *vCard, unsigned long timeLo)
{
    unsigned long    ret;
    PciCanIICardData *hCd = vCard->hwCardData;
    unsigned long    irqFlags;

    os_if_spin_lock_irqsave(&hCd->timeHi_lock, &irqFlags);
    ret = (vCard->timeHi + timeLo) / PCICANII_TICKS_PER_10US;
    os_if_spin_unlock_irqrestore(&hCd->timeHi_lock, irqFlags);

    return ret;
}


//======================================================================
//  Interrupt handling functions
//======================================================================
static void pciCanReceiveIsr (VCanCardData *vCard)
{
    VCAN_EVENT e;
    PciCanIICardData  *hCd     = vCard->hwCardData;
    unsigned int      loopMax  = 1000;
    heliosCmd         cmd;


    // Reading clears interrupt flag

    while (1) {
        if (GetCmdFromQ(hCd, &cmd) != MEM_Q_SUCCESS) {
          break;
        }

        // A loop counter as a safety measure.
        if (--loopMax == 0) {
          DEBUGPRINT(1, "pciCanReceiverIsr: Loop counter as a safety measure!!!\n");
          return;
        }

        switch (cmd.head.cmdNo) {

            case CMD_RX_STD_MESSAGE:
            {
                char dlc;
                unsigned char flags;
                unsigned int chan = cmd.rxCanMessage.channel;


                if (chan < (unsigned)vCard->nrChannels) {
                    VCanChanData *vChd = vCard->chanData[cmd.rxCanMessage.channel];
                    e.tag               = V_RECEIVE_MSG;
                    e.transId           = 0;
                    e.timeStamp         = pciCanTimeStamp(vCard, cmd.rxCanMessage.time);
                    e.tagData.msg.id    = (cmd.rxCanMessage.rawMessage[0] & 0x1F) << 6;
                    e.tagData.msg.id   += (cmd.rxCanMessage.rawMessage[1] & 0x3F);
                    flags = cmd.rxCanMessage.flags;
                    e.tagData.msg.flags = 0;
                    if (flags & MSGFLAG_OVERRUN)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_OVERRUN;
                    if (flags & MSGFLAG_REMOTE_FRAME)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_REMOTE_FRAME;
                    if (flags & MSGFLAG_ERROR_FRAME)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_ERROR_FRAME;
                    if (flags & MSGFLAG_TX)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_TXACK;
                    if (flags & MSGFLAG_TXRQ)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_TXRQ;

                    dlc = cmd.rxCanMessage.rawMessage[5] & 0x0F;
                    e.tagData.msg.dlc = dlc;
                    memcpy(e.tagData.msg.data, &cmd.rxCanMessage.rawMessage[6], 8);

                    vCanDispatchEvent(vChd, &e);
                } else {
                    DEBUGPRINT(1, "QQQ: CMD_RX_STD_MESSAGE, dlc = %d flags = %x, chan = %d\n",
                               cmd.rxCanMessage.rawMessage[5] & 0x0F,
                               cmd.rxCanMessage.flags,chan);
                }
                break;
            }

            case CMD_RX_EXT_MESSAGE:
            {
                char dlc;
                unsigned char flags;
                unsigned int chan = cmd.rxCanMessage.channel;

                if (chan < (unsigned)vCard->nrChannels) {
                    VCanChanData *vChd  = vCard->chanData[cmd.rxCanMessage.channel];
                    e.tag               = V_RECEIVE_MSG;
                    e.transId           = 0;
                    e.timeStamp         = pciCanTimeStamp(vCard, cmd.rxCanMessage.time);
                    e.tagData.msg.id    = (cmd.rxCanMessage.rawMessage[0] & 0x1F) << 24;
                    e.tagData.msg.id   += (cmd.rxCanMessage.rawMessage[1] & 0x3F) << 18;
                    e.tagData.msg.id   += (cmd.rxCanMessage.rawMessage[2] & 0x0F) << 14;
                    e.tagData.msg.id   += (cmd.rxCanMessage.rawMessage[3] & 0xFF) <<  6;
                    e.tagData.msg.id   += (cmd.rxCanMessage.rawMessage[4] & 0x3F);
                    e.tagData.msg.id   += EXT_MSG;
                    flags = cmd.rxCanMessage.flags;
                    e.tagData.msg.flags = 0;
                    if (flags & MSGFLAG_OVERRUN)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_OVERRUN;
                    if (flags & MSGFLAG_REMOTE_FRAME)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_REMOTE_FRAME;
                    if (flags & MSGFLAG_ERROR_FRAME)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_ERROR_FRAME;
                    if (flags & MSGFLAG_TX)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_TXACK;
                    if (flags & MSGFLAG_TXRQ)
                        e.tagData.msg.flags |= VCAN_MSG_FLAG_TXRQ;
                    dlc = cmd.rxCanMessage.rawMessage[5] & 0x0F;
                    e.tagData.msg.dlc = dlc;
                    memcpy(e.tagData.msg.data, &cmd.rxCanMessage.rawMessage[6], 8);

                    vCanDispatchEvent(vChd, &e);
                }
                else
                {
                    DEBUGPRINT(1, "QQQ: CMD_RX_EXT_MESSAGE, dlc = %d flags = %x, chan = %d\n",
                               cmd.rxCanMessage.rawMessage[5] & 0x0F,
                               cmd.rxCanMessage.flags,chan);
                }
                break;
            }

            case CMD_TX_ACKNOWLEDGE:
            {
                // A TxAck - handle it the manual way, just as in the old PCIcan.
                // Send a tx ack. qqq review this

                unsigned int transId;
                unsigned int chan = cmd.txAck.channel;
                /*
                DEBUGPRINT(3, "CMD_TX_ACKNOWLEDGE: chan(%d), time(%d), transId(%x)\n",
                           chan,
                           cmd.txAck.time,
                           cmd.txAck.transId
                          );
                */

                if (chan < (unsigned)vCard->nrChannels) {

                    VCanChanData     *vChd = vCard->chanData[cmd.txAck.channel];
                    PciCanIIChanData *hChd = vChd->hwChanData;


#if 1
                    DEBUGPRINT(3, "ACK: ch%d, tId(%x) o:%d\n",
                               chan, cmd.txAck.transId,
                               atomic_read(&hChd->outstanding_tx));
#endif

                    transId = cmd.txAck.transId;
                    if ((transId == 0) || (transId > HELIOS_MAX_OUTSTANDING_TX)) {
                        DEBUGPRINT(1, "CMD_TX_ACKNOWLEDGE chan %d ERROR transid %d\n", chan, transId);
                        break;
                    }

                    if (hChd->current_tx_message[transId - 1].flags & VCAN_MSG_FLAG_TXACK) {
                        VCAN_EVENT *e = (VCAN_EVENT *)&hChd->current_tx_message[transId - 1];
                        e->tag       = V_RECEIVE_MSG;
                        e->timeStamp = pciCanTimeStamp(vCard, cmd.txAck.time);
                        e->tagData.msg.flags &= ~VCAN_MSG_FLAG_TXRQ;

                        vCanDispatchEvent(vChd, e);
                    }

#if 1
                    hChd->current_tx_message[transId - 1].user_data = 0;
#endif

                    // Wake up those who are waiting for all
                    // sending to finish
                    if (atomic_add_unless(&hChd->outstanding_tx, -1, 0)) {
                        // Is anyone waiting for this ack?
                        if ((atomic_read(&hChd->outstanding_tx) == 0) &&
                           queue_empty(&vChd->txChanQueue)          &&
                           test_and_clear_bit(0, &vChd->waitEmpty)) {
                           os_if_wake_up_interruptible(&vChd->flushQ);
                        }

                        if (!queue_empty(&vChd->txChanQueue))
                        {
                            DEBUGPRINT(4, "Requesting send\n");
                            pciCanRequestSend(vCard, vChd);
                        } else {
                            DEBUGPRINT(4, "Queue empty: %d\n",
                                       queue_length(&vChd->txChanQueue));
                        }
                    }
                    else
                    {
                      DEBUGPRINT(1, "TX ACK when not waiting for one\n");
                    }
                }
                else {
                  DEBUGPRINT(1, "QQQ: CMD_TX_ACKNOWLEDGE, chan = %d\n", chan);
                }
                break;
            }

            case CMD_TX_REQUEST:
            {
                unsigned int transId;
                unsigned int chan      = cmd.txRequest.channel;
                VCanChanData     *vChd = vCard->chanData[cmd.txRequest.channel];
                PciCanIIChanData *hChd = vChd->hwChanData;
                DEBUGPRINT(3, "CMD_TX_REQUEST, chan = %d, cmd.txRequest.transId = %d ",
                           chan, cmd.txRequest.transId);
                if (chan < (unsigned)vCard->nrChannels) {
                    // A TxReq. Take the current tx message, modify it to a
                    // receive message and send it back.
                    transId = cmd.txRequest.transId;
                    if ((transId == 0) || (transId > HELIOS_MAX_OUTSTANDING_TX)) {
                        DEBUGPRINT(1, "CMD_TX_REQUEST chan %d ERROR transid to high %d\n",
                                   chan, transId);
                        break;
                    }

                    if (hChd->current_tx_message[transId - 1].flags & VCAN_MSG_FLAG_TXRQ)
                    {
                        VCAN_EVENT *e          = (VCAN_EVENT *)&hChd->current_tx_message[transId - 1];
                        VCAN_EVENT tmp         = *e;
                        tmp.tag                = V_RECEIVE_MSG;
                        tmp.timeStamp          = pciCanTimeStamp(vCard, cmd.txRequest.time);
                        tmp.tagData.msg.flags &=  ~VCAN_MSG_FLAG_TXACK;

                        vCanDispatchEvent(vChd, e);
                    }
                }
                break;
            }

            case CMD_GET_BUSPARAMS_RESP:
            {
                DEBUGPRINT(1, "CMD_GET_BUSPARAMS_RESP\n");
                // qqq not used since bug in firmware
                break;
            }

            case CMD_GET_DRIVERMODE_RESP:
                DEBUGPRINT(3, "CMD_GET_DRIVERMODE_RESP\n");
                break;

            case CMD_START_CHIP_RESP:
                DEBUGPRINT(3, "CMD_START_CHIP_RESP chan %d\n", cmd.startChipResp.channel);

                break;

            case CMD_STOP_CHIP_RESP:
#if 0
            {
                VCanChanData *vChd = vCard->chanData[cmd.stopChipResp.channel];
                DEBUGPRINT(3, "transId = %d\n",
                           atomic_read(&vChd->transId));
            }
#endif
                DEBUGPRINT(3, "CMD_STOP_CHIP_RESP ch %d\n", cmd.stopChipResp.channel);
                break;

            case CMD_CHIP_STATE_EVENT:
            {
                unsigned int chan  = cmd.chipStateEvent.channel;
                VCanChanData *vChd = vCard->chanData[chan];

                if (chan < (unsigned)vCard->nrChannels) {
                    vChd->txErrorCounter = cmd.chipStateEvent.txErrorCounter;
                    vChd->rxErrorCounter = cmd.chipStateEvent.rxErrorCounter;
                }

                // ".busStatus" is the contents of the CnSTRH register.
                switch (cmd.chipStateEvent.busStatus &
                        (M16C_BUS_PASSIVE | M16C_BUS_OFF)) {
                    case 0:
                        vChd->chipState.state = CHIPSTAT_ERROR_ACTIVE;
                        break;
                    case M16C_BUS_PASSIVE:
                        vChd->chipState.state = CHIPSTAT_ERROR_PASSIVE |
                                                CHIPSTAT_ERROR_WARNING;
                        break;
                    case M16C_BUS_OFF:
                        vChd->chipState.state = CHIPSTAT_BUSOFF;
                        break;
                    case (M16C_BUS_PASSIVE | M16C_BUS_OFF):
                        vChd->chipState.state = CHIPSTAT_BUSOFF        |
                                                CHIPSTAT_ERROR_PASSIVE |
                                                CHIPSTAT_ERROR_WARNING;
                        break;
                }
                // Reset is treated like bus-off
                if (cmd.chipStateEvent.busStatus & M16C_BUS_RESET) {
                    vChd->chipState.state = CHIPSTAT_BUSOFF;
                    vChd->txErrorCounter = 0;
                    vChd->rxErrorCounter = 0;
                }

                e.tag       = V_CHIP_STATE;
                e.timeStamp = pciCanTimeStamp(vCard, cmd.chipStateEvent.time);
                e.transId   = 0;
                e.tagData.chipState.busStatus      = (unsigned char)vChd->chipState.state;
                e.tagData.chipState.txErrorCounter = (unsigned char)vChd->txErrorCounter;
                e.tagData.chipState.rxErrorCounter = (unsigned char)vChd->rxErrorCounter;
                vCanDispatchEvent(vChd, &e);

                break;
            }

            case CMD_CLOCK_OVERFLOW_EVENT:
            {
                unsigned long irqFlags;
                os_if_spin_lock_irqsave(&hCd->timeHi_lock, &irqFlags);
                vCard->timeHi = cmd.clockOverflowEvent.currentTime & 0xFFFF0000;
                os_if_spin_unlock_irqrestore(&hCd->timeHi_lock, irqFlags);
                break;
            }

            case CMD_READ_CLOCK_RESP:
            {
                unsigned long irqFlags;
                DEBUGPRINT(3, "CMD_READ_CLOCK_RESP\n");
                hCd->recClock = (cmd.readClockResp.time[1] << 16) +
                                cmd.readClockResp.time[0];
                os_if_spin_lock_irqsave(&hCd->timeHi_lock, &irqFlags);
                vCard->timeHi = cmd.readClockResp.time[1] << 16;
                os_if_spin_unlock_irqrestore(&hCd->timeHi_lock, irqFlags);
                break;
            }

            case CMD_GET_CARD_INFO_RESP:
            {
                unsigned int chan;
                chan = cmd.getCardInfoResp.channelCount;
                DEBUGPRINT(3, "CMD_GET_CARD_INFO_RESP chan = %d\n",chan);
                if (!hCd->initDone) {
                    vCard->nrChannels = chan;
                }
                memcpy(vCard->ean, &cmd.getCardInfoResp.EAN[0], 8);
                vCard->serialNumber = cmd.getCardInfoResp.serialNumberLow;
                hCd->hardware_revision_major = cmd.getCardInfoResp.hwRevision >> 4;
                hCd->hardware_revision_minor = cmd.getCardInfoResp.hwRevision & 0x0F;

                // qqq This should be per channel!
                vCard->capabilities = VCAN_CHANNEL_CAP_SEND_ERROR_FRAMES    |
                                      VCAN_CHANNEL_CAP_RECEIVE_ERROR_FRAMES |
                                      VCAN_CHANNEL_CAP_TIMEBASE_ON_CARD     |
                                      VCAN_CHANNEL_CAP_BUSLOAD_CALCULATION  |
                                      VCAN_CHANNEL_CAP_ERROR_COUNTERS       |
                                      VCAN_CHANNEL_CAP_EXTENDED_CAN         |
                                      VCAN_CHANNEL_CAP_TXREQUEST            |
                                      VCAN_CHANNEL_CAP_TXACKNOWLEDGE;

                vCard->hw_type      = HWTYPE_PCICAN_II;

                if (hCd->isWaiting) {
                    os_if_wake_up_interruptible(&hCd->waitHwInfo);
                }
                hCd->receivedHwInfo = 1;
                break;
            }

            case CMD_GET_SOFTWARE_INFO_RESP:
            {
                uint32_t appVersion = cmd.getSoftwareInfoResp.applicationVersion;
                vCard->firmwareVersionMajor = appVersion >> 24;
                vCard->firmwareVersionMinor = (appVersion >> 16) & 0xFF;
                vCard->firmwareVersionBuild = (appVersion & 0xFFFF);

                if (hCd->isWaiting) {
                    os_if_wake_up_interruptible(&hCd->waitSwInfo);
                }
                hCd->receivedSwInfo = 1;

                DEBUGPRINT(3, "PCIcanII firmware version %d.%d.%d\n",
                           (int)(appVersion >> 24) & 0xFF,
                           (int)(appVersion >> 16) & 0xFF,
                           (int)appVersion & 0xFFFF);

                break;
            }

            // qqq Not done
            case CMD_GET_TRANSCEIVER_INFO_RESP:
            {
                unsigned int chan = cmd.getTransceiverInfoResp.channel;
                VCanChanData *vChd = vCard->chanData[chan];
                DEBUGPRINT(3, "CMD_GET_TRANSCEIVER_INFO_RESP chan = %d\n",chan);
                if (chan < (unsigned)vCard->nrChannels) {
                    vChd = vCard->chanData[chan];
                    vChd->transType = cmd.getTransceiverInfoResp.transceiverType;
                }
                // Wake up
                break;
            }

            case CMD_CAN_ERROR_EVENT:
            {
                int errorCounterChanged;
                // first channel
                VCanChanData *vChd = vCard->chanData[0];

                // Known problem: if the error counters of both channels
                // are max then there is no way of knowing which channel got an errorframe

                // It's an error frame if any of our error counters has
                // increased..
                errorCounterChanged =  (cmd.canErrorEvent.txErrorCounterCh0 >
                                        vChd->txErrorCounter);
                errorCounterChanged |= (cmd.canErrorEvent.rxErrorCounterCh0 >
                                        vChd->rxErrorCounter);

                // It's also an error frame if we have seen a bus error while
                // the other channel hasn't seen any bus errors at all.
                errorCounterChanged |= ( (cmd.canErrorEvent.busStatusCh0 &
                                          M16C_BUS_ERROR) &&
                                        !(cmd.canErrorEvent.busStatusCh1 &
                                          M16C_BUS_ERROR));

                vChd->txErrorCounter = cmd.canErrorEvent.txErrorCounterCh0;
                vChd->rxErrorCounter = cmd.canErrorEvent.rxErrorCounterCh0;

                switch (cmd.canErrorEvent.busStatusCh0 &
                        (M16C_BUS_PASSIVE | M16C_BUS_OFF)) {
                    case 0:
                        vChd->chipState.state = CHIPSTAT_ERROR_ACTIVE;
                        break;

                    case M16C_BUS_PASSIVE:
                        vChd->chipState.state = CHIPSTAT_ERROR_PASSIVE |
                                                CHIPSTAT_ERROR_WARNING;
                        break;

                    case M16C_BUS_OFF:
                        vChd->chipState.state = CHIPSTAT_BUSOFF;
                        errorCounterChanged = 0;
                        break;

                    case (M16C_BUS_PASSIVE|M16C_BUS_OFF):
                        vChd->chipState.state = CHIPSTAT_BUSOFF        |
                                                CHIPSTAT_ERROR_PASSIVE |
                                                CHIPSTAT_ERROR_WARNING;
                        errorCounterChanged = 0;
                        break;

                    default:
                        break;
                }

                // Reset is treated like bus-off
                if (cmd.canErrorEvent.busStatusCh0 & M16C_BUS_RESET) {
                    vChd->chipState.state = CHIPSTAT_BUSOFF;
                    vChd->txErrorCounter = 0;
                    vChd->rxErrorCounter = 0;
                    errorCounterChanged = 0;
                }

                e.tag       = V_CHIP_STATE;
                e.timeStamp = pciCanTimeStamp(vCard, cmd.canErrorEvent.time);
                e.transId                           = 0;
                e.tagData.chipState.busStatus       = vChd->chipState.state; // qqq really chipState?
                e.tagData.chipState.txErrorCounter  = vChd->txErrorCounter;
                e.tagData.chipState.rxErrorCounter  = vChd->rxErrorCounter;
                vCanDispatchEvent(vChd, &e);

                if (errorCounterChanged) {
                  e.tag               = V_RECEIVE_MSG;
                  e.transId           = 0;
                  e.timeStamp         = pciCanTimeStamp(vCard, cmd.canErrorEvent.time);
                  e.tagData.msg.id    = 0;
                  e.tagData.msg.flags = VCAN_MSG_FLAG_ERROR_FRAME;
                  e.tagData.msg.dlc   = 0;
                  vCanDispatchEvent(vChd, &e);
                }

                // Next channel
                if ((unsigned)vCard->nrChannels > 0) {

                    VCanChanData *vChd = vCard->chanData[1];

                    // It's an error frame if any of our error counters has
                    // increased..
                    errorCounterChanged  = (cmd.canErrorEvent.txErrorCounterCh1 >
                                            vChd->txErrorCounter);
                    errorCounterChanged |= (cmd.canErrorEvent.rxErrorCounterCh1 >
                                            vChd->rxErrorCounter);

                    // It's also an error frame if we have seen a bus error while
                    // the other channel hasn't seen any bus errors at all.
                    errorCounterChanged |= ( (cmd.canErrorEvent.busStatusCh1 &
                                              M16C_BUS_ERROR) &&
                                            !(cmd.canErrorEvent.busStatusCh0 &
                                              M16C_BUS_ERROR));

                    vChd->txErrorCounter = cmd.canErrorEvent.txErrorCounterCh1;
                    vChd->rxErrorCounter = cmd.canErrorEvent.rxErrorCounterCh1;

                    switch (cmd.canErrorEvent.busStatusCh1 &
                            (M16C_BUS_PASSIVE | M16C_BUS_OFF)) {
                        case 0:
                            vChd->chipState.state = CHIPSTAT_ERROR_ACTIVE;
                            break;

                        case M16C_BUS_PASSIVE:
                            vChd->chipState.state = CHIPSTAT_ERROR_PASSIVE |
                                                    CHIPSTAT_ERROR_WARNING;
                            break;

                        case M16C_BUS_OFF:
                            vChd->chipState.state = CHIPSTAT_BUSOFF;
                            errorCounterChanged = 0;
                            break;

                        case (M16C_BUS_PASSIVE | M16C_BUS_OFF):
                            vChd->chipState.state = CHIPSTAT_BUSOFF        |
                                                    CHIPSTAT_ERROR_PASSIVE |
                                                    CHIPSTAT_ERROR_WARNING;
                            errorCounterChanged = 0;
                            break;

                        default:
                            break;
                    }

                    // Reset is treated like bus-off
                    if (cmd.canErrorEvent.busStatusCh1 & M16C_BUS_RESET) {
                        vChd->chipState.state = CHIPSTAT_BUSOFF;
                        vChd->txErrorCounter  = 0;
                        vChd->rxErrorCounter  = 0;
                        errorCounterChanged   = 0;
                    }

                    e.tag       = V_CHIP_STATE;
                    e.timeStamp = pciCanTimeStamp(vCard, cmd.canErrorEvent.time);
                    e.transId                           = 0;
                    e.tagData.chipState.busStatus       = vChd->chipState.state;
                    e.tagData.chipState.txErrorCounter  = vChd->txErrorCounter;
                    e.tagData.chipState.rxErrorCounter  = vChd->rxErrorCounter;
                    vCanDispatchEvent(vChd, &e);

                    if (errorCounterChanged) {
                      e.tag               = V_RECEIVE_MSG;
                      e.transId           = 0;
                      e.timeStamp         = pciCanTimeStamp(vCard, cmd.canErrorEvent.time);
                      e.tagData.msg.id    = 0;
                      e.tagData.msg.flags = VCAN_MSG_FLAG_ERROR_FRAME;
                      e.tagData.msg.dlc   = 0;
                      vCanDispatchEvent(vChd, &e);
                    }
                }
                break;
            }

            case CMD_ERROR_EVENT:
            {
#ifdef PCICANII_DEBUG
              VCanChanData *vChd = vCard->chanData[0];
              DEBUGPRINT(1, "CMD_ERROR_EVENT, chan = %d\n", vChd->channel);
#endif
              break;
            }
            case 0:
                // This means we have read corrupted data
                DEBUGPRINT(1, "ERROR: Corrupt data. QQQ\n");
                return;

            default:
                DEBUGPRINT(1, "Unknown command %d received. QQQ\n", cmd.head.cmdNo);
                break;
        }


        if (cmd.head.cmdNo > CMD_TX_EXT_MESSAGE) {
            // Copy command and wakeup those who are waiting for this reply
            struct list_head *currHead, *tmpHead;
            WaitNode *currNode;
            unsigned long irqFlags;
            os_if_read_lock_irqsave(&hCd->replyWaitListLock, &irqFlags);
            list_for_each_safe(currHead, tmpHead, &hCd->replyWaitList) {
                currNode = list_entry(currHead, WaitNode, list);
                if (currNode->cmdNr == cmd.head.cmdNo &&
                    getTransId(&cmd) == currNode->transId) {
                    copyCmd(currNode->replyPtr, &cmd);
                    os_if_up_sema(&currNode->waitSemaphore);
                }
            }
            os_if_read_unlock_irqrestore(&hCd->replyWaitListLock, irqFlags);
        }
    }

} // pciCanReceiveIsr


//======================================================================
//  Main ISR
//======================================================================
// Interrupt handler prototype changed in 2.6.19.
static OS_IF_INTR_HANDLER
#if (LINUX_VERSION_CODE < 0x020613)
pciCanInterrupt (int irq, void *dev_id, struct pt_regs *regs)
#else
pciCanInterrupt (int irq, void *dev_id)
#endif
{
    VCanCardData      *vCard   = (VCanCardData *)dev_id;
    PciCanIICardData  *hCd     = vCard->hwCardData;
    unsigned long     tmp;
    unsigned int      loopMax  = 1000;
    int               handled  = 0;

    tmp = ioread32(hCd->baseAddr + DPRAM_INTERRUPT_REG);

    while ((tmp & DPRAM_INTERRUPT_ACTIVE) != 0) {
        iowrite32(tmp | DPRAM_INTERRUPT_ACK, hCd->baseAddr + DPRAM_INTERRUPT_REG);
        // This ioread32 was added 051208 because otherwise we cannot be
        // sure that the first iowrite32 isn't overwritten by the next one.
        ioread32(hCd->baseAddr + DPRAM_INTERRUPT_REG);
        iowrite32(tmp & ~DPRAM_INTERRUPT_ACK, hCd->baseAddr + DPRAM_INTERRUPT_REG);

        tmp = ioread32(hCd->baseAddr + DPRAM_INTERRUPT_REG);
        handled = 1;

        if (--loopMax == 0) {
            // Kill the card.
            DEBUGPRINT(1, "Channel runaway.\n");
            pciCanInterrupts(vCard, 0);
            return IRQ_HANDLED;
        }
        pciCanReceiveIsr(vCard);
        tmp = ioread32(hCd->baseAddr + DPRAM_INTERRUPT_REG);
    }

    return IRQ_RETVAL(handled);
} // pciCanInterrupt


//======================================================================
//  Sends a can message
//======================================================================
static int pciCanTransmitMessage (VCanChanData *vChd, CAN_MSG *m)
{
    PciCanIICardData *hCard = vChd->vCard->hwCardData;
    PciCanIIChanData *hChd  = vChd->hwChanData;
    heliosCmd         msg;
    int               transId;

    if (!atomic_add_unless(&hChd->outstanding_tx, 1,
                           HELIOS_MAX_OUTSTANDING_TX)) {
        DEBUGPRINT(1, "Trying to increase outstanding_tx above max\n");
        return VCAN_STAT_NO_RESOURCES;
    }

    // Save a copy of the message.
    // qqq The transid update itself should be atomic!
    transId = atomic_read(&vChd->transId);

#if 1
    if (hChd->current_tx_message[transId - 1].user_data) {
        DEBUGPRINT(1, "In use: %x %d   %x %d\n", 
                   hChd->current_tx_message[transId - 1].id, transId,
                   m->id, atomic_read(&hChd->outstanding_tx));
    }
#endif

    hChd->current_tx_message[transId - 1] = *m;

    msg.txCanMessage.cmdLen       = sizeof(cmdTxCanMessage);
    msg.txCanMessage.channel      = (unsigned char)vChd->channel;
    msg.txCanMessage.transId      = (unsigned char)transId;

#if 0
    atomic_add(1, &hChd->outstanding_tx);
#endif

    if (m->id & VCAN_EXT_MSG_ID) { // Extended CAN
        msg.txCanMessage.cmdNo         = CMD_TX_EXT_MESSAGE;
        msg.txCanMessage.rawMessage[0] = (unsigned char)((m->id >> 24) & 0x1F);
        msg.txCanMessage.rawMessage[1] = (unsigned char)((m->id >> 18) & 0x3F);
        msg.txCanMessage.rawMessage[2] = (unsigned char)((m->id >> 14) & 0x0F);
        msg.txCanMessage.rawMessage[3] = (unsigned char)((m->id >>  6) & 0xFF);
        msg.txCanMessage.rawMessage[4] = (unsigned char)((m->id      ) & 0x3F);
    }
    else { // Standard CAN
        msg.txCanMessage.cmdNo         = CMD_TX_STD_MESSAGE;
        msg.txCanMessage.rawMessage[0] = (unsigned char)((m->id >>  6) & 0x1F);
        msg.txCanMessage.rawMessage[1] = (unsigned char)((m->id      ) & 0x3F);
    }
    msg.txCanMessage.rawMessage[5] = m->length & 0x0F;
    memcpy(&msg.txCanMessage.rawMessage[6], m->data, 8);

    msg.txCanMessage.flags = m->flags & (VCAN_MSG_FLAG_TX_NOTIFY   |
                                         VCAN_MSG_FLAG_TX_START    |
                                         VCAN_MSG_FLAG_ERROR_FRAME |
                                         VCAN_MSG_FLAG_REMOTE_FRAME);
    // Transmit is capable of retrying without involving pciCanNoResponse().
    if (QCmd(hCard, &msg)) {
#if 0
        DEBUGPRINT(1, "*********MEMQ error can msg******** \n");
#endif
        // We must restore outstanding_tx here!
        atomic_dec(&hChd->outstanding_tx);
#if 1
        hChd->current_tx_message[transId - 1].user_data = 0;
#endif
        return VCAN_STAT_NO_RESOURCES;
    }

    // Update transId after we know QCmd() was succesful!
    if (transId + 1 > HELIOS_MAX_OUTSTANDING_TX) {
#if 0
        DEBUGPRINT(3, "transId = %d (%p), rewind\n", transId, &vChd->transId);
#endif
        atomic_set(&vChd->transId, 1);
    }
    else {
#if 0
        DEBUGPRINT(3, "transId = %d (%p), add\n", transId, &vChd->transId);
#endif
        atomic_add(1, &vChd->transId);
    }

    /*
    // dispatch message??
    if (m->flags & VCAN_MSG_FLAG_TXRQ) {
        DEBUGPRINT(1, "Msg with VCAN_MSG_FLAG_TXRQ\n");
    }

    if (msg.txCanMessage.flags & VCAN_MSG_FLAG_ERROR_FRAME) {
      DEBUGPRINT(1, "pciCanTransmitMessage: Transmit ERROR FRAME\n");
    }
    */

    return VCAN_STAT_OK;
} // pciCanTransmitMessage


//======================================================================
//  Read transmit error counter
//======================================================================
static int pciCanGetTxErr (VCanChanData *vChd)
{
    pciCanRequestChipState(vChd);

    return vChd->txErrorCounter;
}


//======================================================================
//  Read transmit error counter
//======================================================================
static int pciCanGetRxErr (VCanChanData *vChd)
{
    pciCanRequestChipState(vChd);

    return vChd->rxErrorCounter;
}


//======================================================================
//  Read receive queue length in hardware/firmware
//======================================================================
static unsigned long pciCanRxQLen (VCanChanData *vChd)
{
  // qqq Why _tx_ channel buffer?
    return queue_length(&vChd->txChanQueue);
}


//======================================================================
//  Read transmit queue length in hardware/firmware
//======================================================================
static unsigned long pciCanTxQLen (VCanChanData *vChd)
{
    PciCanIIChanData *hChd  = vChd->hwChanData;

    return atomic_read(&hChd->outstanding_tx);
}


//======================================================================
//  Clear send buffer on card
//  qqq Shouldn't this be atomic?
//======================================================================
static int pciCanFlushSendBuffer (VCanChanData *vChan)
{
    PciCanIIChanData *hChd  = vChan->hwChanData;
    VCanCardData     *vCard = vChan->vCard;
    PciCanIICardData *hCard = vCard->hwCardData;
    heliosCmd cmd;
    int ret;

    cmd.head.cmdNo         = CMD_FLUSH_QUEUE;
    cmd.flushQueue.cmdLen  = sizeof(cmd.flushQueue);
    cmd.flushQueue.channel = (unsigned char)vChan->channel;
    cmd.flushQueue.flags   = 0;

    ret = pciCanNoResponse(hCard, &cmd);
    if (ret == VCAN_STAT_OK) {
        atomic_set(&hChd->outstanding_tx, 0);
#if 0
        atomic_set(&vChan->transId, 1);
#endif

        queue_reinit(&vChan->txChanQueue);
    } else {
        DEBUGPRINT(1, "ERROR: pciCanFlushSendBuffer\n");
    }

    return ret;
}


//======================================================================
//  Initialize H/W
//======================================================================
static int DEVINIT pciCanInitHW (VCanCardData *vCard)
{
    PciCanIICardData  *hCard = vCard->hwCardData;
    void __iomem      *addr;
    int               timeOut = 0;
    heliosCmd         cmd;
    int               i;

    // The card must be present!
    if (!vCard->cardPresent) {
        DEBUGPRINT(1, "Error: The card must be present!\n");
        return VCAN_STAT_NO_DEVICE;
    }

    addr = hCard->baseAddr;
    if (!addr) {
        DEBUGPRINT(1, "Error: In address!\n");
        vCard->cardPresent = 0;
        return VCAN_STAT_FAIL;
    }

    if (!MemQSanityCheck(hCard)) {
        DEBUGPRINT(1, "DPRAM communication failed - check hardware");
        vCard->cardPresent = 0;
        return VCAN_STAT_NO_DEVICE;
    }

    os_if_init_waitqueue_head(&hCard->waitHwInfo);
    os_if_init_waitqueue_head(&hCard->waitSwInfo);
    hCard->isWaiting = 0;

    // Enable interrupts from card and reset it.
    // We should get a CMD_GET_CARD_INFO_RESP
    // and a CMD_GET_SOFTWARE_INFO_RESP back.
    pciCanInterrupts(vCard, 1);
    cmd.head.cmdNo          = CMD_RESET_CARD_REQ;
    cmd.resetCardReq.cmdLen = sizeof(cmd.resetCardReq);
    // This is at startup, so QCmd() should not fail.
    if (QCmd(hCard, &cmd)) {
        DEBUGPRINT(1, "Error: In QCmd (reset card)\n");
        goto error;
    }

    hCard->isWaiting = 1;
    timeOut = os_if_wait_event_interruptible_timeout(hCard->waitHwInfo,
                                                     hCard->receivedHwInfo, 1000);
    if (!timeOut) {
        DEBUGPRINT(1, "no HW wakeup\n");
        goto error;
    }

    timeOut = os_if_wait_event_interruptible_timeout(hCard->waitSwInfo,
                                                     hCard->receivedSwInfo, 1000);
    if (!timeOut) {
        DEBUGPRINT(1, "no SW wakeup\n");
        goto error;
    }
    hCard->isWaiting = 0;

    for (i = 0; i < vCard->nrChannels; i++) {
        VCanChanData     *vChd = vCard->chanData[i];

        cmd.resetChipReq.cmdNo   = CMD_RESET_CHIP_REQ;
        cmd.resetChipReq.cmdLen  = sizeof(cmd.resetChipReq);
        cmd.resetChipReq.channel = (unsigned char)vChd->channel;

        // This is at startup, so QCmd() should not fail.
        if (QCmd(hCard, &cmd) != MEM_Q_SUCCESS) {
            DEBUGPRINT(1, "Error: In QCmd (reset chip)\n");
            goto error;
        }
    }
    hCard->initDone = 1;

    DEBUGPRINT(1, "pcicanII: hw initialized. \n");

    return VCAN_STAT_OK;

error:
    os_if_delete_waitqueue_head(&hCard->waitHwInfo);
    os_if_delete_waitqueue_head(&hCard->waitSwInfo);
    hCard->isWaiting   = 0;
    hCard->initDone    = 0;
    vCard->cardPresent = 0;

    return VCAN_STAT_FAIL;
}


//======================================================================
//  Find out addresses for one card
//======================================================================
static int DEVINIT readPCIAddresses (struct pci_dev *dev, VCanCardData *vCard)
{
  PciCanIICardData *hCd = vCard->hwCardData;

  if (pci_request_regions(dev, "Kvaser PCIcanII")) {
    DEBUGPRINT(1, "request regions failed\n");
    return VCAN_STAT_FAIL;
  }

  if (pci_enable_device(dev)) {
    DEBUGPRINT(1, "enable device failed\n");
    pci_release_regions(dev);
    return VCAN_STAT_NO_DEVICE;
  }

  hCd->irq = dev->irq;
  hCd->baseAddr = pci_iomap(dev, 0, 0);

  if (!hCd->baseAddr) {
    DEBUGPRINT(1, "pci_iomap failed\n");
    pci_disable_device(dev);
    pci_release_regions(dev);
    return VCAN_STAT_FAIL;
  }

  DEBUGPRINT(1, "baseAddr = 0x%lx\n", (long)hCd->baseAddr);

  return VCAN_STAT_OK;
}


//======================================================================
// Request send
//======================================================================
static int pciCanRequestSend (VCanCardData *vCard, VCanChanData *vChan)
{
    PciCanIIChanData *hChan = vChan->hwChanData;
    if (pciCanTxAvailable(vChan)) {
        os_if_queue_task(&hChan->txTaskQ);
    }
#if DEBUG
    else {
        DEBUGPRINT(3, "SEND FAILED \n");
    }
#endif

    return VCAN_STAT_OK;
}


//======================================================================
//  Process send Q - This function is called from the immediate queue
//======================================================================
#if LINUX_VERSION_CODE < 0x020614
static void pciCanSend (void *void_chanData)
#else
static void pciCanSend (OS_IF_TASK_QUEUE_HANDLE *work)
#endif
{
#if LINUX_VERSION_CODE < 0x020614
    VCanChanData *chd = (VCanChanData *)void_chanData;
#else
    PciCanIIChanData *devChan = container_of(work, PciCanIIChanData, txTaskQ);
    VCanChanData   *chd = devChan->vChan;
#endif
    int queuePos;

    if (!chd->isOnBus) {
        DEBUGPRINT(2, "Attempt to send when not on bus\n");
        return;
    }

    if (chd->minorNr < 0) {  // Channel not initialized?
        DEBUGPRINT(2, "Attempt to send on unitialized channel\n");
        return;
    }

    if (!pciCanTxAvailable(chd)) {
        DEBUGPRINT(2, "Maximum number of messages outstanding reached\n");
        return;
    }

    // Send Messages
    queuePos = queue_front(&chd->txChanQueue);
    if (queuePos >= 0) {
        if (pciCanTransmitMessage(chd, &chd->txChanBuffer[queuePos]) ==
              VCAN_STAT_OK) {
            DEBUGPRINT(2, "Message sent\n");
            queue_pop(&chd->txChanQueue);
            queue_wakeup_on_space(&chd->txChanQueue);
        } else {
            DEBUGPRINT(2, "Message send failed\n");
            queue_release(&chd->txChanQueue);
            // Need to retry work!
            os_if_queue_task(&devChan->txTaskQ);
        }
    } else {
        DEBUGPRINT(2, "Nothing in queue\n");
        queue_release(&chd->txChanQueue);
    }

    return;
}


//======================================================================
//  Timeout handler for the waitResponse below
//======================================================================
static void responseTimeout (unsigned long voidWaitNode)
{
#if LINUX
    WaitNode *waitNode = (WaitNode *)voidWaitNode;
    waitNode->timedOut = 1;
    os_if_up_sema(&waitNode->waitSemaphore);
    return;
#else
#           pragma message("responseTimeout...")
    return;
#endif
}


//======================================================================
// Send out a command and wait for a response with timeout
//======================================================================
static int pciCanWaitResponse (VCanCardData *vCard, heliosCmd *cmd,
                               heliosCmd *replyPtr, unsigned char cmdNr,
                               unsigned char transId)
{

    PciCanIICardData *hCard = vCard->hwCardData;
    WaitNode waitNode;
    unsigned long irqFlags = 0;
    struct timer_list waitTimer;

    os_if_init_sema(&waitNode.waitSemaphore);

    waitNode.replyPtr = replyPtr;
    waitNode.cmdNr = cmdNr;
    waitNode.transId = transId;
    waitNode.timedOut = 0;

    // Add to card's list of expected responses

    os_if_write_lock_irqsave(&hCard->replyWaitListLock, &irqFlags);
    list_add(&waitNode.list, &hCard->replyWaitList);
    os_if_write_unlock_irqrestore(&hCard->replyWaitListLock, irqFlags);

    if (pciCanNoResponse(hCard, cmd)) {
        DEBUGPRINT(1, "ERROR----- pciCanWaitResponse----------\n");
        os_if_write_lock_irqsave(&hCard->replyWaitListLock, &irqFlags);
        list_del(&waitNode.list);
        os_if_write_unlock_irqrestore(&hCard->replyWaitListLock, irqFlags);
        return VCAN_STAT_NO_RESOURCES;
    }

    init_timer(&waitTimer);
    waitTimer.function = responseTimeout;
    waitTimer.data = (unsigned long)&waitNode;
    waitTimer.expires = jiffies + msecs_to_jiffies(PCICANII_CMD_RESP_WAIT_TIME);
    add_timer(&waitTimer);

    os_if_down_sema(&waitNode.waitSemaphore);

    // Now we either got a response or a timeout
    os_if_write_lock_irqsave(&hCard->replyWaitListLock, &irqFlags);
    list_del(&waitNode.list);
    os_if_write_unlock_irqrestore(&hCard->replyWaitListLock, irqFlags);
    del_timer_sync(&waitTimer);

    if (waitNode.timedOut) {
        DEBUGPRINT(1, "pciCanWaitResponse: return VCAN_STAT_TIMEOUT\n");
        return VCAN_STAT_TIMEOUT;
    }

    return VCAN_STAT_OK;
}


//======================================================================
// Send out a command but do not wait for a response
// qqq Should perhaps be extended later to be able to wait and/or queue.
//======================================================================
static int pciCanNoResponse (PciCanIICardData *hCard, heliosCmd *cmd)
{
    // qqq This should perhaps wait when no space is available,
    //     and keep the ordering amongst waiters.
    if (QCmd(hCard, cmd) != MEM_Q_SUCCESS) {
        return VCAN_STAT_NO_RESOURCES;
    }

    return VCAN_STAT_OK;
}


//======================================================================
//  Initialize H/W specific data
//======================================================================
static void DEVINIT pciCanInitData (VCanCardData *vCard)
{
    PciCanIICardData *hCd = vCard->hwCardData;

    int chNr;
    vCanInitData(vCard);

    INIT_LIST_HEAD(&hCd->replyWaitList);

    os_if_rwlock_init(&hCd->replyWaitListLock);
    os_if_spin_lock_init(&hCd->timeHi_lock);
    os_if_spin_lock_init(&hCd->memQLock);   // Only used in memQ.c

    for (chNr = 0; chNr < vCard->nrChannels; chNr++) {
        VCanChanData *vChd = vCard->chanData[chNr];
        PciCanIIChanData *hChd = vChd->hwChanData;
        hChd->vChan = vChd;
        os_if_init_task(&hChd->txTaskQ, pciCanSend, vChd);
        memset(hChd->current_tx_message, 0, sizeof(hChd->current_tx_message));
        atomic_set(&hChd->outstanding_tx, 0);
        atomic_set(&vChd->transId, 1);
#if 0
        DEBUGPRINT(3, "Initialized %d: %d (%p)\n",
                   chNr, atomic_read(&vChd->transId), &vChd->transId);
#endif
        vChd->overrun    = 0;        // qqq overrun not used
        vChd->errorCount = 0;
        vChd->errorTime  = 0;
    }
}


//======================================================================
// Initialize the HW for one card
//======================================================================
#if 0
static int DEVINIT pciCanInitOne (struct pci_dev *dev)
#else
static int DEVINIT pciCanInitOne (struct pci_dev *dev,
                                  const struct pci_device_id *id)
#endif
{
    // Helper struct for allocation
    typedef struct {
        VCanChanData *dataPtrArray[MAX_CHANNELS];
        VCanChanData vChd[MAX_CHANNELS];
        PciCanIIChanData hChd[MAX_CHANNELS];
    } ChanHelperStruct;

    ChanHelperStruct *chs;
    PciCanIICardData *hCd;

    int chNr;
    VCanCardData *vCard;

    // Allocate data area for this card
    vCard  = os_if_kernel_malloc(sizeof(VCanCardData) + sizeof(PciCanIICardData));
    if (!vCard) {
      goto card_alloc_err;
    }
    memset(vCard, 0, sizeof(VCanCardData) + sizeof(PciCanIICardData));

    // hwCardData is directly after VCanCardData
    vCard->hwCardData = vCard + 1;
    hCd = vCard->hwCardData;

    // Allocate memory for n channels
    chs = os_if_kernel_malloc(sizeof(ChanHelperStruct));
    if (!chs) {
      goto chan_alloc_err;
    }
    memset(chs, 0, sizeof(ChanHelperStruct));

    // Init array and hwChanData
    for (chNr = 0; chNr < MAX_CHANNELS; chNr++){
        chs->dataPtrArray[chNr]    = &chs->vChd[chNr];
        chs->vChd[chNr].hwChanData = &chs->hChd[chNr];
        chs->vChd[chNr].minorNr    = -1;   // No preset minor number
    }
    vCard->chanData = chs->dataPtrArray;

    // Get PCI controller address
    if (readPCIAddresses(dev, vCard) != VCAN_STAT_OK) {
        DEBUGPRINT(1, "readPCIAddresses failed");
        goto pci_err;
    }

    // Find out type of card i.e. N/O channels etc
    if (pciCanProbe(vCard) != VCAN_STAT_OK) {
        DEBUGPRINT(1, "pciCanProbe failed");
        goto probe_err;
    }

    // Init channels
    pciCanInitData(vCard);

    pci_set_drvdata(dev, vCard);

    // ISR
    // SA_* changed name in 2.6.18 and the old ones were to go away January '07
    if (request_irq(hCd->irq, pciCanInterrupt,
#if (LINUX_VERSION_CODE < 0x020612)
                    SA_SHIRQ,
#else
                    IRQF_SHARED,
#endif
                    "Kvaser PCIcanII", vCard)) {
        printk("<1>request_irq failed");
        goto irq_err;
    }

    // Init h/w & enable interrupts in PCI Interface
    if (pciCanInitHW(vCard) != VCAN_STAT_OK) {
        DEBUGPRINT(1, "pciCanInitHW failed\n");
        goto inithw_err;
    }

    // Insert into list of cards
    os_if_spin_lock(&canCardsLock);
    vCard->next = canCards;
    canCards = vCard;
    os_if_spin_unlock(&canCardsLock);

    return VCAN_STAT_OK;

inithw_err:
    free_irq(hCd->irq, vCard);
irq_err:
    pci_set_drvdata(dev, NULL);
    for (chNr = 0; chNr < vCard->nrChannels; chNr++) {
#if 0  // No task to destroy since none declared (using standard work queue).
      PciCanIIChanData *hChd = vCard->chanData[chNr]->hwChanData;
      os_if_destroy_task(&hChd->txTaskQ);
#endif
      os_if_spin_lock_remove(&vCard->chanData[chNr]->openLock);
    }
    os_if_rwlock_remove(&hCd->replyWaitListLock);
    os_if_spin_lock_remove(&hCd->timeHi_lock);
    os_if_spin_lock_remove(&hCd->memQLock);
probe_err:
    pci_iounmap(dev, hCd->baseAddr);
    pci_disable_device(dev);
    pci_release_regions(dev);
pci_err:
    os_if_kernel_free(vCard->chanData);
chan_alloc_err:
    os_if_kernel_free(vCard);
card_alloc_err:

    return VCAN_STAT_FAIL;
} // pciCanInitOne


//======================================================================
// Shut down the HW for one card
//======================================================================
static void DEVEXIT pciCanRemoveOne (struct pci_dev *dev)
{
  VCanCardData *vCard, *lastCard;
  VCanChanData *vChan;
  PciCanIICardData *hCd;
  int chNr;

  vCard = pci_get_drvdata(dev);
  hCd   = vCard->hwCardData;
  pci_set_drvdata(dev, NULL);

  pciCanInterrupts(vCard, 0);

  free_irq(hCd->irq, vCard);

  // qqq Mark as not working somehow!

  for (chNr = 0; chNr < vCard->nrChannels; chNr++) {
    vChan = vCard->chanData[chNr];
    DEBUGPRINT(3, "Waiting for all closed on minor %d\n", vChan->minorNr);
    while (atomic_read(&vChan->fileOpenCount) > 0) {
      os_if_wait_for_event_timeout_simple(10);
    }
  }

  for (chNr = 0; chNr < vCard->nrChannels; chNr++) {
#if 0  // No task to destroy since none declared (using standard work queue).
    PciCanIIChanData *hChd = vCard->chanData[chNr]->hwChanData;
    os_if_destroy_task(&hChd->txTaskQ);
#endif
    os_if_spin_lock_remove(&vCard->chanData[chNr]->openLock);
  }

  os_if_rwlock_remove(&hCd->replyWaitListLock);
  os_if_spin_lock_remove(&hCd->timeHi_lock);
  os_if_spin_lock_remove(&hCd->memQLock);

  pci_iounmap(dev, hCd->baseAddr);
  pci_disable_device(dev);
  pci_release_regions(dev);

  // Remove from canCards list
  os_if_spin_lock(&canCardsLock);
  lastCard = canCards;
  if (lastCard == vCard) {
    canCards = vCard->next;
  } else {
    while (lastCard && (lastCard->next != vCard)) {
      lastCard = lastCard->next;
    }
    if (!lastCard) {
      DEBUGPRINT(1, "Card not in list!\n");
    } else {
      lastCard->next = vCard->next;
    }
  }
  os_if_spin_unlock(&canCardsLock);

  os_if_kernel_free(vCard->chanData);
  os_if_kernel_free(vCard);
}


//======================================================================
// Find and initialize all cards
//======================================================================
static int INIT pciCanInitAllDevices (void)
{
    int found;

    driverData.deviceName = DEVICE_NAME_STRING;

    found = pci_register_driver(&pcican_tbl);
    DEBUGPRINT(1, "pciCanInitAllDevices %d\n", found);

    // We need to find at least one
    return  (found < 0) ? found : 0;
} // pciCanInitAllDevices


//======================================================================
// Shut down and free resources before unloading driver
//======================================================================
static int EXIT pciCanCloseAllDevices (void)
{
    // qqq check for open files
    DEBUGPRINT(1, "pciCanCloseAllDevices\n");
    pci_unregister_driver(&pcican_tbl);

    return 0;
} // pciCanCloseAllDevices
