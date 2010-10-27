#include "fifo-ipc.h"
#define FIFOFILE "/home/eric/uwaft-hmi/canmsg"

//Initialize the IPC

int start_ipc()
{
    //make a FIFO with permissions:
    // RW for user, RW for group, R for others
    return mkfifo(FIFOFILE, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP |
            S_IROTH);
}

//Destroy the IPC
int stop_ipc()
{
    return remove(FIFOFILE);
}

int ipcputs(char *msg)
{
    FILE *fp;
    //open the fifo, return on error
    fp = fopen(FIFOFILE, "a");
    if (fp == 0)
        return -1;
    //write to the fifo
    fputs(msg, fp);
    fclose(fp);
    //success!
    return 0;
}
