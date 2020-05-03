/*
	Author: Sarker Nadir Afridi Azmi
*/

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
	return ((sector - 2) * RsvdSec.BPB_BytesPerSec) +
			(RsvdSec.BPB_NumFATs * RsvdSec.BPB_FATSz32 * RsvdSec.BPB_BytesPerSec) +
			(RsvdSec.BPB_RsvdSecCnt * RsvdSec.BPB_BytesPerSec);
}

int16_t NextLB(FILE * FAT32Ptr, uint32_t sector, struct ReservedSection RsvdSec)
{
	uint32_t FATAddress = (RsvdSec.BPB_BytesPerSec * RsvdSec.BPB_RsvdSecCnt) + (sector * 4);
	int16_t val;
	fseek(FAT32Ptr, FATAddress, SEEK_SET);
	fread(&val, 2, 1, FAT32Ptr);
	return val;
}

// Comapares a user friendly filename like bar.txt with BAR     TXT
// Param 1: Pass in DIR_Name member of the DirEntry struct
// Param 2: User inputter filename
bool CompareFilename(const char IMG_Name[], const char input[])
{
	char expanded_name[12];
	memset( expanded_name, ' ', 12 );

	char TempInput[12];
	
	// Change this later to check bounds
	strcpy(TempInput, input);
	
	char *token = strtok( TempInput, "." );
	
	if(token == NULL)
	{
		strncpy(expanded_name, "..", strlen(".."));
	}
	else
	{
		strncpy( expanded_name, token, strlen( token ) );
	}

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
// If true, returns its index
// Param 1: Pass in array of DirectoryEntry structs that is already populated
// Param 2: User inputted name
int empty(const struct DirectoryEntry DirEntry[], const char Name[])
{
	int EntryIdx = 0;
	for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
	{
		if((DirEntry[EntryIdx].DIR_Attr == 0x01 ||
		    DirEntry[EntryIdx].DIR_Attr == 0x10 ||
		    DirEntry[EntryIdx].DIR_Attr == 0x20 ||
			DirEntry[EntryIdx].DIR_Name[0] == 0x2e) &&
			(DirEntry[EntryIdx].DIR_Name[0] != (signed char)0xe5) &&
		    CompareFilename(DirEntry[EntryIdx].DIR_Name, Name))
			return EntryIdx;
	}
	return -1;
}

// Changes directories
// Param 1: Pointer to the file image
// Param 2: Array of Directory Entries - used to look up the file name
// Param 3: The Lower Cluster number of the file
// Param 4: Root cluster number found using RootDirOffset(const struct ReservedSection RsvdSec);
// Param 5: Reserved section info as defined by the struct above [struct ReservedSection]
void ChDir(FILE * FAT32Ptr, struct DirectoryEntry DirEntry[], const uint16_t Cluster,
			const uint32_t Root, const struct ReservedSection RsvdSec)
{
	int Offset = 0;
	// Cluster Low of 0x0000 represents that parent directory is the root directory
	if(Cluster == 0x0000)
		Offset = Root;
	else
		Offset = LBAToOffset(Cluster, RsvdSec);

	fseek(FAT32Ptr, Offset, SEEK_SET);
	fread(DirEntry, 32 * MAX_NUM_OF_FILES, 1, FAT32Ptr);
}

// Lists the current directory
// Param: Directory Entry for directory to list
void List(const struct DirectoryEntry DirEntry[])
{
	int EntryIdx = 0;
	for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
	{
		// If the file is deleted, move on to the next file
		// Well, the comparison is a hack because I forgot to declare
		// the DIR_Name as unsigned char
		if(DirEntry[EntryIdx].DIR_Name[0] == (signed char)0xe5)
		{
			continue;
		}
		// All of these hex values are defined in the FAT32 spec
		// Print only if:
		// Not deleted - 0xe5
		// Not NULL - 0x00
		// Read only - 0x01
		// A sub directory - 0x10
		// Archived - 0x20
		if(DirEntry[EntryIdx].DIR_Attr == 0x01 ||
		   DirEntry[EntryIdx].DIR_Attr == 0x10 ||
		   DirEntry[EntryIdx].DIR_Attr == 0x20 ||
		   DirEntry[EntryIdx].DIR_Name[0] == 0x2e
		   )
		{
			char TempFileName[12];
			memset(&TempFileName, 0, sizeof(TempFileName));
			strncpy(TempFileName, DirEntry[EntryIdx].DIR_Name, 11);
			
			// If the first character is 0x05, repalce it with 0xe5
			// This is because a filename may contain 0xe5 as the first char
			// But 0xe5 also means a deleted file
			// So, a filename that starts with 0xe5, is replaced with 0x05
			// Which has to be replaced back
			if(DirEntry[EntryIdx].DIR_Name[0] == 0x05)
				TempFileName[0] = (signed char)0xe5;
			
			printf("%s\n", TempFileName);
		}
	}
}

// Tokenizes the path and returns the number of directories
// Param 1: Pass in path to tokenize - path must include /
// Param 2: Array of pointers to char that will hold all of the Directory names
int TokenizePath(const char Path[], char *DirNames[])
{
	char WorkingPath[strlen(Path) + 1];
	memset(&WorkingPath, 0, sizeof(WorkingPath));
	strncpy(WorkingPath, Path, strlen(Path));
	
	char *token = strtok(WorkingPath, "/");
	
	int i = 0;
	for(i = 0; token != NULL; i++)
	{
		DirNames[i] = strndup(token, strlen(token));
		token = strtok(NULL, "/");
	}
	
	return i;
}

// Checks to see if the input is a number or not
// Param: Pass in a string
bool IsInt(const char *token)
{
	int i = 0;
	for(i = 0; i < strlen(token); i++)
	{
		if(!isdigit(token[i]))
			return false;
	}
	return true;
}

// This function is used for both reading and writing a file
// Param 1: Pointer to the file image
// Param 2: Array of Directory Entries - used to look up the file name
// Param 3: Reserved section info as defined by the struct above [struct ReservedSection]
// Param 4: Name of the file to read
// Param 5: Position to star reading the file
// Param 6: Number of bytes to read
// Param 7: 0 - Readfile, 1 - Getfile
// Param 8: Pointer to a size variable which will hold the size of the file
char * ReadFile(FILE * FAT32Ptr, const struct DirectoryEntry DirEntry[],
							   const struct ReservedSection RsvdSec,
							   const char Name[], int Pos, int Bytes, int ReadOrGet, size_t *size)
{
	int Exists = empty(DirEntry, Name);
	if(Exists == -1)
	{
		printf("Error: Could not find file.\n");
		return NULL;
	}
	else
	{
		if(DirEntry[Exists].DIR_Name[0] == 0x2e || DirEntry[Exists].DIR_Attr == 0x10)
		{
			printf("Error: Not a file.\n");
			return NULL;
		}
		else
		{
			if((Pos < 0 || Pos > DirEntry[Exists].DIR_FileSize) ||
				(Bytes < 0 || Bytes > DirEntry[Exists].DIR_FileSize))
			{
				printf("Cannot read given range.\n");
				return NULL;
			}
			
			int16_t sector = DirEntry[Exists].DIR_FstCluLO;
			int TempPos = Pos;
			
			// First we need to find which cluster the position is in
			// If the pos 512 or greater, look into the next logical block
			while(TempPos >= RsvdSec.BPB_BytesPerSec * RsvdSec.BPB_SecPerClus)
			{
				sector = NextLB(FAT32Ptr, sector, RsvdSec);
				TempPos -= RsvdSec.BPB_BytesPerSec * RsvdSec.BPB_SecPerClus;
			}
			
			// Whatever value is left for the Pos, is relative to the cluster
			int Offset = TempPos + LBAToOffset(sector, RsvdSec);
			
			if(Bytes + Pos > DirEntry[Exists].DIR_FileSize)
			{
				printf("Cannot read bytes more than the file size.\n");
				return NULL;
			}
			
			char * buffer = (char *)malloc(DirEntry[Exists].DIR_FileSize);
			memset(buffer, 0, DirEntry[Exists].DIR_FileSize);
			
			int BytesToRead = Bytes;
			*size = DirEntry[Exists].DIR_FileSize;
			
			// If we are using get, read the entire file
			if(ReadOrGet == 1)
			{
				BytesToRead = DirEntry[Exists].DIR_FileSize;
			}
			
			int BytesToReadTemp = BytesToRead;
			int ByteIdx = 0;
			size_t TotalSizeRead = 0;
			
			while(sector != -1)
			{
				// If the bytes to read are more than the cluster size, read a cluster first
				if(BytesToRead > RsvdSec.BPB_BytesPerSec * RsvdSec.BPB_SecPerClus)
				{
					BytesToReadTemp = RsvdSec.BPB_BytesPerSec * RsvdSec.BPB_SecPerClus - TempPos;
				}
				
				fseek(FAT32Ptr, Offset, SEEK_SET);
				size_t SizeRead = fread((buffer + ByteIdx), 1, BytesToReadTemp, FAT32Ptr);
				
				TotalSizeRead += SizeRead;
				// Move the pointer so that we move to a new contiguous block to store
				// the next piece of data
				ByteIdx += BytesToReadTemp;
				
				// Subtract the bytes read
				BytesToRead -= BytesToReadTemp;
				BytesToReadTemp = BytesToRead;
				
				// If there are still bytes to read, locate the next logical block
				if(BytesToReadTemp > 0)
				{
					sector = NextLB(FAT32Ptr, sector, RsvdSec);
					Offset = LBAToOffset(sector, RsvdSec);
					TempPos = 0;
				}
				else
					break;
			}
			
			// If we are not getting the file, print it
			if(ReadOrGet == 0)
			{
				int i = 0;
				for(i = 0; i < Bytes; i++)
						printf("%x ", buffer[i]);
				
				printf("\n");
			}
			
			/* Debugging
			if(TotalSizeRead != DirEntry[Exists].DIR_FileSize)
				printf("Error while reading file.\n");
			*/
			
			return buffer;
		}
	}
}

// Frees memory allocated with malloc
// Param 1: Array of pointers to char
// Param 2: Length of the array
void FreePaths(char *DirNames[], int NumberOfDirectories)
{
	int i = 0;
	for(i = 0; DirNames[i] != NULL && i < NumberOfDirectories; i++)
	{
		free(DirNames[i]);
	}
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
				free(token[token_count]);
				token[token_count] = NULL;
			}
			token_count++;
		}
		
		free(working_str);
		free(working_root);
		
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
					// perror("Error: File system image not found.");
					printf("Error: File system image not found.\n");
					continue;
				}
				// Read the reserved section
				ReadReservedSection(&RsvdSec, FAT32Ptr);
				
				// Get the position at which the root directory is
				RtDirOffset = RootDirOffset(RsvdSec);
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
					printf("Error: File not found.\n");
				}
			}
			else if(strcmp(token[0], "read") == 0)
			{
				// Check Args
				if((token[1] == NULL) || (token[2] == NULL) || (token[3] == NULL))
				{
					printf("Format: read <filename> <position> <number of bytes>\n");
					continue;
				}
				
				if(!IsInt(token[2]) || !IsInt(token[3]))
				{
					printf("read : <position> <number of bytes> have to be integers.\n");
					continue;
				}
				
				int Pos = atoi(token[2]);
				int NoOfBytes = atoi(token[3]);
				
				// Read bytes of the file
				// rf just means Read File
				size_t size;
				char * rf = ReadFile(FAT32Ptr, DirEntry, RsvdSec, token[1], Pos, NoOfBytes,
																					0, &size);
				free(rf);
			}
			else if(strcmp(token[0], "get") == 0)
			{
				// This is just paranoia error checking
				if(token_count < 2 || token_count > 3 || token[1] == NULL)
				{
					printf("Format: get <filename>\n");
					continue;
				}
				
				// Read bytes of the file
				// rf just means Read File
				// Stores the size of the file to write
				size_t size;
				char * rf = ReadFile(FAT32Ptr, DirEntry, RsvdSec, token[1], 0, 0, 1, &size);
				
				if(rf != NULL)
				{
					FILE *fp = fopen(token[1], "w");
					size_t ByteWritten = fwrite(rf, 1, size, fp);
					
					if(size != ByteWritten)
						printf("There were some errors writing the file.\n");
					
					fclose(fp);
				}
				free(rf);
			}
			else if(strcmp(token[0], "ls") == 0)
			{	
				int Exists = -1;
				
				if(token[1] != NULL && strcmp(token[1], "..") == 0)
				{
					// Stores a copy of the directory so that we do not change directory as we ls
					struct DirectoryEntry TempDirEntry[MAX_NUM_OF_FILES];
					Exists = empty(DirEntry, token[1]);
					if(Exists == -1)
						ChDir(FAT32Ptr, TempDirEntry, 0x0000, RtDirOffset, RsvdSec);
					else
						ChDir(FAT32Ptr, TempDirEntry,
										DirEntry[Exists].DIR_FstCluLO, RtDirOffset, RsvdSec);
					List(TempDirEntry);
				}
				else if(token[1] != NULL && strcmp(token[1], ".") == 0)
				{
					continue;
				}
				else if(token[1] == NULL)
				{
					List(DirEntry);
				}
				else
				{
					printf("Invalid flag. Only supports . & ..\n");
				}
			}
			else if(strcmp(token[0], "cd") == 0)
			{
				if(token[1] == NULL)
				{
					printf("Specify destination.\n");
					continue;
				}
				// Stores tokenized file path
				char *DirNames[MAX_NUM_OF_FILES];
				memset(&DirNames, 0, sizeof(DirNames));
				// TOKENIZE path
				int DirectoriesToTraverse = TokenizePath(token[1], DirNames);
				
				int DirCount = 0;
				for(DirCount = 0; DirCount < DirectoriesToTraverse; DirCount++)
				{
					int DirNameLen = strlen(DirNames[DirCount]);
					if(strncmp(DirNames[DirCount], ".", DirNameLen) == 0 ||
					   strncmp(DirNames[DirCount], "./", DirNameLen) == 0)
					{
						continue;
					}
					// Go to the root directory
					else if(strncmp(DirNames[DirCount], "~", DirNameLen) == 0)
					{
						// Cluster Low of 0x0000 represents that parent directory is
						// the root directory
						ChDir(FAT32Ptr, DirEntry,
										0x0000, RtDirOffset, RsvdSec);
					}
					// Going back a directory
					else if(strncmp(DirNames[DirCount], "..", DirNameLen) == 0)
					{
						int EntryIdx = 0;
						for(EntryIdx = 0; EntryIdx < MAX_NUM_OF_FILES; EntryIdx++)
						{
							// An entry is with the first byte of the name being 0x2e
							// is a directory
							// An entry with the second byte also being 0x2e means
							// the cluster field contains
							// cluster number of the parent
							if(DirEntry[EntryIdx].DIR_Name[0] == 0x2e &&
							   DirEntry[EntryIdx].DIR_Name[1] == 0x2e)
							{
								ChDir(FAT32Ptr, DirEntry,
										DirEntry[EntryIdx].DIR_FstCluLO, RtDirOffset, RsvdSec);
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
							if(CompareFilename(DirEntry[EntryIdx].DIR_Name, DirNames[DirCount]))
							{
								// Check to see if the file is a directory by checking the attribute against 0x10
								if(DirEntry[EntryIdx].DIR_Attr == 0x10)
								{
									ChDir(FAT32Ptr, DirEntry,
										DirEntry[EntryIdx].DIR_FstCluLO, RtDirOffset, RsvdSec);
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
							// If for some reason, the inital entry exists, but the next one doesn't,
							// stay in the current directory
							if(DirCount > 0)
							{
								int ParentIdx = empty(DirEntry, ".");
								ChDir(FAT32Ptr, DirEntry,
										DirEntry[ParentIdx].DIR_FstCluLO, RtDirOffset, RsvdSec);
							}
							printf("The file/directory does not exist.\n");
							break;
						}
					}
				}
				
				FreePaths(DirNames, DirectoriesToTraverse);
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
		
		FreePaths(token, token_count);
	}
	
	free(cmd_str);
	return 0;
}
