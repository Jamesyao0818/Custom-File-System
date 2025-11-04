/**************************************************************
* Class::  CSC-415-01 Spring 2024
* Name:: Enrique Cabral 
* Student IDs:: 924056104
* GitHub-Name:: rommix408
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: VCB.h
*
* Description:: Volume Control Block header file.
*	Defines the VCB structure and function declarations
*	for filesystem metadata management.
*
**************************************************************/

#ifndef _VCB_H
#define _VCB_H

#include <sys/types.h>

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

// Magic number to identify valid filesystem
#define VCB_MAGIC 0x41534442  // "ASDB" in ASCII hex

// Volume Control Block structure
// Stored at block 0 on disk
struct VCB
	{
	unsigned int numBlocks;			// Total number of blocks in the volume
	unsigned int blockSize;			// Block size in bytes
	unsigned int blockIndexFSM;		// Index of the block containing the File System Metadata
	unsigned int blockIndexRootDirectory;  // Index of the block containing the Root Directory
	unsigned int MagicNo;			// Magic number identifier (signature)
	};

// Initialize a new VCB and write it to disk (for formatting new filesystem)
// Returns 0 on success, -1 on error
int initVCB(uint64_t numberOfBlocks, uint64_t blockSize);

// Read VCB from disk (block 1) into memory
// blockSize is needed to allocate the correct buffer size
// Returns 0 on success, -1 on error
int readVCB(uint64_t blockSize);

// Write VCB from memory to disk (block 1)
// Returns 0 on success, -1 on error
int writeVCB(void);

// Get pointer to in-memory VCB (for other modules to access)
// Returns NULL if VCB not loaded
struct VCB* getVCB(void);

#endif

