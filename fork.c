/*
	CSE3320 - Operating Systems
	Author: Trevor Jay Bakker, University of Texas at Arlington
*/

#include <stdio.h>
// includes fork()
#include <unistd.h>
// includes pid_t
#include <sys/types.h>
#include <stdlib.h>

int main(void)
{
	/*
		fork() allows for the creation of a child process from the parent process
		where fork() was called.
	*/
	pid_t pid = fork();

	if(pid == -1)
	{
		// When fork() return -1, an error has occured
		perror("fork failed");
		/*
			If the stream argument is NULL for fflush(), it flushes all open output
       			streams.
		*/
		fflush(NULL);
		exit(EXIT_FAILURE);
	}
	else if(pid == 0)
	{	
		// When fork() returns 0, we are in the child
		printf("Hello from the child process!\n");
		fflush(NULL);
		exit(EXIT_SUCCESS);
	}
	else
	{
		// When fork() return a positive number, we are in the parent process
		// and the return value is the PID of the new child process
		printf("Hello from the parent process!\n");
		fflush(NULL);
	}

	return EXIT_SUCCESS;
}