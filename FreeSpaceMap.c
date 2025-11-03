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
#include <string.h>
#include "fsLow.h"
#include "FSM.h"

// Global so it doesn't need to be read/write from disk every time
static char *fsm = NULL;

int initFSM(int blockSize) {
    fsm = malloc(5 * blockSize);

    // Mark all blocks free;
    memset(fsm, 0, (5 * blockSize));

    // VCB will be block 1 (To account for block 0 being Professor Partition Table), FSM blocks 2-6 inclusive, so mark first 7 blocks used
    for (int i = 0; i <= 6; i++) {
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

// Returns the bit position of the next free block, and marks it as used
// Gets free block
int allocate(int blockSize) {
    int totalBits = 5 * 8 * blockSize;
    for (int i = 0; i < totalBits; i++) {
        if(getBit(i) == 0) {
            setBit(i);
            return i;
        }
    }
    return -1; // No free blocks left
}

// Allows the caller to commit fsm changes (that are in memory) to disk
void fsmWrite() {
    if(fsm != NULL) {
        // write 5 blocks to disk starting from block 1
        LBAwrite(fsm, 5, 1);
    }
}