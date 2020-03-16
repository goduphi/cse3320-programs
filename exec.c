/*
	This program fork()'s a child and then uses execl to execute ls
	Added the ability to take in user input and execute the ls command
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_CMD_LENGTH 1025

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
		char user_command[MAX_CMD_LENGTH];
		char full_program_path[MAX_CMD_LENGTH + strlen("/bin/")];

		printf("msh> ");
		fgets(user_command, MAX_CMD_LENGTH, stdin);

		user_command[strlen(user_command) - 1] = '\0';

		strncpy(full_program_path, "/bin/", strlen("/bin/"));
		strcat(full_program_path, user_command);
		
		// replace the newly created child process by the specified new program
		execl(full_program_path, user_command, NULL);
		exit(EXIT_SUCCESS);
	}

	// wait until the child terminates
	waitpid(child_pid, &status, 0);
	
	return EXIT_SUCCESS;
}