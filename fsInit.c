/**************************************************************
* Class::  CSC-415-01 Spring 2024
* Name:: Ian Kligman , Joe Bowen, Cheng-Chieh Yao, Enrique Cabral-Mixco	
* Student IDs:: 924122742, 922966782, 923978442, 924056104
* GitHub-Name:: ikligman, josephb333, Jamesyao0818, rommix408
* Group-Name:: Don't Worry We Don't Byte
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
#include "FSM.h"
#include "DE.h"

//Globals for access between functions.
DE *alreadyLoadedRootDir = NULL;
DE *alreadyLoadedCWD = NULL;


int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	// Check if filesystem already exists by reading block 0 and checking magic number
	char* testBuffer = (char*)malloc(blockSize);
	if (testBuffer == NULL){
		printf("Error: Failed to allocate memory for filesystem check\n");
		return -1;
	}

	memset(testBuffer, 0, blockSize);
	// Try to read VCB at block 1
	uint64_t blocksRead = LBAread(testBuffer, 1, 0);

	// Check signature to see if we need to format volume
	int filesystemExists = 0;
	if (blocksRead == 1){
		// Cast to VCB struct to check magic number
		struct VCB* testVCB = (struct VCB*)testBuffer;
		if (testVCB->MagicNo == VCB_MAGIC){ filesystemExists = 1;}
	}
	free(testBuffer);
	// File system found path.
	if (filesystemExists){
		// Existing filesystem - read VCB from disk
		printf("Existing filesystem detected. Loading VCB...\n");
		if (readVCB(blockSize) != 0){
			printf("Error: Failed reading VCB from existing filesystem.\n");
		}
		struct VCB* v = getVCB();
		if (loadFSM(blockSize, v->blockIndexFSM) != 0){
			printf("Error: Failed to load FSM from disk.\n");
			return -1;
		} 
		DE rootStub = {0};
		rootStub.isDir = 1;
		rootStub.blockNumber = v->blockIndexRootDirectory;
		alreadyLoadedRootDir = loadDirectory(&rootStub);
		alreadyLoadedCWD = alreadyLoadedRootDir;
		return 0;
		// Load root directory into memory so available for
		// path parsing operations. Ensures parsePath can
		// always find the root directory when processing absolute paths.
		//if (loadRootDirectory() != 0) {
			//printf("Error: Failed to load root directory\n");
			//return -1;
		//}
		} else {
			// Fresh filestem path
			// New filesystem - initialize and write VCB
			printf("New filesystem detected. Initializing VCB...\n");
			if (initVCB(numberOfBlocks, blockSize) != 0){
			{
			printf("Error: Failed to initialize VCB\n");
			return -1;
			}
			}
			// Initilize FSM (Mark 1..6 used; and write to blocks 2..6)
			if(initFSM(blockSize) != 1){
				printf("Error: Failed to initialize FSM\n");
				return -1;
			}
			// Create Root Directory (should allocate to block 7)
			DE *rootDir = createDir(2, NULL, blockSize);
			if (rootDir == NULL){
				printf("Error: Failed to create directories.\n");
				return -1;
			}
			// Cache the newly created root directory in memory.
			// createDir returns a pointer to the directory in memory,
			// so we can use it directly without reading from disk.
			struct VCB *v = getVCB();
			v->blockIndexRootDirectory = rootDir[0].blockNumber;
			writeVCB();
			alreadyLoadedCWD = rootDir;
			alreadyLoadedRootDir = rootDir;
			//alreadyLoadedRootDir = loadDirectory(&rootStub);
			//alreadyLoadedCWD = rootDir;  // Start at root
			return 0;
		}
}
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}