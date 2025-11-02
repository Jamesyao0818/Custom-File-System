/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Joseph Bowen & James Yao
* Student IDs:: 92296678
* GitHub-Name:: josephb333
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: DirectoryEntry.c
*
* Description:: This file contains all functions related to 
* creating root directory and subsequent directories,
* as well as writing them to the disk.
*
**************************************************************/

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "fsLow.h"
#include "FSM.h"
#include "DE.h"

/*
4) Initialize the Root Directory
a. First again, you need memory – how much?
b. Decide how many Directory Entries (DE) you want for a directory (at least an
initial amount). I will use 50 for this example.
c. Now multiply the size of your directory entry by the number of entries. For
example, if my directory entry is 60 bytes, I would multiply 60 * 50 = 3000 bytes
needed.
d. Now you need to determine how many blocks you need. In the example that
would be 6 blocks. But wait.... 6 blocks is 3072 bytes and I am only using 3000
bytes, and my directo
void fsmWrite();
int allocate(int blockSize);
int initFSM(int blockSize);
static void setBit(int bitToSet);
static void clearBit(int bitToClear);
static int getBit(int bitToGet);
ry entry is only 60 bytes, so I can actually fit 51 directory
entries in the space of 3072 bytes (now only wasting 12 bytes instead of 72).
e. Now you have a pointer to an array of directory entries. Loop through them (in
our case for (i = 0; i < 51; i++) and initialize each directory entry structure to be in
a known free state.
f. Now ask the free space system for 6 blocks. It should return to you the starting
block number for those 6 blocks.
g. Now set the first directory entry to the “.” Directory. So, the name is “.”, the size
is 3060 bytes, it starts at block number (whatever the free space system
returned above), the time stamps are the current time, it is a directory, etc.
h. Now set the second directory entry to “..”, but since this is the root, it is the
same as the “.” except the name is “..” rather than “.”
i. Now write the root directory – 6 blocks starting from block (what was returned
from the free space allocation).
j. Return the starting block number of the root directory (or set it in the VCB
yourself
*/
DE* createDir(int minEntries, DE * parent, int blockSize) {
    //4a) How much memory neeeded
    //4b) Number of entries wanted is minEntries parameter.
    //4c) Multiply size of your directory entry by number of entry to get number of bytes needed.
    // Make sure we never have less then 2 directories.
    if (minEntries < 2) { minEntries = 2;}
    int bytesNeeded = minEntries * sizeof(DE);
    //4d) How many blocks needed.
    int blocksNeeded = (bytesNeeded + (blockSize - 1)) / blockSize;
    int bytesToMalloc = blocksNeeded * blockSize;
    // How many entries could fit in that space
    int actualEntriesCount = bytesToMalloc / sizeof(DE);

    DE * dir = (DE *)malloc(bytesToMalloc);
    if (!dir) { return NULL;}

    //Make clean slate.
    memset(dir, 0, bytesToMalloc);
    //4e) Initiliaze entries
    // Initialize all entries as empty
    for (int i = 2; i < actualEntriesCount; i++) {
        dir[i].name[0] = '\0';
        dir[i].isDir = 0;
        dir[i].numBytes = 0;
        dir[i].blockNumber = -1;
        dir[i].loc = -1;
    }
    
    time_t curDT = time(NULL);
    //4f) Allocate blocks.
    int startLoc = allocate(blockSize);
    // Setup "." entry (current directory)
    strcpy(dir[0].name, ".");
    dir[0].numBytes = actualEntriesCount * sizeof(DE);
    dir[0].isDir = 1;
    dir[0].createDate = curDT;
    dir[0].modifyDate = curDT;
    dir[0].accessDate = curDT;
    dir[0].loc = startLoc;
    dir[0].blockNumber = startLoc;
    
    // Setup ".." entry (parent directory)
    if (parent == NULL) {
        // Root directory - parent points to itself
        parent = dir;
    }

    //4h)
    strcpy(dir[1].name, "..");
    dir[1].numBytes = parent->numBytes;
    dir[1].loc = parent->loc;
    dir[1].blockNumber = parent->blockNumber;
    dir[1].createDate = parent->createDate;
    dir[1].modifyDate = parent->modifyDate;
    dir[1].accessDate = parent->accessDate;
    dir[1].isDir = parent->isDir;
    //4i)
    writeDirToDisk(dir, blockSize);
    //4j)
    return dir;
}

void writeDirToDisk(const DE * dir, int blockSize) {
    //This version of writing directory to disk will make it contigous.
    //For next milestone we will implement linked functionality to make it discontiguous.
    //Remember to refactor this function LATER!!!!
    //Returns starting index of first free block
    int blocksToWrite = (dir[0].numBytes + blockSize - 1) / blockSize;
    for (int i = 0; i < blocksToWrite; i++){
        //int blockIndex = getNthBlockOfChain(dir->loc,i);
        int blockIndex = allocate(blockSize);
        LBAwrite((char*)dir + i*blockSize, 1, blockIndex);
        //char * [numBytes...loc....blocknumber...]
    }
}