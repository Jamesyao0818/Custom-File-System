/**************************************************************
* Class::  CSC-415-01 Spring 2024
* Name:: Enrique Cabral Joseph Bowen
* Student IDs:: 924056104 922966782
* GitHub-Name:: rommix408
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: VCB.c
*
* Description:: Volume Control Block implementation.
*	Handles reading, writing, and initialization of the VCB
*	which is stored at block 1 on disk.
*
**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include "VCB.h"

// Global variable to store VCB in memory after reading
// This avoids repeated disk access
static struct VCB* vcbInMemory = NULL;

// Store block size for read/write operations
static uint64_t vcbBlockSize = 512;  // Default block size

// Initialize a new VCB and write it to disk (for formatting new filesystem)
int initVCB(uint64_t numberOfBlocks, uint64_t blockSize)
	{
	// Store block size for future operations
	vcbBlockSize = blockSize;

	// Allocate buffer for one block (VCB will be stored in this block)
	char* buffer = (char*)malloc(blockSize);
	if (buffer == NULL)
		{
		printf("Error: Failed to allocate memory for VCB\n");
		return -1;
		}

	// Zero out the buffer to ensure clean state
	memset(buffer, 0, blockSize);

	// Cast buffer to VCB struct pointer
	struct VCB* vcb = (struct VCB*)buffer;

	// Set VCB fields
	vcb->MagicNo = VCB_MAGIC;
	vcb->numBlocks = (unsigned int)numberOfBlocks;
	vcb->blockSize = (unsigned int)blockSize;
	vcb->numFreeBlocks = numberOfBlocks - 7;
	vcb->freeTail = 7;
	// Due to partition table being at physical block address 0, vcb is at lba 0, but really pba 1.
	// Subsequently fsm is at lba 1 pba 2.
	vcb->blockIndexFSM = 1;
	
	// Root directory will start after VCB (lba 0) and FSM (lba's 1-6)
	// So root directory starts at lba 6 phys 7.
	vcb->blockIndexRootDirectory = 6;

	// Write VCB to disk at block 1
	uint64_t blocksWritten = LBAwrite(buffer, 1, 0);
	if (blocksWritten != 1)
		{
		printf("Error: Failed to write VCB to disk\n");
		free(buffer);
		return -1;
		}

	// Store VCB in memory (allocate separate memory for persistent storage)
	vcbInMemory = (struct VCB*)malloc(sizeof(struct VCB));
	if (vcbInMemory == NULL)
		{
		printf("Error: Failed to allocate memory for VCB storage\n");
		free(buffer);
		return -1;
		}

	// Copy VCB data to persistent memory
	memcpy(vcbInMemory, vcb, sizeof(struct VCB));

	// Free temporary buffer
	free(buffer);

	printf("VCB initialized: MagicNo=0x%X, numBlocks=%u, blockSize=%u, FSM=%u, RootDir=%u, NumFreeBlocks=%u\n",
		vcbInMemory->MagicNo, vcbInMemory->numBlocks, vcbInMemory->blockSize,
		vcbInMemory->blockIndexFSM, vcbInMemory->blockIndexRootDirectory, vcbInMemory->numFreeBlocks);

	return 0;
	}

// Read VCB from disk (block 1) into memory
int readVCB(uint64_t blockSize)
	{
	// Store block size for future operations
	vcbBlockSize = blockSize;

	// Allocate buffer for one block
	char* buffer = (char*)malloc(blockSize);
	if (buffer == NULL)
		{
		printf("Error: Failed to allocate memory for VCB read\n");
		return -1;
		}

	// Zero out the buffer
	memset(buffer, 0, blockSize);

	// Read block 1 from disk
	uint64_t blocksRead = LBAread(buffer, 1, 0);
	if (blocksRead != 1)
		{
		printf("Error: Failed to read VCB from disk\n");
		free(buffer);
		return -1;
		}

	// Cast buffer to VCB struct pointer
	struct VCB* vcb = (struct VCB*)buffer;

	// Verify magic number to ensure valid filesystem
	if (vcb->MagicNo != VCB_MAGIC)
		{
		printf("Error: Invalid VCB magic number (0x%X). Expected 0x%X\n",
			vcb->MagicNo, VCB_MAGIC);
		free(buffer);
		return -1;
		}

	// Allocate memory for persistent storage if not already allocated
	if (vcbInMemory == NULL)
		{
		vcbInMemory = (struct VCB*)malloc(sizeof(struct VCB));
		if (vcbInMemory == NULL)
			{
			printf("Error: Failed to allocate memory for VCB storage\n");
			free(buffer);
			return -1;
			}
		}

	// Copy VCB data to persistent memory
	memcpy(vcbInMemory, vcb, sizeof(struct VCB));

	// Free temporary buffer
	free(buffer);

	printf("VCB loaded: MagicNo=0x%X, numBlocks=%u, blockSize=%u, FSM=%u, RootDir=%u\n",
		vcbInMemory->MagicNo, vcbInMemory->numBlocks, vcbInMemory->blockSize,
		vcbInMemory->blockIndexFSM, vcbInMemory->blockIndexRootDirectory);

	return 0;
	}

// Write VCB from memory to disk (block 1)
int writeVCB(void)
	{
	if (vcbInMemory == NULL)
		{
		printf("Error: VCB not loaded in memory\n");
		return -1;
		}

	// Allocate buffer for one block
	char* buffer = (char*)malloc(vcbBlockSize);
	if (buffer == NULL)
		{
		printf("Error: Failed to allocate memory for VCB write\n");
		return -1;
		}

	// Zero out the buffer
	memset(buffer, 0, vcbBlockSize);

	// Copy VCB to buffer
	memcpy(buffer, vcbInMemory, sizeof(struct VCB));

	// Write to disk at block 1
	uint64_t blocksWritten = LBAwrite(buffer, 1, 0);
	if (blocksWritten != 1)
		{
		printf("Error: Failed to write VCB to disk\n");
		free(buffer);
		return -1;
		}

	// Free temporary buffer
	free(buffer);

	return 0;
	}

// Get pointer to in-memory VCB (for other modules to access)
struct VCB* getVCB(void)
	{
	return vcbInMemory;
	}

