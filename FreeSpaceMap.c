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

char * allocate(int blockSize);
void setBit(char * fsm, int bitToSet);
void clearBit(char * fsm, int bitToSet);
int getBit(char * fsm, int bitToSet);

int initFSM(int blockSize) {
    char * fsm = allocate(blockSize);

    // Mark all blocks free;
    memset(fsm, 0, (5 * blockSize));

    // VCB will be block 0, FSM blocks 1-5 inclusive, so mark first 6 blocks used
    for (int i = 0; i < 6; i++) {
        setBit(fsm, i);
    }

    // write 5 blocks to disk starting from block 1
    LBAwrite(fsm, 5, 1);

    return 1;
}

void setBit(char * fsm, int bitToSet) {
    int byteNum = bitToSet / 8;
    int bitInByte = bitToSet % 8;

    int mask = 1 << bitInByte;

    fsm[byteNum] = fsm[byteNum] | mask;
}

void clearBit(char * fsm, int bitToSet) {
    int byteNum = bitToSet / 8;
    int bitInByte = bitToSet % 8;

    int mask = 1 << bitInByte;
    mask = ~mask;

    fsm[byteNum] = fsm[byteNum] & mask;
}

int getBit(char * fsm, int bitToSet) {
    int byteNum = bitToSet / 8;
    int bitInByte = bitToSet % 8;

    int mask = 1 << bitInByte;

    return (fsm[byteNum] & mask);
}

char * allocate(int blockSize) {
    char * fsm = malloc(5 * blockSize);
    return fsm;
}