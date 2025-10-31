/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "VCB.h"


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	// Check if filesystem already exists by reading block 0 and checking magic number
	char* testBuffer = (char*)malloc(blockSize);
	if (testBuffer == NULL)
		{
		printf("Error: Failed to allocate memory for filesystem check\n");
		return -1;
		}

	// Try to read block 0
	memset(testBuffer, 0, blockSize);
	uint64_t blocksRead = LBAread(testBuffer, 1, 0);
	
	int filesystemExists = 0;
	if (blocksRead == 1)
		{
		// Cast to VCB struct to check magic number
		struct VCB* testVCB = (struct VCB*)testBuffer;
		if (testVCB->MagicNo == VCB_MAGIC)
			{
			filesystemExists = 1;
			}
		}

	free(testBuffer);

	if (filesystemExists)
		{
		// Existing filesystem - read VCB from disk
		printf("Existing filesystem detected. Loading VCB...\n");
		if (readVCB(blockSize) != 0)
			{
			printf("Error: Failed to read VCB from existing filesystem\n");
			return -1;
			}
		}
	else
		{
		// New filesystem - initialize and write VCB
		printf("New filesystem detected. Initializing VCB...\n");
		if (initVCB(numberOfBlocks, blockSize) != 0)
			{
			printf("Error: Failed to initialize VCB\n");
			return -1;
			}
		}

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}