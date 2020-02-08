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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
	Author: Trever Jay Bakker

  	Call waitpid to wait on our child, then intentionally SEGFAULT the child
  	so we can use the WIFSIGNALED and WTERMSIG to learn how our child died

	Things that I never knew about:
		
		Definitions from: https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html

		* int WIFSIGNALED(int status) - This macro returns a nonzero value if the child process terminated
		because it received a signal that was not handled.

		* int WTERMSIG(int status) -  If WIFSIGNALED is true of status, this macro returns the signal number
		ofthe signal that terminated the child process.
*/

int main(void)
{
	pid_t child_pid = fork();
	// returns what status the child exited with
	int status;
	
	if(child_pid == -1)
	{
		perror("fork failed: ");
	}
	else if(child_pid == 0)
	{

		// cause intentional SEGFAULT within the child process
		int *p = NULL;
		*p = 1;

		printf("Hello from the child!\n");
		exit(EXIT_SUCCESS);
	}
	
	// wait for the child to exit
	waitpid(child_pid, &status, 0);

	// see if the child was terminated by a signal
	if(WIFSIGNALED(status))
	{
		// print the signal the child was terminated with
		printf("Child returned with status %d.\n", WTERMSIG(status));
	}
	return EXIT_SUCCESS;
}