/*
	Author: Sarker Nadir Afridi Azmi
	CSE3320 - Operating Systems
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE 1024
#define MIN_ARGS 2
#define DELIMITER "\r\n"

// Param1: argc
void CheckCmdArgs(int argc)
{
	if(argc < MIN_ARGS)
	{
		printf("Error: You must provide a datafile as an argument.\n");
		printf("Example: ./pf datafile.txt\n");
		exit(EXIT_FAILURE);
	}
}

// First in first out page replacement algorithm
// Param1: Size of the working set
// Param2: All the page requests
int FIFO_PF(int working_set_size, char page_requests[])
{
	int page_faults = 0;
	int replaced_page_idx = 0;
	
	int working_set[working_set_size];
	
	// Initialize the working set
	int page_req_idx = 0;
	for(page_req_idx = 0; page_req_idx < working_set_size; page_req_idx++)
	{
		working_set[page_req_idx] = -1;
	}
	
	char *token = strtok(page_requests, " ");
	
	page_req_idx = 0;
	
	while(token != NULL)
	{
		int page = atoi(token);
		
		// Put the requested pages into the working set
		if(working_set[page_req_idx] == -1 && page_req_idx < working_set_size)
		{
			working_set[page_req_idx] = page;
			page_faults++;
		}
		else
		{
			// Look for the token value inside of the working set first
			int i = 0;
			for(i = 0; i < working_set_size; i++)
			{
				// If the page exists within the working set, don't search anymore
				// You don't have a page fault
				if(working_set[i] == page)
				{
					break;
				}
			}
			
			// Only executes if there is a page fault
			if(i == working_set_size)
			{
				// Remove the first page entered
				working_set[replaced_page_idx % working_set_size] = page;
				replaced_page_idx++;
				page_faults++;
			}
		}
		
		token = strtok(NULL, " ");
		
		page_req_idx++;
	}
	
	return page_faults;
}

// Initializes an int array with -1
void init_arr(int res[], const int len)
{
	int i = 0;
	for(i = 0; i < len; i++)
		res[i] = -1;
}

// Converts a string to an int array and return the length of the array
int convert_to_int_arr(int res[], char *input)
{
	int length = 0;
	char *token = strtok(input, " ");
	
	while(token != NULL)
	{
		res[length] = atoi(token);
		length++;
		token = strtok(NULL, " ");
	}
	
	return length;
}

// For testing
void print(int res[], const int len)
{
	int i = 0;
	for(i = 0; i < len; i++)
		printf("%d ", res[i]);
	
	printf("\n");
}

// Find the given value inside of the array
bool find_val(const int res[], const int len, const int val_to_find, int *idx_found)
{
	int i = 0;
	for(i = 0; i < len; i++)
	{
		if(res[i] == val_to_find)
		{
			*idx_found = i;
			return true;
		}
	}
	
	*idx_found = -1;
	return false;
}

bool empty(const int working_set[], const int len)
{
	int i = 0;
	for(i = 0; i < len; i++)
	{
		if(working_set[i] == -1)
			return true;
	}
	
	return false;
}

// Optimal page replacement algorithm
// Param1: Size of the working set
// Param2: All the page requests
int Optimal_PF(int working_set_size, char page_requests[])
{
	// Array to store working set
	int working_set[working_set_size];
	int pg_rqst_int[MAX_LINE];
	
	// Initialize the arrays
	init_arr(pg_rqst_int, MAX_LINE);
	init_arr(working_set, working_set_size);
	int array_length = convert_to_int_arr(pg_rqst_int, page_requests);
	
	int page_faults = 0;
	
	// Initialize the working set with pages
	int page_req_idx = 0;
	int ws_idx = 0;
	int idx = 0;
	
	while(empty(working_set, working_set_size))
	{
		if(!find_val(working_set, working_set_size, pg_rqst_int[page_req_idx], &idx))
		{
			working_set[ws_idx] = pg_rqst_int[page_req_idx];
			page_faults++;
			ws_idx++;
		}
		page_req_idx++;
	}
	
	int main_loop_idx = 0;
	int curr_idx = page_req_idx;
	
	for(main_loop_idx = page_req_idx; main_loop_idx < array_length; main_loop_idx++)
	{
		// Look inside of the working set first
		// If the value is within the working set, we do nothing
		if(find_val(working_set, working_set_size, pg_rqst_int[main_loop_idx], &idx))
		{
			curr_idx++;
			continue;
		}
		
		// If not found inside of the working set
		int i = 0;
		int prev_dist = curr_idx;
		int idx_found = -1;
		
		// 2 3 5 6 8 9 7 5 3 1 3 6 3 2 5 2 8 5 4 3 1 1
		for(i = 0; i < working_set_size; i++)
		{
			int j = 0;
			
			for(j = curr_idx; j < array_length; j++)
			{
				if(working_set[i] == pg_rqst_int[j])
				{
					// When we find a page with a greater index value, we save it
					if(j > prev_dist)
					{
						idx_found = i;
						prev_dist = j;
					}
					break;
				}
			}
			
			// If I reach the end of the j-loop, it means we haven't found the page
			// So, replace it
			if(j == array_length)
			{
				idx_found = i;
				break;
			}
		}
		
		// If none of the pages were referenced, remove the first page from the working set
		if(idx_found == -1)
			idx_found = 0;
		
		working_set[idx_found] = pg_rqst_int[main_loop_idx];
		page_faults++;
		curr_idx++;
	}
	
	return page_faults;
}

void init_ref(int ref_counter[], int len)
{
	int i = 0;
	for(i = 0; i < len; i++)
		ref_counter[i] = 0;
}

void increment_count(int ref_counter[], const int idx, const int time)
{
	ref_counter[idx] = time;
}

int find_min(const int ref_counter[], const int len)
{
	int min = ref_counter[0];
	int i = 0;
	int idx = i;
	for(i = 1; i < len; i++)
	{
		if(min > ref_counter[i])
		{
			min = ref_counter[i];
			idx = i;
		}
	}
	return idx;
}

// Least recently used page replacement algorithm
// Param1: Size of the working set
// Param2: All the page requests
int LRU_PF(int working_set_size, char page_requests[])
{
	// Array to store working set
	int working_set[working_set_size];
	int pg_rqst_int[MAX_LINE];
	
	// Reference counter
	int ref[working_set_size];
	
	// Initialize the arrays
	init_arr(pg_rqst_int, MAX_LINE);
	init_arr(working_set, working_set_size);
	init_ref(ref, working_set_size);
	int array_length = convert_to_int_arr(pg_rqst_int, page_requests);
	
	int page_faults = 0;
	
	// Initialize the working set with pages
	int page_req_idx = 0;
	int ws_idx = 0;
	int idx = 0;
	int time = 0;
	
	// Fill up the working set
	// We use time as the variable to determine which was the recently used page
	while(empty(working_set, working_set_size))
	{
		if(!find_val(working_set, working_set_size, pg_rqst_int[page_req_idx], &idx))
		{
			working_set[ws_idx] = pg_rqst_int[page_req_idx];
			increment_count(ref, ws_idx, time);
			page_faults++;
			ws_idx++;
		}
		else if(find_val(working_set, working_set_size, pg_rqst_int[page_req_idx], &idx))
		{
			increment_count(ref, idx, time);
		}
			
		page_req_idx++;
		time++;
	}
	
	int main_loop_idx = 0;
	
	for(main_loop_idx = page_req_idx; main_loop_idx < array_length; main_loop_idx++)
	{
		// Look inside of the working set first
		// If the value is within the working set, we do nothing
		if(find_val(working_set, working_set_size, pg_rqst_int[main_loop_idx], &idx))
		{
			increment_count(ref, idx, time);
			time++;
			continue;
		}
		
		// If I don't find the page, I remove the least recently used from the working set
		int min = find_min(ref, working_set_size);
		working_set[min] = pg_rqst_int[main_loop_idx];
		increment_count(ref, min, time);
		page_faults++;
		time++;
	}
	
	// Least recently used is special
	return page_faults;
}

int find_max(const int ref_counter[], const int len)
{
	int max = ref_counter[0];
	int i = 0;
	int idx = i;
	for(i = 1; i < len; i++)
	{
		if(max < ref_counter[i])
		{
			max = ref_counter[i];
			idx = i;
		}
	}
	return idx;
}

// Most frequently used page replacement algorithm
// Param1: Size of the working set
// Param2: All the page requests
int MFU_PF(int working_set_size, char page_requests[])
{
	// Array to store working set
	int working_set[working_set_size];
	int pg_rqst_int[MAX_LINE];
	
	// Reference counter
	int ref[working_set_size];
	
	// Initialize the arrays
	init_arr(pg_rqst_int, MAX_LINE);
	init_arr(working_set, working_set_size);
	init_ref(ref, working_set_size);
	int array_length = convert_to_int_arr(pg_rqst_int, page_requests);
	
	int page_faults = 0;
	
	// Initialize the working set with pages
	int page_req_idx = 0;
	int ws_idx = 0;
	int idx = 0;
	
	// Fill up the working set
	// We use time as the variable to determine which was the recently used page
	while(empty(working_set, working_set_size))
	{
		if(!find_val(working_set, working_set_size, pg_rqst_int[page_req_idx], &idx))
		{
			working_set[ws_idx] = pg_rqst_int[page_req_idx];
			ref[ws_idx]++;
			page_faults++;
			ws_idx++;
		}
		else if(find_val(working_set, working_set_size, pg_rqst_int[page_req_idx], &idx))
		{
			ref[idx]++;
		}
			
		page_req_idx++;
	}
	
	int main_loop_idx = 0;
	
	for(main_loop_idx = page_req_idx; main_loop_idx < array_length; main_loop_idx++)
	{
		// Look inside of the working set first
		// If the value is within the working set, we do nothing
		if(find_val(working_set, working_set_size, pg_rqst_int[main_loop_idx], &idx))
		{
			ref[idx]++;
			continue;
		}
		
		// If I don't find the page, I remove the least recently used from the working set
		int max = find_max(ref, working_set_size);
		working_set[max] = pg_rqst_int[main_loop_idx];
		ref[max] = 1;
		page_faults++;
	}
	
	return page_faults;
}

int main(int argc, char *argv[])
{
	char *line = NULL;
	size_t line_length = MAX_LINE;
	// Stores duplicates of the reference string
	char temp_str1[MAX_LINE];
	char temp_str2[MAX_LINE];
	char temp_str3[MAX_LINE];
	char temp_str4[MAX_LINE];
	
	FILE *fp;
	
	// Check for valid arguments
	CheckCmdArgs(argc);
	
	// Allocate space for the max allowable line from the text file
	line = (char *)malloc(MAX_LINE);
	
	printf("Opening file %s.\n", argv[1]);
	
	// Get a file handle
	fp = fopen(argv[1], "r");
	
	if(fp)
	{
		while(fgets(line, line_length, fp))
		{
			char *token;
			
			token = strtok(line, " ");
			int working_set_size = atoi(token);
			
			printf("\nWorking set size: %d\n", working_set_size);
			
			// Get the all of the page requests
			token = strtok(NULL, DELIMITER);
			strcpy(temp_str1, token);
			strcpy(temp_str2, token);
			strcpy(temp_str3, token);
			strcpy(temp_str4, token);
			
			char string_format[] = "%-20s%10d\n";
			
			printf(string_format, "Page faults of FIFO:", FIFO_PF(working_set_size, temp_str1));
			printf(string_format, "Page faults of LRU:", LRU_PF(working_set_size, temp_str2));
			printf(string_format, "Page faults of MFU:", MFU_PF(working_set_size, temp_str3));
			printf("%20s%7d\n", "Page faults of Optimal:", Optimal_PF(working_set_size, temp_str4));
		}
		
		free(line);
		fclose(fp);
	}
	else
	{
		printf("Error opening the file. Exiting ...\n");
	}
	
	return EXIT_SUCCESS;
}