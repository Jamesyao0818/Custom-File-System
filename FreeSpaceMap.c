/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman
* Student IDs:: 924122742, tbd, tbd, tbd
* GitHub-Name:: ikligman
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: FreeSpaceMap.c
*
* Description:: This file contains all functions related to 
* free space management of the file system. We are using a
* bitmap to keep track of free space.
*
**************************************************************/

#include <stdlib.h>

// Global so it doesn't need to be read/write from disk every time
static char * fsm = NULL;

int allocate(int n, int blockSize);
static void setBit(int bitToSet);
static void clearBit(int bitToClear);
static int getBit(int bitToGet);

int initFSM(int blockSize) {
    fsm = malloc(5 * blockSize);

    // Mark all blocks free;
    memset(fsm, 0, (5 * blockSize));

    // VCB will be block 0, FSM blocks 1-5 inclusive, so mark first 6 blocks used
    for (int i = 0; i < 6; i++) {
        setBit(i);
    }

    // Commit changes to disk
    fsmWrite();

    return 1;
}

// Sets the given bit to 1
static void setBit(int bitToSet) {
    int byteNum = bitToSet / 8;
    int bitInByte = bitToSet % 8;

    int mask = 1 << bitInByte;

    fsm[byteNum] = fsm[byteNum] | mask;
}

// Sets the given bit to 0
static void clearBit(int bitToClear) {
    int byteNum = bitToClear / 8;
    int bitInByte = bitToClear % 8;

    int mask = 1 << bitInByte;
    mask = ~mask;

    fsm[byteNum] = fsm[byteNum] & mask;
}

// Returns the value of the given bit
static int getBit(int bitToGet) {
    int byteNum = bitToGet / 8;
    int bitInByte = bitToGet % 8;

    int mask = 1 << bitInByte;

    return (fsm[byteNum] & mask);
}

// Finds a grouping of n free blocks, returns starting block position
int allocate(int n, int blockSize) {
    int totalBits = 5 * 8 * blockSize;
    for(int i = 0; i < (totalBits - n); i++) {
        // Check if current bit is free
        if(getBit(i) == 0) {
            // Check if the following requested bits are also free
            int found = 1;
            for(int j = 1; j < n; j++) {
                if(getBit(i + j) == 1) {
                    found = 0;
                    // No need to recheck this section of blocks, skip
                    i = i + j;
                    break;
                }
            }
            if(found) {
                // Mark the allocated blocks as used
                for(int j = 0; j < n; j++) {
                    setBit(i + j);
                }
                return i;
            }
        }
    }
    return -1; // No free block group of requested size
}

// Allows the caller to commit fsm changes (that are in memory) to disk
void fsmWrite() {
    if(fsm != NULL) {
        // write 5 blocks to disk starting from block 1
        LBAwrite(fsm, 5, 1);
    }
}