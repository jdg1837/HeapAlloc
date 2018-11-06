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
#define SLASH "/"

#define MAX_COMMAND_SIZE 255	// The maximum command-line size

#define MAX_NUM_ARGUMENTS 4	// Mav shell only supports one command plus 10 arguments at a time
#define MAX_LENGTH_PATH 15

FILE *fp;
int is_open = 0;

char 	 VolumeName[11];
uint16_t BPB_BytesPerSec;
uint8_t  BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t	 BPB_NumFATS;
uint32_t BPB_FATSz32;

uint32_t root_offset;
int current_offset;

struct __attribute__((__packed__)) DirectoryEntry
{
	char 		DIR_Name[11];
	uint8_t 	DIR_Attr;
	uint8_t		Unused1[8];
	uint16_t	DIR_FirstClusterHigh;
	uint8_t		Unused2[4];
	uint16_t	DIR_FirstClusterLow;
	uint32_t	DIR_FileSize;
};

struct DirectoryEntry dir[16];

void parse_input(char**, char*, int);		//foo to parse input and tokenize it
void get_directory( int directory_LBA );
int16_t NextLB(uint32_t sector);
int LBAToOffset( int32_t sector );
int compare_names(char*);
void to_uppercase(char*);
void format_name(int, char*);

int main()
{
	int i, j;
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
		parse_input(token, cmd_str, 0);

		if((strcmp(token[0], "exit") == 0) || strcmp(token[0], "quit") == 0)
		{
			if(is_open != 0)
			{
				fclose(fp);
			}

			return 0;
		}

		else if(strcmp(token[0], "open") == 0)
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

				fseek(fp, 71, SEEK_SET);
				fread(&VolumeName, 11, 1, fp);

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

				root_offset = (BPB_RsvdSecCnt*BPB_BytesPerSec) + (BPB_NumFATS*BPB_FATSz32*BPB_BytesPerSec);

				get_directory(root_offset);

				current_offset = root_offset;
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
			if(token[1] == NULL)
			{
				printf("Error: Filename needed\n");
				continue;			
			}

			int filename_pos = compare_names(token[1]);

			if(filename_pos == -1)
					printf("Error: File not found\n");
			
			else
			{	
					printf("\nAttributes:\t%x", dir[filename_pos].DIR_Attr);
					printf("\nSize:\t\t%d", dir[filename_pos].DIR_FileSize);
					printf("\nCluster\t\t%d\n", dir[filename_pos].DIR_FirstClusterLow);
			}
		}

		else if(strcmp(token[0], "read") == 0)
		{
			if(token[1] == NULL || token[2] == NULL || token[3] == NULL)
			{
				printf("Error: Make sure command is of form: read <filename> <position> <number of bytes> \n");
				continue;			
			}

			if(token[2] < 0 || token[3] < 0)
			{
				printf("Error: offset and number of bytes must be positive");
				continue;
			}

			int offset_in = atoi(token[2]);
			int remaining = atoi(token[3]);

			int pos = compare_names(token[1]);

			if(pos == -1)
			{
				printf("Error: File not found\n");
				continue;
			}
			
			int file_size = dir[pos].DIR_FileSize;

			if(remaining > (file_size - offset_in))
			{
				printf("Error: Values given exceed file size\n");
				continue;
			}

			int LowCluster = dir[pos].DIR_FirstClusterLow;

			int offset = LBAToOffset(LowCluster) + offset_in;

			fseek(fp, offset, SEEK_SET);

			while(remaining >= 512)
			{
            	char * ptr = (char*)malloc( 512 );
            	fread( ptr, 1, 512, fp );
				printf("%s", ptr);
            	free( ptr );
				
				remaining -= 512;

				LowCluster = NextLB(LowCluster);

				if(LowCluster == -1)
					break;

				offset = LBAToOffset(LowCluster);
				fseek(fp, offset, SEEK_SET);
			}

			if(remaining > 0) 
            {
            	char * ptr = (char*)malloc( remaining );
            	fread( ptr, 1, remaining, fp );
				printf("%s", ptr);
            	free( ptr );
            }				
		}

		else if(strcmp(token[0], "get") == 0)
		{
			if(token[1] == NULL)
			{
				printf("Error: Filename needed\n");
				continue;			
			}

			int pos = compare_names(token[1]);

			if(pos == -1)
			{
				printf("Error: File not found\n");
				continue;
			}
			
			FILE *new_fp;
			new_fp = fopen(token[1], "w");
			
			int file_size = dir[pos].DIR_FileSize;
			int LowCluster = dir[pos].DIR_FirstClusterLow;

			int offset = LBAToOffset(LowCluster);

			fseek(fp, offset, SEEK_SET);

			while(file_size >= 512)
			{
            	char * ptr = (char*)malloc( 512 );
            	fread( ptr, 1, 512, fp );
				fwrite(ptr, 1, 512, new_fp);
            	free( ptr );
				
				file_size -= 512;

				LowCluster = NextLB(LowCluster);

				if(LowCluster == -1)
					break;

				offset = LBAToOffset(LowCluster);
				fseek(fp, offset, SEEK_SET);
			}

			if(file_size > 0) 
            {
            	char * ptr = (char*)malloc( file_size );
            	fread( ptr, 1, file_size, fp );
				fwrite(ptr, file_size, 1, new_fp);
            	free( ptr );
            }

			fclose(new_fp);									
		}

		else if(strcmp(token[0], "cd") == 0)
		{
			if(token[1] == NULL)
			{
				get_directory(root_offset);
				current_offset = root_offset;
				continue;
			}

			//define array for tokenized input
			char* path[MAX_LENGTH_PATH];

			//call function to tokenize input string		                                   
			parse_input(path, token[1], 1);

			int buffer_offset = current_offset;

			int failed = 0;

			i = 0;

            while(path[i] != NULL)
            {
                if(strcmp(path[i], "~") == 0 )
                {
					if(i == 0)
					{
						current_offset = root_offset;
						get_directory(root_offset);
						i++;
						continue;
					}

					else
					{
						failed = 1;					
						break;
					}
                }
                
			    int filename_pos = compare_names(path[i]);

				if(filename_pos == -1)
				{
					failed = 1;
					break;
				}

				else	
				{
					if(dir[filename_pos].DIR_FileSize == 0) 
					{
						current_offset = dir[filename_pos].DIR_FirstClusterLow;	
						int cluster = dir[filename_pos].DIR_FirstClusterLow;
						if( cluster == 0 ) cluster = 2;
						get_directory(LBAToOffset(cluster));
					}

					else
					{
						failed = 1;
						break;
					}
				}

                i++;
			}

			if(failed == 1)
			{
				printf("Error: Path not found\n");
				get_directory(LBAToOffset(buffer_offset));
			}
	
		}

		else if(strcmp(token[0], "ls") == 0)
		{
			int dir_changed = 0;

			if(token[1] != NULL)
			{
				if(strcmp(token[1], "..") == 0)
				{
					int filename_pos = compare_names(token[1]);
					if(filename_pos == -1) continue;

					int cluster = dir[filename_pos].DIR_FirstClusterLow;
					if( cluster == 0 ) cluster = 2;
					get_directory(LBAToOffset(cluster));

					dir_changed = 1;
				}

				else if(strcmp(token[1], ".") != 0)
				{
					printf("Error: %s not a valid ls command\n", cmd_str);
				}
			}

			for(i = 0; i < 16; i ++)
			{
				if(((dir[i].DIR_Name[0] == 0xffffffe5) || 
                    (dir[i].DIR_Name[0] == 0xffffff05) ||  
                    (dir[i].DIR_Name[0] == 0xffffff00)))
                {
                    continue;
                }

				if((dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20))
				{
					char name_buffer[12];
					memcpy(name_buffer, dir[i].DIR_Name, 11);
					name_buffer[11] = '\0';
					printf("%s\n", name_buffer);
				}
			}

			if(dir_changed)
			{
				get_directory(LBAToOffset(current_offset));
			}
		}

		else if(strcmp(token[0], "volume") == 0)
		{
			char name_buffer[12];
			memcpy(name_buffer, VolumeName, 11);
			name_buffer[11] = '\0';
			printf("Volume Label: %s\n", name_buffer);
		}
	}
}

//This function takes the command string
//tokenizes it, using whitespaces as parameter
//and puts the tokens in the token array
void parse_input(char** token, char* cmd_str, int del)
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
    if(del == 0)
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
    else if(del == 1)
	while ( ( (arg_ptr = strsep(&working_str, SLASH ) ) != NULL) && 
			(token_count < MAX_LENGTH_PATH))
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

int compare_names(char* filename)
{
	if(filename[0]== '\0')
		return -1;
	
	to_uppercase(filename);

	int i;

	for(i = 0; i <16; i++)
	{
		char name_buffer[13];

		format_name(i, name_buffer);

		if(strcmp(name_buffer, filename) == 0)
			return i;
	}

	return -1;
}

void to_uppercase(char* filename)
{
	int i;

	int filename_length = strlen(filename);

	for(i = 0;  i < filename_length; i++)
		if(filename[i] > 96 && filename[i] < 123)
			filename[i] -= 32;
}

void format_name(int i, char name_buffer[13])
{
	int j;


	bzero(name_buffer, 13);

	for(j = 0;;  j++)
	{
		if(dir[i].DIR_Name[j] == ' ' || j == 8)
			break;					
	
		name_buffer[j] = dir[i].DIR_Name[j];
	}

	if(dir[i].DIR_Name[8] != ' ')
	{
		name_buffer[j] = '.';
		name_buffer[++j] = dir[i].DIR_Name[8];
		name_buffer[++j] = dir[i].DIR_Name[9];
		name_buffer[++j] = dir[i].DIR_Name[10];
	}

}

int16_t NextLB(uint32_t sector)
{
	uint32_t FAT_address = (BPB_BytesPerSec*BPB_RsvdSecCnt) + (sector * 4);
	int16_t next;
	fseek(fp, FAT_address, SEEK_SET);
	fread(&next, 2, 1, fp);
	return next;
}

int LBAToOffset( int32_t sector)
{
	return (( sector - 2 ) * BPB_BytesPerSec ) + ( BPB_BytesPerSec * BPB_RsvdSecCnt ) +
             ( BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec );
}

void get_directory( int directory_LBA )
{
	int i;

	fseek(fp, directory_LBA, SEEK_SET);

	for(i = 0; i < 16; i ++)
	{
		fread(&dir[i], sizeof(struct DirectoryEntry), 1, fp);
	}
}
