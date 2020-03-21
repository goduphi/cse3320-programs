/*
	Name: Sarker Nadir Afridi Azmi
	ID: 1001644326
*/

// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
	* There are only 3 seats. What can I use to stop more than 3 students entering?
*/
  
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */
#define MAX_STUDENTS_CHANGE 5 /* Maximum number of students until another class has to be let in */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4

/* TODO */
/* Add your synchronization variables here */

pthread_mutex_t ClassNoStudent = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t BreakMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t StallMtx = PTHREAD_MUTEX_INITIALIZER;

// Controls the entrance of students into the class
pthread_cond_t ClassAMtx = PTHREAD_COND_INITIALIZER;
pthread_cond_t ClassBMtx = PTHREAD_COND_INITIALIZER;
pthread_cond_t BreakCond = PTHREAD_COND_INITIALIZER;

sem_t ClassSeat;
sem_t OnlyOneClass;
sem_t Break;
sem_t StallA;
sem_t StallB;

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */	

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;
static int total_classa;
static int total_classb;

static int AGreaterThanZero;
static int BGreaterThanZero;

typedef struct 
{
	int arrival_time;  // time between the arrival of this student and the previous student
	int question_time; // time the student needs to spend with the professor
	int student_id;
	int class;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
	students_in_office = 0;
	classa_inoffice = 0;
	classb_inoffice = 0;
	students_since_break = 0;
	total_classa = 0;
	total_classb = 0;
	
	AGreaterThanZero = 0;
	BGreaterThanZero = 0;

	/* Initialize your synchronization variables (and 
	* other variables you might use) here
	*/
	
	/*
	 * For keeping track of how many students are using how many seats, I decieded to use a
	   semaphore
	 * Initialize a semaphore with the max number of seats
	 * Call sem_wait inside the classa_enter() and classb_enter() functions to indicate that
	   a 'student' (thread) has entered the class
	 * When sem_wait is called, it is going to decrement the value of the semaphore
	 * Call sem_post() to increment semaphore value to indicate the 'student' (thread) has left
	   the class
	 */
	int semRetVal = sem_init(&ClassSeat, 0, MAX_SEATS);
	int semRetValBreak = sem_init(&Break, 0, professor_LIMIT);
	int semRetValStallA = sem_init(&StallA, 0, MAX_STUDENTS_CHANGE);
	int semRetValStallB = sem_init(&StallB, 0, MAX_STUDENTS_CHANGE);
	
	if(semRetVal || semRetValBreak || semRetValStallA || semRetValStallB)
		perror("sem_init failed");

	/* Read in the data file and initialize the student array */
	FILE *fp;

	if((fp=fopen(filename, "r")) == NULL) 
	{
		printf("Cannot open input file %s for reading.\n", filename);
		exit(1);
	}

	int i = 0;
	while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
		   i < MAX_STUDENTS ) 
	{
		i++;
	}

	fclose(fp);
	return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break()
{
	printf("The professor is taking a break now.\n");
	sleep(5);
	assert( students_in_office == 0 );
	students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
	printf("The professor arrived and is starting his office hours\n");

	/* Loop while waiting for students to arrive. */
	while (1) 
	{

		/* TODO */
		/* Add code here to handle the student's request.             */
		/* Currently the body of the loop is empty. There's           */
		/* no communication between professor and students, i.e. all  */
		/* students are admitted without regard of the number         */ 
		/* of available seats, which class a student is in,           */
		/* and whether the professor needs a break. You need to add   */
		/* all of this.
		*/
		
		/*
			This section of code is responsible for controlling the professor break time
			Take a break, and then increment the semaphore to the max number students until
			the professor has to take a break again
		*/
		
		pthread_mutex_lock(&BreakMtx);
		
		if(students_since_break == professor_LIMIT && students_in_office == 0)
		{
			take_break();
			int counter = 0;
			while(counter < professor_LIMIT)
			{
				sem_post(&Break);
				counter++;
			}
		}
		
		pthread_mutex_unlock(&BreakMtx);
		
		/*
			This section of code is reponsible for signaling which class should be allowed in
		*/
		
		pthread_mutex_lock(&ClassNoStudent);
		//printf("S = %d, A = %d, B = %d, TA = %d, TB = %d\n", students_in_office, classa_inoffice, classb_inoffice, total_classa, total_classb);
		
		// If Class A == 0, let Class B in
		if(classa_inoffice == 0 && classb_inoffice > 0)
		{
			pthread_cond_broadcast(&ClassAMtx);
		}
		// If Class B == 0, let Class A in
		else if(classb_inoffice == 0 && classa_inoffice > 0)
		{
			pthread_cond_broadcast(&ClassBMtx);
		}
		// If there are no students in the class, let the thread who goes in first
		// be the thread to determine the sequence in which to let the other classes enter
		else if(classa_inoffice == 0 && classb_inoffice == 0)
		{
			pthread_cond_broadcast(&ClassAMtx);
			pthread_cond_broadcast(&ClassBMtx);
		}
		
		pthread_mutex_unlock(&ClassNoStudent);
	}
	pthread_exit(NULL);
}

/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{
	/* TODO */
	/* Request permission to enter the office.  You might also want to add  */
	/* synchronization for the simulations variables below                  */
	/*  YOUR CODE HERE.          
	*/
	
	sem_wait(&StallA);
	sem_wait(&Break);
	sem_wait(&ClassSeat);
	
	/*
		This section of code is reponsible for allowing only one class to enter
		While the number of students in Class A is zero and there are students in
		Class B, make Class A wait.
	*/
	
	pthread_mutex_lock(&ClassNoStudent);

	BGreaterThanZero = 0;

	while(classb_inoffice > 0 && classa_inoffice == 0)
	{
		pthread_cond_wait(&ClassBMtx, &ClassNoStudent);
	}
	
	students_in_office += 1;
	classa_inoffice += 1;
	total_classa += 1;
	
	pthread_mutex_unlock(&ClassNoStudent);
	
	pthread_mutex_lock(&BreakMtx);
	
	students_since_break += 1;
	
	pthread_mutex_unlock(&BreakMtx);
}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{

	/* TODO */
	/* Request permission to enter the office.  You might also want to add  */
	/* synchronization for the simulations variables below                  */
	/*  YOUR CODE HERE.                                                     */

	sem_wait(&StallB);
	sem_wait(&Break);
	sem_wait(&ClassSeat);
	
	/*
		This section of code is reponsible for allowing only one class to enter
		While the number of students in Class B is zero and there are students in
		Class A, make Class B wait.
	*/
	
	pthread_mutex_lock(&ClassNoStudent);
	
	AGreaterThanZero = 0;
	
	while(classa_inoffice > 0 && classb_inoffice == 0)
	{
		pthread_cond_wait(&ClassAMtx, &ClassNoStudent);
	}
	
	students_in_office += 1;
	classb_inoffice += 1;
	total_classb += 1;
	
	pthread_mutex_unlock(&ClassNoStudent);
	
	pthread_mutex_lock(&BreakMtx);
	
	students_since_break += 1;
	
	pthread_mutex_unlock(&BreakMtx);
	
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
	sleep(t);
}

/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
	/* 
	*  TODO
	*  YOUR CODE HERE. 
	*/
	
	pthread_mutex_lock(&ClassNoStudent);
	
	students_in_office -= 1;
	classa_inoffice -= 1;
	
	/*
		This section of code deals with the professor being fair where, if 5 students
		from one class enters, the current class (A) is blocked and the other class (B)
		is let in.
	*/
	
	int SemBVal;
	sem_getvalue(&StallB, &SemBVal);
	
	// This if statment is dependent on the other class, Class B being present
	// If there are at least 1 student from Class B, when Class A completely leaves
	// there is room for Class B, so let Class B enter
	if(total_classb > 0)
	{
		BGreaterThanZero = 1;
		int counter = 0;
		// Set the value of the semaphore to the max value, 5, again
		while(counter < MAX_STUDENTS_CHANGE && counter < total_classb)
		{
			sem_post(&StallB);
			counter++;
		}
		total_classb = 0;
	}
	// If the value of semaphore B is 5, then no students of Class B exists
	// So, reset Class A semaphore
	else if(SemBVal == MAX_STUDENTS_CHANGE && BGreaterThanZero == 0)
	{
		int counter = 0;
		while(counter < MAX_STUDENTS_CHANGE && counter < total_classa)
		{
			sem_post(&StallA);
			counter++;
		}
	}
	
	pthread_mutex_unlock(&ClassNoStudent);
	
	sem_post(&ClassSeat);
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
	/* 
	* TODO
	* YOUR CODE HERE. 
	*/
	pthread_mutex_lock(&ClassNoStudent);
	
	students_in_office -= 1;
	classb_inoffice -= 1;
	
	/*
		This section of code deals with the professor being fair where, if 5 students
		from one class enters, the current class (B) is blocked and the other class (A)
		is let in.
	*/
	
	int SemAVal;
	sem_getvalue(&StallA, &SemAVal);
	
	// This if statment is dependent on the other class, Class A being present
	// If there are at least 1 student from Class A, when Class B completely leaves
	// there is room for Class A, so let Class A enter
	if(total_classa > 0)
	{
		AGreaterThanZero = 1;
		int counter = 0;
		// Set the value of the semaphore to the max value, 5, again
		while(counter < MAX_STUDENTS_CHANGE && counter < total_classa)
		{
			sem_post(&StallA);
			counter++;
		}
		total_classa = 0;
	}
	// If the value of semaphore A is 5, then no students of Class A exists
	// So, reset Class B semaphore
	else if(SemAVal == MAX_STUDENTS_CHANGE && AGreaterThanZero == 0)
	{
		int counter = 0;
		while(counter < MAX_STUDENTS_CHANGE && counter < total_classb)
		{
			sem_post(&StallB);
			counter++;
		}
	}
	
	pthread_mutex_unlock(&ClassNoStudent);
	
	sem_post(&ClassSeat);
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void * classa_student(void *si) 
{
	student_info *s_info = (student_info*)si;

	/* enter office */
	classa_enter();

	printf("Student %d from class A enters the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classb_inoffice == 0);
	

	/* ask questions  --- do not make changes to the 3 lines below*/
	printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
	ask_questions(s_info->question_time);
	printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

	/* leave office */
	classa_leave();  

	printf("Student %d from class A leaves the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS); 
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	
	pthread_exit(NULL);
}

/* Main code for class B student threads.
* You do not need to change anything here, but you can add
* debug statements to help you during development/debugging.
*/
void * classb_student(void *si) 
{
	student_info *s_info = (student_info*)si;

	/* enter office */
	classb_enter();

	printf("Student %d from class B enters the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	assert(classa_inoffice == 0 );

	printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
	ask_questions(s_info->question_time);
	printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

	/* leave office */
	classb_leave();        

	printf("Student %d from class B leaves the office\n", s_info->student_id);
	
	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

	pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args) 
{
	int i;
	int result;
	int student_type;
	int num_students;
	void *status;
	pthread_t professor_tid;
	pthread_t student_tid[MAX_STUDENTS];
	student_info s_info[MAX_STUDENTS];

	if (nargs != 2) 
	{
		printf("Usage: officehour <name of inputfile>\n");
		return EINVAL;
	}

	num_students = initialize(s_info, args[1]);
	if (num_students > MAX_STUDENTS || num_students <= 0) 
	{
		printf("Error:  Bad number of student threads. "
			   "Maybe there was a problem with your input file?\n");
		return 1;
	}

	printf("Starting officehour simulation with %d students ...\n",
	num_students);

	result = pthread_create(&professor_tid, NULL, professorthread, NULL);

	if (result) 
	{
		printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
		exit(1);
	}

	for (i=0; i < num_students; i++) 
	{
		s_info[i].student_id = i;
		sleep(s_info[i].arrival_time);
					
		student_type = random() % 2;

		if (s_info[i].class == CLASSA)
		{
		  result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
		}
		else // student_type == CLASSB
		{
		  result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
		}
		if (result) 
		{
		  printf("officehour: thread_fork failed for student %d: %s\n", 
				i, strerror(result));
		  exit(1);
		}
	}
	
	/* wait for all student threads to finish */
	for (i = 0; i < num_students; i++) 
	{
		pthread_join(student_tid[i], &status);
	}

	/* tell the professor to finish. */
	pthread_cancel(professor_tid);

	printf("Office hour simulation done.\n");

	return 0;
}
