/*
	Group Member 0: Juan Diego Gonzalez German
	ID: 1001401837
	Group Member 1: Chelsea May
	ID: 1001140094	
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define WHITESPACE " \t\n"	//separators to parse cmd line

#define MAX_COMMAND_SIZE 255	// The maximum command-line size

#define MAX_NUM_ARGUMENTS 4	// Mav shell only supports one command plus 10 arguments at a time

FILE *fp;
int is_open = 0;

int16_t BPB_BytesPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATS;
int32_t BPB_FATSz32;

int cluster_offset;
int cluster_size;

int buffer;

void parse_input(char**, char*);		//foo to parse input and tokenize it into array of cmd + parameters

int main()
{
	int i;
	char* cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

	while(1)
	{
		// Print out the prompt
		printf ("mfs> ");

		// Read the command from the commandline.
		// Maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		int is_valid = 0;

		for(i = 0; i < strlen(cmd_str) -1; i++)
			if(cmd_str[i] != ' ' && cmd_str[i] != '\t' && cmd_str[i] != '\n')
				is_valid = 1;

		if(is_valid == 0)
			continue;

		//define array for tokenized input
		char* token[MAX_NUM_ARGUMENTS];                       
	
		//call function to tokenize input string		                                   
		parse_input(token, cmd_str);

		if(strcmp(token[0], "open") == 0)
		{
			if(is_open == 1)
			{
				printf("Error: File system image already open.\n");
				continue;
			}

			fp = fopen(token[1], "r");

			if(fp == NULL)
			{
				printf("Error: File system image not found\n");
			}

			else
			{
				is_open = 1;

				fseek(fp, 11, SEEK_SET);
				fread(&BPB_BytesPerSec, 2, 1, fp);


				fseek(fp, 13,  SEEK_SET);
				fread(&BPB_SecPerClus, 1, 1, fp);


				fseek(fp, 14,  SEEK_SET);
				fread(&BPB_RsvdSecCnt, 2, 1, fp);


				fseek(fp, 16,  SEEK_SET);
				fread(&BPB_NumFATS, 1, 1, fp);


				fseek(fp, 36,  SEEK_SET);
				fread(&BPB_FATSz32, 4, 1, fp);

				cluster_offset = (BPB_RsvdSecCnt *BPB_BytesPerSec) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec);
				cluster_size = BPB_SecPerClus * BPB_BytesPerSec;
			}

			continue;
		}

		if(is_open == 0)
		{
			printf("Error: File system image must be opened first.\n");
			continue;
		}

		if(strcmp(token[0], "close") == 0)
		{
			if(is_open == 0)
			{
				printf("Error: File system not open\n");
			}

			else if(fclose(fp) != 0)
			{
				printf("Error: File system not closed\n");
			}

			else
			{
				is_open = 0;
			}		
		}

		else if(strcmp(token[0], "info") == 0)
		{
			printf("BPB_BytesPerSec = %d, %x\n", BPB_BytesPerSec, BPB_BytesPerSec);
			printf("BPB_SecPerClus = %d, %x\n", BPB_SecPerClus, BPB_SecPerClus);
			printf("BPB_RsvdSecCnt = %d, %x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
			printf("BPB_NumFATS = %d, %x\n", BPB_NumFATS, BPB_NumFATS);
			printf("BPB_FATSz32 = %d, %x\n", BPB_FATSz32, BPB_FATSz32);
		}

		else if(strcmp(token[0], "stat") == 0)
		{
			fseek(fp, cluster_offset, SEEK_SET);

			fseek(fp, 28, SEEK_CUR);
			fread(&buffer, 4, 1, fp);
			printf("%d\n", buffer);		

		}
	}
}

//This function takes the command string
//tokenizes it, using whitespaces as parameter
//and puts the tokens in the token array
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

	return;
}


