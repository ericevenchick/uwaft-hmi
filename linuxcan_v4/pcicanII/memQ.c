/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

//////////////////////////////////////////////////////////////////////////////////
// FILE: memQ.c                                /
//////////////////////////////////////////////////////////////////////////////////
// memQ  --  Structs and functions for manipulating memQ             /
//////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------
// NOTE! module_versioning HAVE to be included first
#include "module_versioning.h"
//--------------------------------------------------


#include <asm/io.h>

#include "osif_functions_kernel.h"
#include "helios_cmds.h"
#include "memq.h"

/////////////////////////////////////////////

#define MEM_Q_TX_BUFFER       1
#define MEM_Q_RX_BUFFER       2

/////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////

int MemQSanityCheck (PciCanIICardData *ci)
{
    uint32_t p;
    unsigned long irqFlags;

    os_if_spin_lock_irqsave(&ci->memQLock, &irqFlags);

    p = ioread32(ci->baseAddr + DPRAM_HOST_WRITE_PTR);
    if (p > 10000)
        goto error;

    p = ioread32(ci->baseAddr + DPRAM_HOST_READ_PTR);
    if (p > 10000)
        goto error;

    p = ioread32(ci->baseAddr + DPRAM_M16C_WRITE_PTR);
    if (p > 10000)
        goto error;

    p = ioread32(ci->baseAddr + DPRAM_M16C_READ_PTR);
    if (p > 10000)
        goto error;

    os_if_spin_unlock_irqrestore(&ci->memQLock, irqFlags);

    return 1;

error:
    os_if_spin_unlock_irqrestore(&ci->memQLock, irqFlags);

    return 0;
}

/////////////////////////////////////////////////////////////////////////
//help fkt for Tx/Rx- FreeSpace(...)
/////////////////////////////////////////////////////////////////////////

static int AvailableSpace (unsigned int  cmdLen, unsigned long rAddr,  
                           unsigned long wAddr,  unsigned long bufStart, 
                           unsigned int bufSize) 
{
    int remaining;
    //cmdLen in bytes

    // Note that "remaining" is not actually number of remaining bytes.
    // Also, the writer never wraps for a single message (it actually
    // guarantees that the write pointer will never be too close to
    // the top of the buffer, so the first case below will always be ok).
    if (wAddr >= rAddr) {
      remaining = bufSize - (wAddr - bufStart) - cmdLen;
    } else {
      remaining = (rAddr - wAddr - 1) - cmdLen;
    }

    // Shout if enough space not available!
    if (remaining < 0) {
      static int count = 0;
      if (count++ % 1000 == 0) {
        printk("<1>memQ: Too little space remaining (%d): %d\n",
               remaining, count);
      }
    }

    return remaining >= 0;
}

////////////////////////////////////////////////////////////////////////////
//returns true if there is enough room for cmd in Tx-memQ, else false
////////////////////////////////////////////////////////////////////////////
static int TxAvailableSpace (PciCanIICardData *ci, unsigned int cmdLen) 
{
    uint32_t hwp, crp;
    int      tmp;

    hwp = ioread32(ci->baseAddr + DPRAM_HOST_WRITE_PTR);
    crp = ioread32(ci->baseAddr + DPRAM_M16C_READ_PTR);
    tmp = AvailableSpace(cmdLen,
                         (unsigned long)(ci->baseAddr + crp),
                         (unsigned long)(ci->baseAddr + hwp),
                         (unsigned long)(ci->baseAddr + DPRAM_TX_BUF_START),
                         DPRAM_TX_BUF_SIZE);

    return tmp;
}


//////////////////////////////////////////////////////////////////////////

typedef struct _tmp_context {
    PciCanIICardData *ci;
    heliosCmd *cmd;
    int res;
} TMP_CONTEXT;


int QCmd (PciCanIICardData *ci, heliosCmd *cmd) 
{
    int           i;
    void __iomem  *p;
    uint32_t      hwp, crp;
    uint32_t      *tmp;
    void __iomem  *addr = ci->baseAddr;
    unsigned long irqFlags;
    int           pos;

    os_if_spin_lock_irqsave(&ci->memQLock, &irqFlags);

    if (!TxAvailableSpace(ci, cmd->head.cmdLen)) {
        os_if_spin_unlock_irqrestore(&ci->memQLock, irqFlags);
        return MEM_Q_FULL;
    }

    hwp = ioread32(addr + DPRAM_HOST_WRITE_PTR);
    crp = ioread32(addr + DPRAM_M16C_READ_PTR);

    p = addr + hwp;
    tmp = (uint32_t *)cmd;

    for (i = 0; i < cmd->head.cmdLen; i += 4) {
        iowrite32(*tmp++, p);
        p += 4;
    }

    pos = hwp + cmd->head.cmdLen;
 
    if ((pos + MAX_CMD_LEN) > (DPRAM_TX_BUF_START + DPRAM_TX_BUF_SIZE)) {
        pos = DPRAM_TX_BUF_START;
    }

    iowrite32(pos, addr + DPRAM_HOST_WRITE_PTR);

    os_if_spin_unlock_irqrestore(&ci->memQLock, irqFlags);

    return MEM_Q_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
int GetCmdFromQ (PciCanIICardData *ci, heliosCmd *cmdPtr) 
{
    void __iomem  *p;
    uint32_t      hrp, cwp;
    uint32_t      *tmp;
    int           empty;
    void __iomem  *addr = ci->baseAddr;
    unsigned long irqFlags;
    int           pos;

    os_if_spin_lock_irqsave(&ci->memQLock, &irqFlags);

    hrp = ioread32(addr + DPRAM_HOST_READ_PTR);
    cwp = ioread32(addr + DPRAM_M16C_WRITE_PTR);

    empty = (hrp == cwp);

    if (!empty) {
        int len;

        p = addr + hrp;
        tmp = (uint32_t *)cmdPtr;
        *tmp++ = ioread32(p);
        len = cmdPtr->head.cmdLen - 4;
        p += 4;

        while (len > 0) {
            *tmp++ = ioread32(p);
            len -= 4;
            p += 4;
        }

        pos = hrp + cmdPtr->head.cmdLen;

        if ((pos + MAX_CMD_LEN) > (DPRAM_RX_BUF_START + DPRAM_RX_BUF_SIZE)) {
            pos = DPRAM_RX_BUF_START;
        }

        iowrite32(pos, addr + DPRAM_HOST_READ_PTR);

        os_if_spin_unlock_irqrestore(&ci->memQLock, irqFlags);

        return MEM_Q_SUCCESS;
    }
    else {
        os_if_spin_unlock_irqrestore(&ci->memQLock, irqFlags);

        return MEM_Q_EMPTY;
    }
}
