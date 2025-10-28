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

void setBit(char * fsm, int bitToSet);
void clearBit(char * fsm, int bitToSet);
int getBit(char * fsm, int bitToSet);

int initFSM() {
    char * fsm = malloc(2560); // (n*blocksize, 5*512, hard coded for now)

    
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