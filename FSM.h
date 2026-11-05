/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman Joseph Bowen
* Student IDs:: 924122742 922966782
* GitHub-Name:: ikligman josephb333
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: FSM.h
*
* Description:: This file contains all necessary functions prototypes
* for the free space map of the file system.
*
**************************************************************/

#ifndef _FSM_H
#define _FSM_H

//Function Declarations.
void fsmWrite();
void allocate(int tailLocation);
int initFSM(int blockSize);
static void setBit(int bitToSet);
static void clearBit(int bitToClear);
static int getBit(int bitToGet);
int fsmReserveLBA(int lba);
int fsmReleaseLBA(int lba);
int fsmIsUsed(int lba);
int loadFSM (int blockSize, unsigned int startBlock);
int fsmAllocateBlock(int numBlocks);
#endif