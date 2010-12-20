#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

#include "canhandle.h"
#include "fifo-ipc.h"

#define LOGFILE "/home/eric/uwaft-hmi/log.txt"

//log string to file
void logs(char *msg)
{
	char line[100];
	
	//open log
	FILE *fLog;
	fLog = fopen(LOGFILE, "a");
	if (fLog == NULL)
		return;

	//write the message
	sprintf(line, "%s\n", msg);
	fputs(line, fLog);
	fclose(fLog);
	return;

}

//handle signals that want us to end
void handle_signal(int sig)
{
	logs("Caught signal, exiting...");	
    //stop the IPC
    stop_ipc();
	exit(0);
}

//setup a daemon
int daemonize()
{

	//fork off the parent process
	pid_t pid, sid;
	pid = fork();

	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}		
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}

	//set umask
	umask(0);
	
	//get a SID from the kernel
	sid = setsid();
	if (sid < 0)
		return -1;	
	//change to root directory
	if ((chdir("/")) < 0)
		return -1;
	//close standard in/out/error
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	//listen to signals	
	signal(SIGTERM, handle_signal);
	
	//success!
	return 0;
}

int main()
{
    int canH1;              // the can bus 1 handle 
    int canH2;              // the can bus 2 handle 
    char errmsg[100];       // stores any error messages
	int status = 0;         // stores any error codes
    char msg[CANMSGLENGTH]; // stores a message received from the bus 
	int  can1en;			// is channel 1 enabled?
	int  can2en;			// is channel 2 enabled?

    // clear the log
	fclose(fopen(LOGFILE, "w"));
	
	// start up CAN
	can1en = 1;
    logs("Starting CAN 1...");
    if ( (canH1 = can_init(0)) < 0)
    {
        // error. get the message, report it
        canGetErrorText(canH1, errmsg, 100);
        logs(errmsg);
        printf("Could not init CAN 1: %s.\n", errmsg);
		can1en = 0;
    }
	
	can2en = 1;
    logs("Starting CAN 2...");
    if ( (canH2 = can_init(1)) < 0)
    {
        // error. get the message, report it
        canGetErrorText(canH2, errmsg, 100);
        logs(errmsg);
        printf("Could not init CAN 2: %s.\n", errmsg);
		can2en = 0;
    }   
	
	// if neither channel started, exit
	if (!can1en || !can2en)
	{
		printf("Could not open any CAN channels!\n");
		exit(EXIT_FAILURE);
	}

	// start the ipc
    if (start_ipc() < 0)
    {
        logs("Could not start IPC!");
        printf("Could not start IPC!\n");
        exit(EXIT_FAILURE);
    }

	// looks like a good start up
    // become a daemon
	if (daemonize() < 0)
	{
        logs("Could not start daemon!");
		printf("Could not start daemon!\n");
		exit(EXIT_FAILURE);
	}

	//main loop
    logs("Entering main loop...");
    while (1)
	{
        memset(msg, 0, CANMSGLENGTH); 
		// Get a message from bus 1 in range 0xA0 - 0xA0 
        if (can1en == 1)
		{
			status = can_get_msg(canH1, msg, 0xA0, 0x1F1);
			if (status == 0)
			{
				// add the bus number to the message
				ipcputs("1-");
        		ipcputs(msg);
			}
		}
		// Get a message from bus 2 in range 0xA0 - 0x1F1
		if (can2en == 1)
		{
			status = can_get_msg(canH2, msg, 0xA0, 0x1F1);
			if (status == 0)
			{
				// add the bus number to the message
				ipcputs("2-");
        		ipcputs(msg);
			}
		}
	}
	return 0;
}
