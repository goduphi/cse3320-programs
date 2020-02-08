/*
	This program fork()'s a child and then uses execl to execute ls
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	pid_t child_pid = fork();

	// returns what status the child exited with
	int status;

	if(child_pid == -1)
	{
		perror("fork failed: ");
		exit(EXIT_FAILURE);
	}
	else if(child_pid == 0)
	{
		// replace the newly created child process by the specified new program
		execl("/bin/ls", "ls", NULL);
		exit(EXIT_FAILURE);
	}

	// wait until the child terminates
	waitpid(child_pid, &status, 0);

	return EXIT_SUCCESS;
}