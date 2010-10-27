/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

/*
 * Kvaser Linux Canlib
 * List available channels
 */

#include "canlib.h"
#include <stdio.h>

 /*
 Lists available CAN channels
 */

int main(int argc, char* argv[])
{
    int chanCount = 0;
    int stat, i;
    char name[256];
    unsigned int ean[2], fw[2], serial[2];
    
    stat = canGetNumberOfChannels(&chanCount);
    if (stat < 0);
    if (chanCount < 0 || chanCount > 64) {
        printf("ChannelCount = %d but I don't believe it.\n", chanCount);
        exit(1);
    }
    else {
        if (chanCount == 1)
            printf("Found %d channel.\n", chanCount);
        else
            printf("Found %d channels.\n", chanCount);
    }

    for (i = 0; i < chanCount; i++) {
        stat = canGetChannelData(i, canCHANNELDATA_CHANNEL_NAME,
                                 &name, sizeof(name));
        if (stat < 0) {
            printf("Error in canGetChannelData - CHANNEL_NAME\n");
            exit(1);
        }

        stat = canGetChannelData(i, canCHANNELDATA_CARD_UPC_NO,
                                 &ean, sizeof(ean));
        if (stat < 0) {
            printf("Error in canGetChannelData - CARD_UPC_NO\n");
            exit(1);
        }

        stat = canGetChannelData(i, canCHANNELDATA_CARD_SERIAL_NO,
                                 &serial, sizeof(serial));
        if (stat < 0) {
            printf("Error in canGetChannelData - CARD_SERIAL_NO\n");
            exit(1);
        }

        stat = canGetChannelData(i, canCHANNELDATA_CARD_FIRMWARE_REV,
                                 &fw, sizeof(fw));
        if (stat < 0) {
            printf("Error in canGetChannelData - CARD_FIRMWARE_REV\n");
            exit(1);
        }

        printf("channel %d = %s, %x-%04x-%05x-%x, (%d)%d, %d.%d.%d.%d\n",
               i, name,
               ean[1] >> 8, ((ean[1] & 0xff) << 8) | ((ean[0] >> 24) & 0xff),
               (ean[0] >> 4) & 0xfffff, ean[0] & 0x0f,
               serial[1], serial[0],
               fw[1] & 0xffff, fw[1] >> 16, fw[0] & 0xffff, fw[0] >> 16);
    }    

    return 0;
}

