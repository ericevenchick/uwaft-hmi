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
	//open log
	FILE *fLog;
	fLog = fopen(LOGFILE, "a");
	if (fLog == NULL)
		return;
	//write the message
	fputs(msg, fLog);
	fclose(fLog);
	return;

}

//handle signals that want us to end
void handle_signal(int sig)
{
	logs("Caught signal, exiting...\n");	
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
    int canH;               // the can bus handle 
    char errmsg[100];       // stores any error messages
	int status = 0;         // stores any error codes
    char msg[CANMSGLENGTH]; // stores a message received from the bus 
	
    //clear the log
	fclose(fopen(LOGFILE, "w"));
    
	// start up CAN
    logs("Starting CAN...\n");
    if ( (canH = can_init(0)) < 0)
    {
        // error. get the message, report it, and terminate
        canGetErrorText(canH, errmsg, 100);
        logs(errmsg);
        printf("Could not init CAN: %s.\n", errmsg);
		exit(EXIT_FAILURE);
    }

    //start the ipc
    if (start_ipc() < 0)
    {
        logs("Could not start IPC!\n");
        printf("Could not start IPC!\n");
        exit(EXIT_FAILURE);
    }

	// looks like a good start up
    // become a daemon
	if (daemonize() < 0)
	{
        logs("Could not start daemon!\n");
		printf("Could not start daemon!\n");
		exit(EXIT_FAILURE);
	}

	//main loop
    logs("Entering main loop...\n");
    while (1)
	{
        //  get a message (ID 0x1F1 has the data on key position
        //  so i've hard coded it into here for now. This will be
        //  replaced, but it can be used for a nice demo :) )
        status = can_get_msg(canH, msg, 0x1F1);
        if (status < 0)
        {
            canGetErrorText(status, errmsg, 100);
            logs(errmsg);
            printf("Could not read: %s\n", errmsg);
        }
        ipcputs(msg);
        //limit speed
		usleep(100);
	}
	return 0;
}
