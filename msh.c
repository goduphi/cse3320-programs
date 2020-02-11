/*
	Author: Sarker Nadir Afridi Azmi
	
	Resources used: https://github.com/CSE3320/Shell-Assignment
*/

// The only thing I know is that defining this allows me to used
// the get_current_dir_name function
#define _GNU_SOURCE

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

#define CHANGE_DIRECTORY "cd"
#define LAST_CMD "!"

#define CURRENT_DIR "./"
#define PARENT_DIR get_current_dir_name()

typedef struct Queue
{
	char command[MAX_COMMAND_LENGTH];
	struct Queue *tail_next;
} Queue;

void enQueue(Queue **head, Queue **tail, char cmd[])
{
	Queue * new_node = (Queue *)malloc(sizeof(Queue));
	strcpy(new_node->command, cmd);
	new_node->tail_next = NULL;
	
	if(*head == NULL)
	{
		*head = *tail = new_node;
	}
	else
	{
		(*tail)->tail_next = new_node;
		(*tail) = new_node;
	}
}

void deQueue(Queue **head)
{
	Queue *temp = (*head)->tail_next;
	
	if(*head == NULL)
	{
		printf("No commands in history");
	}
	else
	{
		free(temp);
		*head = temp;
	}
}

/*
void list_commands(Queue *head)
{
	int cmd_counter = 0;
	
	if(head->tail_next == NULL)
	{
		printf("\n%s\n\n", head->command);
		return;
	}
	else
	{
		print_queue(head->tail_next);
	}
}
*/

void change_directory(char **cmd)
{
	char temp_path[MAX_COMMAND_LENGTH];
	strcpy(temp_path, PARENT_DIR);
	strcpy(temp_path, CURRENT_DIR);
	
	int token_idx = 1;
	while(cmd[token_idx] != NULL)
	{
		strcat(temp_path, cmd[token_idx]);
		token_idx++;
	}
	
	chdir(temp_path);
}

int main(void)
{
	// This string will hold the entire string written down into the command line
	char * cmd_str = (char *)malloc(MAX_COMMAND_LENGTH);
	
	char paths_array[MAX_NUM_OF_PATHS][MAX_STRING_LENGTH] = {"/usr/local/bin/",
															 "/usr/bin/",
															 "/bin/", "./"};
	
	Queue *head = NULL, *tail = NULL;
		   
	printf("\n\033[01;33m**********************************************\n"
		   "Welcome to MAV SHELL, the Shell of your dreams\n"
		   "**********************************************\033[0m\n\n");
	
	while(1)
	{
		// Print out the msh prompt
		printf("\033[01;33mmsh>\033[0m ");
		printf("\033[1;32m");
		// Read the command from the commandline.
		// The maximum number that can be read is specified by MAX_COMMAND_LENGTH
		// The purpose of using the while loop to get user input, rather than a simple
		// fgets() is so that if the user presses enter, it keeps asking for input
		// since fgets returns NULL when there is no input
		while(!fgets(cmd_str, MAX_COMMAND_LENGTH, stdin));
		
		if(strcmp(cmd_str, "\n") == 0)
			continue;
		
		enQueue(&head, &tail, cmd_str);
		
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
			
			// if we hit a "\n", after which is an empty string, we replace it with NULL
			// this comes in handy when we put all of the arguments inside of execl
			if(strlen(token[token_count]) == 0)
				token[token_count] = NULL;
			
			token_count++;
		}
		
		if(strcmp(token[0], CHANGE_DIRECTORY) == 0)
		{
			change_directory(token);
		}
		/*
		else if(strcmp(token[0], CHANGE_DIRECTORY) == 0 &&
				strcmp(token[1], "..") == 0)
		{
			chdir(PARENT_DIR);
		}*/
		else if(strcmp(token[0], LAST_CMD) == 0)
		{
			printf("Insert last command!");
		}
		else if(token_count > 0)
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
					
					execl_status = execl(path_to_search_cmd, token[0], token[1],
															 token[2], token[3], token[4], NULL);
				}
				
				if(execl_status == -1)
					printf("Invalid command: %s\n", cmd_str);
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