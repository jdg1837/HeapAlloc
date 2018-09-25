/*
	Name: Juan Diego Gonzalez German
	ID: 1001401837
*/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                			// so we need to define what delimits our tokens.
                               				 // In this case  white space
                                			// will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11     // Mav shell only supports one command plus 10 arguments at a time

//#define PATH ":/usr/local/bin:/usr/bin:/bin"

void parse_input(char**, char*);
void update_history(char**, char*, int);
void update_pids(int[15], int, int);
void handle_signal();

int main()
{
	//multi-use counter variable
	int i;

	//Allocate memory to get input from the user
	char* cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

	//set up array of ints to store up to 15 PIDs, initialize all to 0.
	//initialize pid counter to 0
	int pids[15];
	for(i = 0; i < 15; i++)
		pids[i] = 0;
	int pid_ctr = 0;

	//we allocate enough memory to hold a15 input strings
	//initialize history counter to 0;
	char* history[15];
	for(i = 0; i < 15; i++)
		history[i] = (char*)malloc(MAX_COMMAND_SIZE);
	int hist_ctr = 0;
//NEW CODE
	struct sigaction act;
	memset(&act, '\0', sizeof(act));
	act.sa_handler = &handle_signal;
	sigfillset(&act.sa_mask);
  	if (sigaction(SIGINT , &act , NULL) < 0)
	{
		perror ("sigaction: ");
		return 1;
  	}
  	if (sigaction(SIGTSTP , &act , NULL) < 0)
	{
		perror ("sigaction: ");
		return 1;
  	}

//NEW Code ends here

	while( 1 )
	{

	// Print out the msh prompt
	printf ("msh> ");

	// Read the command from the commandline.
	// Maximum command that will be read is MAX_COMMAND_SIZE
	// This while command will wait here until the user
	// inputs something since fgets returns NULL when there
	// is no input
	while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

	//Check if input is a blank line; if it is, we loop and ask for input again
	int is_valid = 0;

	for(i = 0; i < strlen(cmd_str) -1; i++)
		if(cmd_str[i] != ' ' && cmd_str[i] != '\t' && cmd_str[i] != '\n')
			is_valid = 1;

	if(is_valid == 0)
		continue;

	//if input is not void, we store it in history array
	//and we update hist_ctr to the next stop to be filled
	update_history(history, cmd_str, hist_ctr);
	hist_ctr++;

	/* Parse input */
	char* token[MAX_NUM_ARGUMENTS];

	parse_input(token, cmd_str);

	// If cmd is to quit shell, we exit
	if(strcmp(token[0],"quit") == 0 || strcmp(token[0], "exit") ==0)
		return 0;

	// If cmd is tp change directory, we do so from main thread
	else if(strcmp(token[0],"cd") == 0)
	{
		if(chdir(token[1]) == -1)
			printf("Directory could not be changed. Please verify path.\n");
	}

	else if(strcmp(token[0], "listpids") == 0)
	{
		int j = 14;

		if(pid_ctr < 15)
			j = pid_ctr;

		for(i = 0; i < j; i++)
			printf("%d: %d\n", i, pids[i]);
	}

	else if(strcmp(token[0], "history") == 0)
	{
		int j = 14;

		if(hist_ctr < 15)
			j = hist_ctr;

		for(i = 0; i < j; i++)
			printf("%d: %s", i, history[i]);
	}

	else if(token[0][0] == '!')
	{
		token[0][0] = '0';
		int n = atoi(token[0]);
		if(n <= 0)
			printf("'!' command only works for numbers between 1 and 15\n");
		else if(n > hist_ctr)
			printf("Command not in history.\n");
		else{}
	}


	else
	{

		pid_t pid = fork();

		int status;

		if(pid == -1)
		{
			printf("Thread creation attempt failed. Program will exit now\n");
			exit(EXIT_FAILURE);
		}

		else if(pid != 0)
		{
			update_pids(pids, pid, pid_ctr);
			pid_ctr++;
			wait(&status);
		//	pause();
		}

		else
		{

			//Define paths to search commands in, in the order we will search them
	if(token[0][0] == '`')
		sleep(60);
			char* path1 = (char*)malloc(MAX_COMMAND_SIZE + 15);
			strcpy(path1,"./");
			strcat(path1, token[0]);

			char* path2= (char*)malloc(MAX_COMMAND_SIZE + 15);
			strcpy(path2,"/usr/local/bin/");
			strcat(path2, token[0]);

			char* path3= (char*)malloc(MAX_COMMAND_SIZE + 15);
			strcpy(path3,"/usr/bin/");
			strcat(path3, token[0]);

			char* path4= (char*)malloc(MAX_COMMAND_SIZE + 15);
			strcpy(path4, "/bin/");
			strcat(path4, token[0]);

			if(execv(path1, token) == -1)
				if(execv(path2, token) == -1)
					if(execv(path3, token) == -1)
						if(execv(path4, token) == -1)
							printf("%s: Command not found.\n", token[0]);

			free(path1); free(path2); free(path3); free(path4);
			exit(EXIT_SUCCESS);
		}
	}

	}
	return 0;
}

void parse_input(char** token, char* cmd_str)
{
	int token_count = 0;

	// Pointer to point to the token
	// parsed by strsep
	char* arg_ptr;

	char* working_str  = strdup( cmd_str );

	// we are going to move the working_str pointer so
	// keep track of its original value so we can deallocate
	// the correct amount at the end
	char* working_root = working_str;

	// Tokenize the input stringswith whitespace used as the delimiter
	while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
			(token_count < MAX_NUM_ARGUMENTS))
	{
		token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );

		if( strlen( token[token_count] ) == 0 )
		{
			token[token_count] = NULL;
		}
		token_count++;
	}

	free( working_root );

	// We force NULL-termination to each parameter passed
	int token_index  = 0;

	for( token_index = 0; token_index < token_count; token_index ++ )
	{
		if(token[token_index] == NULL)
			continue;

		strcat(token[token_index], "\0");
	}

	return;
}

void update_history(char** history, char* cmd_str, int hist_ctr)
{
	int j;

	if(hist_ctr < 15)
	{
		strcpy(history[hist_ctr], cmd_str);
	}

	else
	{
		for(j = 0; j < 14; j++)
		{
			strcpy(history[j], history[j+1]);
		}
		strcpy(history[14], cmd_str);
	}

	return;
}

void update_pids(int pids[15], int pid, int pid_ctr)
{
	int j;

	if(pid_ctr < 15)
	{
		pids[pid_ctr] = pid;
	}

	else
	{
		for(j = 0; j < 14; j++)
		{
			pids[j] = pids[j+1];
		}
		pids[14] = pid;
	}

	return;
}
