/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman, Joseph Bowen
* Student IDs:: 924122742, 922966782
* GitHub-Name:: ikligman josephb333
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: fileinfo.h
*
* Description:: This file contains all necessary functions prototypes
* and fileInfo Struct for use in IO layer.
*
**************************************************************/
#ifndef _FILEINFO_H
#define _FILEINFO_H


typedef struct fileInfo {
	char fileName[64];		//filename
	int fileSize;			//file size in bytes
	int location;			//starting lba (block number) for the file data
	int allocatedBlocks;		//Number of blocks currently allocated in FSM.
} fileInfo;

fileInfo * getFileInfo (char * filename, int flags);
int updateFileSizeOnDisk(char *pathname, int newSize, int lba);
int ensureAllocation(int fileSize, int location);
#endif