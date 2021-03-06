/*
	Name: Juan Diego Gonzalez German
	ID: 1001401837
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

#define CLASSA 0
#define CLASSB 1

/*semaphores for keeping track of permissions to enter office
  *they represent if students from any class can come in
  *if value is greater than 0, that many students may enter
  *if value is zero, students must wait outside until value changes
  *only the professor can change these values*/
sem_t sem_classA;
sem_t sem_classB;

//mutex to protect critical regions
pthread_mutex_t lock;

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0; /*students allowed in since break*/

static int outside[2] = {0, 0}; /*Total number of students from each class waiting to get in*/
static int counter[2] = {0, 0}; /*Total number of students from each class that have been helped in a row*/

typedef struct 
{
	int arrival_time;  // time between the arrival of this student and the previous student
	int question_time; // time the student needs to spend with the professor
	int student_id;
}	student_info;

//Called at beginning of simulation.  
//it takes the name of the file to read
//and the array to fill with student info
static int initialize(student_info *si, char *filename) 
{
	//all counter variables are initialized to zero
	students_in_office = 0;
	classa_inoffice = 0;
	classb_inoffice = 0;
	students_since_break = 0;

	/*Initilaization of semaphores,
	 *students need permission to enter
	 *at start they don't have it, so values are set to zero*/

	sem_init(&sem_classA, 0, 0);
	sem_init(&sem_classB, 0, 0);
	
	/* Read in the data file and initialize the student array */
	FILE *fp;

	if((fp=fopen(filename, "r")) == NULL) 
	{
		printf("Cannot open input file %s for reading.\n", filename);
		exit(1);
	}

	int i = 0;
	while ( (fscanf(fp, "%d%d\n", &(si[i].arrival_time), &(si[i].question_time))!=EOF) &&
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
	sleep(5); //that's a short break!
	assert( students_in_office == 0 );
	students_since_break = 0;
}

//Code for the professor thread. 
//Handles synchronization of student
//behavior
void *professorthread(void *junk) 
{
	printf("The professor arrived and is starting his office hours\n");

	int to_help = CLASSA;

	/* Loop while waiting for students to arrive. */
	while (1) 
	{
		//locking critical region while values are read
		pthread_mutex_lock(&lock);

		//if it is time to take a break, and office is empty, prof does so
		if(students_in_office == 0 && students_since_break == 10)
		{
			take_break();
			students_since_break = 0; //reset counter
			pthread_mutex_unlock(&lock);
		}

		//if office is full, or professor is about to take a break
		//no more students are allowed in, so we loop again
		else if(students_in_office == 3 || students_since_break == 10)
		{
			pthread_mutex_unlock(&lock);
			continue;
		}
		
		pthread_mutex_unlock(&lock);

		//we lock mutex to handle data reading and writing
		pthread_mutex_lock(&lock);

		//if there are class A waiting
		//we let one in, as long as there are no B waiting
		//or we have not helped more than 5 A in a row, even if there are
		if(outside[CLASSA] > 0 && (outside[CLASSB] == 0 || counter[CLASSA] < 5))
		{
			//we make sure they can be allowed in
			if(classb_inoffice == 0 && students_in_office < 3)
			{
				//we signal semaphore and change counters accordingly
				sem_post(&sem_classA);
				//we remove student from queue and add them to both office counters
				//and to the since-break counter
				outside[CLASSA] -= 1;
				students_in_office += 1;
				students_since_break += 1;
				classa_inoffice += 1;
				//we increment counter of class A's in a row by one
				//and reset class B in a row to zero
				counter[CLASSA] += 1;
				counter[CLASSB] = 0;

			}	
			pthread_mutex_unlock(&lock);		
		}

		//if no A's are let in, we repeat the process but now with B's
		else if(outside[CLASSB] > 0 && (outside[CLASSA] == 0 || counter[CLASSB] < 5))
		{
			if(classa_inoffice == 0 && students_in_office < 3)
			{
				sem_post(&sem_classB);
				outside[CLASSB] -= 1;
				students_in_office += 1;
				students_since_break += 1;
				classb_inoffice += 1;
				counter[CLASSB] += 1;
				counter[CLASSA] = 0 ;
			}	
			pthread_mutex_unlock(&lock);
		}

		//if no one is allowed in, we just release the mutex lock and loop again
		else
			pthread_mutex_unlock(&lock);

	}

	pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office. */
void classa_enter() 
{
	//let professor know a student from class A is waiting
	pthread_mutex_lock(&lock);
	outside[CLASSA] += 1;
	pthread_mutex_unlock(&lock);

	//wait until professor gives permission to enter office
	sem_wait(&sem_classA);
}

/* Code executed by a class B student to enter the office. */
void classb_enter() 
{
	//let professor know a student from class B is waiting
	pthread_mutex_lock(&lock);
	outside[CLASSB] += 1;
	pthread_mutex_unlock(&lock);

	//wait until professor gives permission to enter office
	sem_wait(&sem_classB);
}

/* Code executed by a student to simulate the time he spends in the office asking questions */
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
	//substract student from in-office counters
	pthread_mutex_lock(&lock);
	students_in_office -= 1;
	classa_inoffice -= 1;
	pthread_mutex_unlock(&lock);
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
	//substract student from in-office counters
	pthread_mutex_lock(&lock);
	students_in_office -= 1;
	classb_inoffice -= 1;
	pthread_mutex_unlock(&lock);
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
	student_info *s_info = (student_info*)si;

	/* enter office */
	classa_enter();

	printf("Student %d from class A enters the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classb_inoffice == 0 );

	/* ask questions  --- do not make changes to the 3 lines below*/
	printf("Student %d from class A starts asking questions for %d minutes\n", 
	s_info->student_id, s_info->question_time);

	ask_questions(s_info->question_time);

	printf("Student %d from class A finishes asking questions and prepares to leave\n", 
	s_info->student_id);

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
void* classb_student(void *si) 
{
	student_info *s_info = (student_info*)si;

	/* enter office */
	classb_enter();

	printf("Student %d from class B enters the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	assert(classa_inoffice == 0 );

	printf("Student %d from class B starts asking questions for %d minutes\n", 
	s_info->student_id, s_info->question_time);

	ask_questions(s_info->question_time);

	printf("Student %d from class B finishes asking questions and prepares to leave\n", 
	s_info->student_id);

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
 */
int main(int nargs, char **args) 
{
	int i;
	int result;
	int student_type;
	int num_students;
	void* status;
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

		if (student_type == CLASSA)
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
