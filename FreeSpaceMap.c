/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman
* Student IDs:: 924122742
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

    // VCB will be physical block 1 logical block 0 (To account for block 0 being Professor Partition Table), FSM blocks 2-6 inclusive, so mark first 7 blocks used
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

// Used specifically for creation empty dirs/file i.e. single-block
void allocate(int tailLocation) {
    setBit(tailLocation);
}

// Allows the caller to commit fsm changes (that are in memory) to disk
void fsmWrite() {
    if(fsm != NULL) {
        // write 5 blocks to disk starting from block 1
        LBAwrite(fsm, 5, 1);
    }
}

// Function to read our FSM off disk.
int loadFSM (int blockSize, unsigned int startBlock){
    size_t numBytes = 5 * blockSize;
    if (fsm) { 
        free(fsm);
        fsm = NULL;
        return -1;
    }
    fsm = (char*)malloc(numBytes);
    if (!fsm) return -1;
    uint64_t blocksRead = LBAread(fsm, 5, startBlock);
    if (blocksRead != 5){
        free(fsm);
        fsm = NULL;
        return -1;
    }
    return 0;
}

// Access functions so other layers can use FSM.
int fsmReserveLBA(int lba){ 
    if (lba < 0 )
        return -1;
    setBit(lba); 
    fsmWrite(); 
    return 0; 
}
int fsmReleaseLBA(int lba){
    if ( lba < 0 )
        return -1;
    clearBit(lba); 
    fsmWrite(); 
    return 0; 
}
int fsmIsUsed(int lba){ 
    if ( lba < 0 )
        return 1;
    return getBit(lba); 
}