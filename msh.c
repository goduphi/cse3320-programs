/*
	Author: Sarker Nadir Afridi Azmi
	ID: 1001644326
	
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
#include <stdbool.h>
#include <ctype.h>

// The maximum number of characters that a the command line supports
#define MAX_COMMAND_LENGTH 1024

// The maximum number of arguments that the command line supports
#define MAX_NUM_OF_ARGUMENTS 12

#define MAX_NUM_COMMANDS 14

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

/*
	This struct defines the queue
*/

typedef struct Queue
{
	char command[MAX_COMMAND_LENGTH];
	struct Queue *next_ptr;
} Queue;

/*
	This function frees the entire queue.
	Param 1: Pass in queue head.
	Param 2: Pass in queue tail.
	Param 3: Pass in the command to be stored.
*/

bool enQueue(Queue **head, Queue **tail, char cmd[])
{
	Queue * new_node = (Queue *)malloc(sizeof(Queue));
	strcpy(new_node->command, cmd);
	new_node->next_ptr = NULL;
	
	if(*head == NULL)
	{
		*head = *tail = new_node;
	}
	else
	{
		(*tail)->next_ptr = new_node;
		(*tail) = new_node;
	}
	
	return true;
}

/*
	This function checks to see whether the queue is empty or not!
	Param: Pass in queue head.
*/

bool empty(Queue *head)
{
	if(head == NULL)
		return true;
	else
		return false;
}

/*
	This function frees the first node
	Param: Pass in queue head.
*/

void deQueue(Queue **head)
{
	Queue *temp = (*head)->next_ptr;
	
	if(*head == NULL)
	{
		printf("No commands in history");
	}
	else
	{
		free(*head);
		*head = temp;
	}
}

/*
	This function frees the entire queue.
	Param: Pass in queue head.
*/

void free_queue(Queue **head)
{
	while(*head != NULL)
		deQueue(head);
}

/*
	This function prints all of the contents inside of the queue.
	Param: Pass in queue head.
*/

void list_commands(Queue *head)
{
	int counter = 0;
	Queue * temp = head;
	while(temp != NULL)
	{
		printf("%d. %s\n", counter, temp->command);
		temp = temp->next_ptr;
		counter++;
	}
}

/*
	This function allows the directory to change.
	Param: Pass in a string that contains the path to the desired directory.
*/

void change_directory(char **cmd)
{
	char temp_path[MAX_COMMAND_LENGTH];
	strcpy(temp_path, CURRENT_DIR);
	
	int token_idx = 1;
	while(cmd[token_idx] != NULL)
	{
		// This check is to help change to a directory that has a space in its namespace
		// Let's say Homework 1
		if((cmd[2] != NULL) && ((token_idx % 2) == 0))
		{
			strcat(temp_path, " ");
		}
		strcat(temp_path, cmd[token_idx]);
		token_idx++;
	}
	
	chdir(temp_path);
}

/*
	This function executes the commands based on its index inside of the queue.
	Param 1: Command index.
	Param 2: History queue.
*/

char * execute_specified_command(int command_number, Queue *head)
{
	Queue *temp = head;
	int counter = 0;
	if(head == NULL)
	{
		printf("No commands have been executed yet!\n");
		return "\n";
	}
	
	while(temp != NULL)
	{
		if(counter == command_number)
		{
			counter = 0;
			return temp->command;
		}
		temp = temp->next_ptr;
		counter++;
	}

	return NULL;
}

/*
	This function takes a string and converts it into lower case.
	Param 1: Char array to be converted.
	Praam 2: Char array where the converted string is to be stored.
*/

void str_to_lower(char input_string[],  char output_string[])
{
	int char_idx = 0;
	for(char_idx = 0; char_idx < strlen(input_string); char_idx++)
	{
		if(input_string[char_idx] == '\n')
		{
			output_string[char_idx] = '\0';
		}
		else
		{
			output_string[char_idx] = tolower(input_string[char_idx]);
		}
	}
	return;
}

// This function is only meant to be use if the '!' command is used
// Takes whatever the integer value is right after !, let's say for !4, the 4
// Converts it into an integer and returns it
int parse_command_number(char *cmd_str)
{
	int command_number;
	sscanf(cmd_str, "%*c%d", &command_number);
	return command_number;
}

/*
	This function removes the \n from the passed in string.
*/

void remove_slashn(char cmd[])
{
	if((cmd[strlen(cmd) - 1]) == '\n')
		cmd[strlen(cmd) - 1] = '\0';
}

/*
	This function frees all of the allocated memory for the command line arguments.
	Param: Pass in array of pointers to char of command line arguments.
*/

void free_token_arr(char *token[])
{
	int token_idx = 0;
	for(token_idx = 0; token[token_idx] != NULL && token_idx < MAX_NUM_OF_ARGUMENTS; token_idx++)
	{
		free(token[token_idx]);
	}
}

int main(void)
{
	// This string will hold the entire string written down into the command line
	char * cmd_str = (char *)malloc(MAX_COMMAND_LENGTH);
	
	char paths_array[MAX_NUM_OF_PATHS][MAX_STRING_LENGTH] = {"/usr/local/bin/",
															 "/usr/bin/",
															 "/bin/", "./"};
	
	// holds commands history information
	Queue *history_qhead = NULL, *history_qtail = NULL;
	Queue *pid_qhead = NULL, *pid_qtail = NULL;
		   
	printf("\n\033[01;33m**********************************************\n"
		   "Welcome to MAV SHELL, the Shell of your dreams\n"
		   "**********************************************\033[0m\n\n");
	
	int cmd_counter = 0;
	int pid_counter = 0;
	bool enQueued = false;
	
	while(1)
	{
		// Print out the msh prompt
		printf("\033[01;33mmsh>\033[0m ");
		// Read the command from the commandline.
		// The maximum number that can be read is specified by MAX_COMMAND_LENGTH
		// The purpose of using the while loop to get user input, rather than a simple
		// fgets() is so that if the user presses enter, it keeps asking for input
		// since fgets returns NULL when there is no input
		
		while(!fgets(cmd_str, MAX_COMMAND_LENGTH, stdin));

		char exit_quit[strlen(cmd_str)];
		
		str_to_lower(cmd_str, exit_quit);
		
		if(strcmp(exit_quit, "quit") == 0 || strcmp(exit_quit, "exit") == 0)
		{
			exit(0);
		}
		
		if(cmd_str[0] == '!')
		{
			// call function to find node with command and set cmd_str to that command for processing
			char * cmd_to_execute = execute_specified_command(parse_command_number(cmd_str), history_qhead);
			if(cmd_to_execute == NULL)
			{
				printf("Command not in history.\n");
				continue;
			}
			else
			{
				strcpy(cmd_str, cmd_to_execute);
			}
		}
		
		if(strcmp(cmd_str, "\n") == 0)
		{
			continue;
		}
		
		if(cmd_counter > MAX_NUM_COMMANDS)
		{
			deQueue(&history_qhead);
		}
		
		if((pid_counter > 1 + MAX_NUM_COMMANDS) && enQueued)
		{
			enQueued = false;
			deQueue(&pid_qhead);
		}
		
		//This 'array of string' will hold all of the commands from the command line
		char *token[MAX_NUM_OF_ARGUMENTS];

		int token_count = 0;
		
		// Pointer to point to the token parsed by strsep
		char *arg_ptr;
		
		char *working_str = strdup(cmd_str);
		
		remove_slashn(cmd_str);
		
		if(enQueue(&history_qhead, &history_qtail, cmd_str))
		{
			cmd_counter++;
		}
		
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
			{
				free(token[token_count]);
				token[token_count] = NULL;
			}
			
			token_count++;
		}
		
		free(working_str);
		free(working_root);
		
		if(strcmp(token[0], CHANGE_DIRECTORY) == 0)
		{
			change_directory(token);
		}
		else if(strcmp(token[0], "history") == 0)
		{
			if(!empty(history_qhead))
				list_commands(history_qhead);
			else
				printf("No commands have been executed yet.\n");
		}
		else if(strcmp(token[0], "showpids") == 0)
		{
			if(!empty(pid_qhead))
				list_commands(pid_qhead);
			else
				printf("No processes have been spawned yet.\n");
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
															 token[2], token[3],
															 token[4], token[5],
															 token[6], token[7],
															 token[8], token[9],
															 token[10], NULL);
				}
				
				if(execl_status == -1)
				{
					printf("%s: Command not found\n", cmd_str);
				}
			}
			// enQueue pid info inside of the parent because child has no idea about
			// pid returned to parent
			else
			{
				char pid_buffer[20];
				sprintf(pid_buffer, "%d", (int)child_pid);
				if(enQueue(&pid_qhead, &pid_qtail, pid_buffer))
				{
					enQueued = true;
					pid_counter++;
				}
			}
			
			int child_status = 0;
			
			// Wait for the child process to exit
			waitpid(child_pid, &child_status, 0);
		}
		
		free_token_arr(token);
		
	}
	
	// Do a bit of house keeping by freeing all malloced data
	free(cmd_str);
	free_queue(&history_qhead);
	free_queue(&pid_qhead);
	
	return EXIT_SUCCESS;
}
