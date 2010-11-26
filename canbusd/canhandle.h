#ifndef CAN_HANDLE_H
#define CAN_HANDLE_H

#include <canlib.h>
#include <stdio.h>

#define CANMSGLENGTH 8 // length of CAN messages (in bytes)
#define CANBITRATE BAUD_500K        

int can_init(int channel);
int can_get_msg(int handle, char* msg, int wantidlow, int wantidhigh);

#endif
