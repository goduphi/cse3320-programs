/*
	CSE3320 - Operating Systems
	Author: Trevor Jay Bakker, University of Texas at Arlington
*/

// The MIT License (MIT)
//
// Copyright (c) 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

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
		perror("fork failed: ");
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
		int status = 0;

	   	 // Force the parent process to wait until the child process exits
    		waitpid(pid, &status, 0);

		printf("Hello from the parent process!\n");
		fflush(NULL);
	}

	return EXIT_SUCCESS;
}