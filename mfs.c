// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
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

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define MAX_NUM_OF_FILES 16

// Holds resereved section information
struct ReservedSection
{
	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint32_t BPB_FATSz32;
};

// This struct represents the directory entry
struct __attribute__((__packed__)) DirectoryEntry
{
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t Unused[8];
	uint16_t DIR_FstCluHI;
	uint8_t Unused2[4];
	uint16_t DIR_FstCluLO;
	uint32_t DIR_FileSize;
};

// Reads in the reserved section
// Param 1: Pass in a struct to the reserved section as specified by struct ReservedSection
// Param 2: Pointer to the file
void ReadReservedSection(struct ReservedSection *Rsvd, FILE *FAT32Ptr)
{
	fseek(FAT32Ptr, 11, SEEK_SET);
	fread(&Rsvd->BPB_BytesPerSec, 2, 1, FAT32Ptr);
	
	fseek(FAT32Ptr, 13, SEEK_SET);
	fread(&Rsvd->BPB_SecPerClus, 1, 1, FAT32Ptr);
	
	fseek(FAT32Ptr, 14, SEEK_SET);
	fread(&Rsvd->BPB_RsvdSecCnt, 2, 1, FAT32Ptr);
	
	fseek(FAT32Ptr, 16, SEEK_SET);
	fread(&Rsvd->BPB_NumFATs, 1, 1, FAT32Ptr);
	
	fseek(FAT32Ptr, 36, SEEK_SET);
	fread(&Rsvd->BPB_FATSz32, 4, 1, FAT32Ptr);
}

void PrintReservedSection(const struct ReservedSection RsvdSec)
{
	printf("\nBPB_BytesPerSec = %d\n", RsvdSec.BPB_BytesPerSec);
	printf("BPB_BytesPerSec = %x\n\n", RsvdSec.BPB_BytesPerSec);
	
	printf("BPB_SecPerClus = %d\n", RsvdSec.BPB_SecPerClus);
	printf("BPB_SecPerClus = %x\n\n", RsvdSec.BPB_SecPerClus);
	
	printf("BPB_RsvdSecCnt = %d\n", RsvdSec.BPB_RsvdSecCnt);
	printf("BPB_RsvdSecCnt = %x\n\n", RsvdSec.BPB_RsvdSecCnt);
	
	printf("BPB_NumFATs = %d\n", RsvdSec.BPB_NumFATs);
	printf("BPB_NumFATs = %x\n\n", RsvdSec.BPB_NumFATs);
	
	printf("BPB_FATSz32 = %d\n", RsvdSec.BPB_FATSz32);
	printf("BPB_FATSz32 = %x\n\n", RsvdSec.BPB_FATSz32);
}

// Calculates the byte at which root directory is
int RootDirOffset(const struct ReservedSection RsvdSec)
{
	return (RsvdSec.BPB_NumFATs * RsvdSec.BPB_FATSz32 * RsvdSec.BPB_BytesPerSec) +
			(RsvdSec.BPB_RsvdSecCnt * RsvdSec.BPB_BytesPerSec);
}

int LBAToOffset(const int32_t sector, const struct ReservedSection RsvdSec)
{
	return ((sector - 2) * RsvdSec.BPB_BytesPerSec) + (RsvdSec.BPB_NumFATs * RsvdSec.BPB_FATSz32 * RsvdSec.BPB_BytesPerSec) +
			(RsvdSec.BPB_RsvdSecCnt * RsvdSec.BPB_BytesPerSec);
}

// Comapares a user friendly filename like bar.txt with BAR     TXT
bool CompareFilename(const char IMG_Name[], const char input[])
{
	char expanded_name[12];
	memset( expanded_name, ' ', 12 );

	char TempInput[12];
	strncpy(TempInput, input, strlen(input));
	
	char *token = strtok( TempInput, "." );

	strncpy( expanded_name, token, strlen( token ) );

	token = strtok( NULL, "." );

	if( token )
	{
		strncpy( (char*)(expanded_name+8), token, strlen(token ) );
	}

	expanded_name[11] = '\0';

	int i;
	for( i = 0; i < 11; i++ )
	{
		expanded_name[i] = toupper( expanded_name[i] );
	}
	
	if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
	{
		return true;
	}
	
	return false;
}

// Checks to see if the filename/directory exists or not
int empty(const struct DirectoryEntry DirEntry[], const char Name[])
{
	int EntryIdx = 0;
	for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
	{
		if((DirEntry[EntryIdx].DIR_Attr == 0x01 ||
		   DirEntry[EntryIdx].DIR_Attr == 0x10 ||
		   DirEntry[EntryIdx].DIR_Attr == 0x20) &&
		   CompareFilename(DirEntry[EntryIdx].DIR_Name, Name))
			return EntryIdx;
	}
	return -1;
}

int main()
{
	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
	FILE *FAT32Ptr = NULL;
	struct ReservedSection RsvdSec;
	memset(&RsvdSec, 0, sizeof(RsvdSec));
	struct DirectoryEntry DirEntry[MAX_NUM_OF_FILES];
	memset(&DirEntry, 0, sizeof(DirEntry));
	// Stores the byte where the root directory starts
	uint32_t RtDirOffset = 0;
	
	// This stores the offset that we have to make to go to a sub directory
	// This will also be used to cd back a directory
	uint32_t Offset = 0;
	
	while( 1 )
	{
		// Print out the mfs prompt
		printf ("mfs> ");

		// Read the command from the commandline.  The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		/* Parse input */
		char *token[MAX_NUM_ARGUMENTS];

		int   token_count = 0;                                 
															   
		// Pointer to point to the token
		// parsed by strsep
		char *arg_ptr;                                         
															   
		char *working_str  = strdup( cmd_str );                

		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char *working_root = working_str;

		// Tokenize the input stringswith whitespace used as the delimiter
		while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
				  (token_count<MAX_NUM_ARGUMENTS))
		{
			token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
			if( strlen( token[token_count] ) == 0 )
			{
				token[token_count] = NULL;
			}
			token_count++;
		}
		
		if(token[0] == NULL)
			continue;
		
		if(strcmp(token[0], "open") == 0)
		{
			if(FAT32Ptr == NULL)
			{
				// Read the file
				FAT32Ptr = fopen(token[1], "r");
				
				if(FAT32Ptr == NULL)
				{
					perror("Error: File system image not found.");
					continue;
				}
				// Read the reserved section
				ReadReservedSection(&RsvdSec, FAT32Ptr);
				
				// Get the position at which the root directory is
				RtDirOffset = RootDirOffset(RsvdSec);
				Offset = RtDirOffset;
				// Load in the root directory
				fseek(FAT32Ptr, RtDirOffset, SEEK_SET);
				fread(&DirEntry, 32 * MAX_NUM_OF_FILES, 1, FAT32Ptr);
			}
			else
			{
				printf("FAT32 file is already open.\n");
			}
		}
		else if(strcmp(token[0], "close") == 0)
		{
			if(FAT32Ptr == NULL)
			{
				printf("Error: File system not open.\n");
				continue;
			}
			
			memset(&RsvdSec, 0, sizeof(RsvdSec));
			memset(&DirEntry, 0, sizeof(DirEntry));
			fclose(FAT32Ptr);
			FAT32Ptr = NULL;
		}
		else if(strcmp(token[0], "exit") == 0)
		{
			if(FAT32Ptr != NULL)
				fclose(FAT32Ptr);
			
			break;
		}
		else if(FAT32Ptr != NULL)
		{
			if(strcmp(token[0], "info") == 0)
			{
				PrintReservedSection(RsvdSec);
			}
			else if(strcmp(token[0], "stat") == 0)
			{
				if(token[1] == NULL)
				{
					printf("Specify a file/dir name to know its stats.\n");
					continue;
				}
				// Print out info about file or directory
				int FileExists = empty(DirEntry, token[1]);
				if(FileExists != -1)
				{
					printf("Attribute\t\tSize\t\tStarting Cluster Number\n");
					printf("%d\t\t\t%d\t\t%d\n", DirEntry[FileExists].DIR_Attr,
													   DirEntry[FileExists].DIR_FileSize,
													   DirEntry[FileExists].DIR_FstCluLO);
				}
				else
				{
					printf("File or directory does not exist.\n");
				}
			}
			else if(strcmp(token[0], "ls") == 0)
			{	
				int EntryIdx = 0;
				printf("\n");
				for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
				{
					// All of these hex values are defined in the FAT32 spec
					// Print only if:
					// Not deleted - 0xe5
					// Not NULL - 0x00
					// Read only - 0x01
					// A sub directory - 0x10
					// Archived - 0x20
					if((DirEntry[EntryIdx].DIR_Attr == 0x01 ||
					    DirEntry[EntryIdx].DIR_Attr == 0x10 ||
					    DirEntry[EntryIdx].DIR_Attr == 0x20 ||
					    DirEntry[EntryIdx].DIR_Name[0] == 0x2e) &&
						DirEntry[EntryIdx].DIR_Name[0] != 0xe5)
					{
						char TempFileName[12];
						strncpy(TempFileName, DirEntry[EntryIdx].DIR_Name, 11);
						TempFileName[12] = 0;
						
						// If the first character is 0x05, repalce it with 0xe5
						// This is because a filename may contain 0xe5 as the first char
						// But 0xe5 also means a deleted file
						// So, a filename that starts with 0xe5, is replaced with 0x05
						// Which has to be replaced back
						if(DirEntry[EntryIdx].DIR_Name[0] == 0x05)
							TempFileName[0] = 0xe5;
						
						printf("%s\n", TempFileName);
					}
				}
				printf("\n");
			}
			else if(token[1] != NULL && strcmp(token[0], "cd") == 0)
			{
				if(strcmp(token[1], ".") == 0 || strcmp(token[1], "./") == 0)
				{
					continue;
				}
				// Go to the root directory
				else if(strcmp(token[1], "~") == 0)
				{
					fseek(FAT32Ptr, RtDirOffset, SEEK_SET);
					fread(&DirEntry, 32 * MAX_NUM_OF_FILES, 1, FAT32Ptr);
				}
				// Going back a directory
				else if(strcmp(token[1], "..") == 0)
				{
					int EntryIdx = 0;
					for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
					{
						// An entry is with the first byte of the name being 0x2e is a directory
						// An entry with the second byte also being 0x2e means the cluster field contains
						// cluster number of the parent
						if(DirEntry[EntryIdx].DIR_Name[0] == 0x2e && DirEntry[EntryIdx].DIR_Name[1] == 0x2e)
						{
							// Cluster number of 0x0000 means the root directory
							if(DirEntry[EntryIdx].DIR_FstCluLO == 0x0000)
							{
								fseek(FAT32Ptr, RtDirOffset, SEEK_SET);
								fread(&DirEntry, 32 * MAX_NUM_OF_FILES, 1, FAT32Ptr);
							}
							else
							{
								Offset = LBAToOffset(DirEntry[EntryIdx].DIR_FstCluLO, RsvdSec);
								fseek(FAT32Ptr, Offset, SEEK_SET);
								fread(&DirEntry, 32 * MAX_NUM_OF_FILES, 1, FAT32Ptr);
							}
							break;
						}
					}
				}
				else
				{
					int EntryIdx = 0;
					for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
					{
						// First check if the file exists or not
						if(memcmp(token[1], DirEntry[EntryIdx].DIR_Name, sizeof(token[1]) - 1) == 0)
						{
							// Check to see if the file is a directory by checking the attribute against 0x10
							if(DirEntry[EntryIdx].DIR_Attr == 0x10)
							{
								Offset = LBAToOffset(DirEntry[EntryIdx].DIR_FstCluLO, RsvdSec);
								fseek(FAT32Ptr, Offset, SEEK_SET);
								fread(&DirEntry, 32 * MAX_NUM_OF_FILES, 1, FAT32Ptr);
							}
							else
							{
								printf("You are trying to cd into a file which is not valid.\n");
							}
							break;
						}
					}
					
					if(EntryIdx == MAX_NUM_OF_FILES)
					{
						printf("The file/directory does not exist.\n");
					}
				}
			}
			else
			{
				printf("%s : Command not supported.\n", token[0]);
			}
		}
		else
		{
			printf("Error: File system image must be opened first.\n");
		}
		
		free( working_root );

	}
	return 0;
}
