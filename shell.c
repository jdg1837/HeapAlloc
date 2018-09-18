//MavShell (2018)

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

#define MAX_NUM_ARGUMENTS 11     // Mav shell only supports five arguments

#define PATH ":/usr/local/bin:/usr/bin:/bin"

//void parse_input(char**, )

int main()
{
	//Allocate memory to get input from the user
	char* cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

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
	int is_valid = 0, i =0;

	for(i = 0; i < strlen(cmd_str) -1; i++)
		if(cmd_str[i] != ' ')
			is_valid = 1;

	if(is_valid == 0)
		continue;

	/* Parse input */
	char* token[MAX_NUM_ARGUMENTS];

	int   token_count = 0;                               
			                                   
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

	// If cmd is to quit shell, we exit
	if(strcmp(token[0],"quit") == 0 || strcmp(token[0], "exit") ==0)		
		return 0;

	// If cmd is tp change directory, we do so from main thread
	else if(strcmp(token[0],"cd") == 0)
	{	
		if(chdir(token[1]) == -1)
			printf("Directory could not be changed. Please verify path.\n");
	}
				
	else
	{

		pid_t pid = fork();
	
		int status;

		if(pid == -1)
		{
			perror("Thread creation attempt failed. Program will exit now");
			exit(EXIT_FAILURE);
		}

		else if(pid != 0)
		{
			wait(&status);
		}

		else
		{		

			//Define paths to search commands in, in the order we will search them
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
							printf("%s\n", errno);

			free(path1); free(path2); free(path3); free(path4);
			exit(EXIT_SUCCESS);
		}
	}
	
	}
	return 0;
}
