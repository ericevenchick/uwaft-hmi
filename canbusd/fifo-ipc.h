#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#define FIFOFILE "/home/eric/uwaft-hmi/canrx"

int start_ipc();

int stop_ipc();

int ipcputs(char *msg);

