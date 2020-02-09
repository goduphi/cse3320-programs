#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// The maximum number of characters that a the command line supports
#define MAX_COMMAND_LENGTH 1024

// The maximum number of arguments that the command line supports
#define MAX_NUM_OF_ARGUMENTS 5

// We want to split our command line up into tokens
// so we need to define what delimits our tokens.
// In this case  white space
// will separate the tokens on our command line
#define WHITESPACE_DELIM " \t\n"

#define MAX_NUM_OF_PATHS 4
#define MAX_STRING_LENGTH 15

void change_directory()
{
	
}

int main(void)
{
	// This string will hold the entire string written down into the command line
	char * cmd_str = (char *)malloc(MAX_COMMAND_LENGTH);
	
	char paths_array[MAX_NUM_OF_PATHS][MAX_STRING_LENGTH] = {"/usr/local/bin/", "/usr/bin/", "/bin/", "./"};
	
	while(1)
	{
		// Print out the msh prompt
		printf("msh> ");
		
		// Read the command from the commandline.
		// The maximum number that can be read is specified by MAX_COMMAND_LENGTH
		// The purpose of using the while loop to get user input, rather than a simple
		// fgets() is so that if the user presses enter, it keeps asking for input
		// since fgets returns NULL when there is no input
		while(!fgets(cmd_str, MAX_COMMAND_LENGTH, stdin));
		
		//This 'array of string' will hold all of the commands from the command line
		char *token[MAX_NUM_OF_ARGUMENTS];

		int token_count = 0;
		
		// Pointer to point to the token parsed by strsep
		char *arg_ptr;
		
		char *working_str = strdup(cmd_str);
		
		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char *working_root = working_str;
		
		while((arg_ptr = strsep(&working_root, WHITESPACE_DELIM)) != NULL
				&& (token_count < MAX_NUM_OF_ARGUMENTS))
		{
			token[token_count] = strndup(arg_ptr, strlen(arg_ptr));
			
			// if we hit a "\n", and empty string, we replace it with NULL
			// this comes in handy when we put all of the arguments inside of execl
			if(strlen(token[token_count]) == 0)
				token[token_count] = NULL;
			
			token_count++;
		}
		
		if(token_count > 0)
		{
			/*
				fork() allows for the creation of a child process from the parent process
				where fork() was called.
			*/
			pid_t child_pid = fork();
			
			if(child_pid == -1)
			{
				perror("fork failed: ");
			}
			else if(child_pid == 0)
			{
				int execl_status = 0;
				int path_idx = 0;
				for(path_idx = 0; path_idx < MAX_NUM_OF_ARGUMENTS; path_idx++)
				{
					char path_to_search_cmd[20];
					
					// create the path that will be used as first arg of execl
					// for example, ls is concatenated to /bin/
					strcpy(path_to_search_cmd, paths_array[path_idx]);
					strcat(path_to_search_cmd, token[0]);
					
					execl_status = execl(path_to_search_cmd, token[0], token[1], token[2], token[3], token[4], NULL);
				}
			}
			
			int child_status = 0;
			
			// Wait for the child process to exit
			waitpid(child_pid, &child_status, 0);
		}
		
		free(working_root);
		
	}
	
	free(cmd_str);
	
	return EXIT_SUCCESS;
}