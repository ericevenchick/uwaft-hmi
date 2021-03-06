#include <stdio.h>
#include "canhandle.h"

/* 
    int can_init(int channel)
        takes channel to open (note, Kvaser labels channel 0 as channel 1 on their hardware!)
        returns a handle to the bus, or an error
    Notes:
        operates in silent mode, bus is read-only
        bus speed is set in canhandle.h
*/
int can_init(int channel)
{
    int handle;
    int status;
    // this is required for canlib
    canInitializeLibrary();
    
    // try to get a handle
    handle = canOpenChannel(channel, canOPEN_EXCLUSIVE);
    // if an error occured, return it
    if (handle < 0)
        return handle;
    status = canSetBusParams(handle, CANBITRATE, 4, 3, 1, 1, 0); 
    // try to go into silent mode
    status = canSetBusOutputControl(handle, canDRIVER_SILENT);
    if (status < 0)
        return status;
    
    // try to go on bus
    status = canBusOn(handle);
    if (status < 0)
        return status;

    // looks good, return the handle
    return handle;
}
/* 
    int can_get_msg(int handle, char* msg, int wantidlow, wantidhigh)
        takes the handle to the bus to read from, 
		a pointer to a string to store the message in and 
        the range of IDs to respond to to (all others are ignored) 
        returns 0 on no error, or an error. sets msg to a string in format
        id-dlc-byte0-byte1-...-byteCANMSGLENGTH
    Notes:
    TODO:
        error checking? return error values!
*/
int can_get_msg(int handle, char* msg, int wantidlow, int wantidhigh)
{ 
    long id;                    // id of message
    char data[CANMSGLENGTH];    // data bytes of message
    unsigned int dlc;           // Data Length Code 
    unsigned int flags;         
    unsigned long timestamp;
    int i;
    // get the message from the Kvaser hardware 
    if (canRead(handle, &id, data, &dlc, &flags, &timestamp) < 0)
	{
		// return immidately if no message was received
		return -1;	
	}
    // check if we have a message of requested ID
    if (id >= wantidlow && id <= wantidhigh)
    {
        // if so, create a string in the format id-byte0-byte1-...-byteCANMSGLENGTH
        sprintf(msg, "%ld", id);
        for (i=0; i < CANMSGLENGTH; i++)
        {
           sprintf(msg, "%s-%x", msg, data[i]);
        }
        sprintf(msg, "%s\n", msg); 
    }
    return 0; 
}

/*  
    TODO: int can_close(int handle)
    close the can bus nicely.
*/
