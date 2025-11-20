/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman
* Student IDs:: 924122742
* GitHub-Name:: ikligman
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
u_int64_t allocate(int blockSize, int numBytes);
int initFSM(int blockSize);
static void setBit(int bitToSet);
static void clearBit(int bitToClear);
static int getBit(int bitToGet);
void releaseBlock(int blocknum);
void markBlockUsed(int blocknum);
int fsmReserveLBA(int lba);
int fsmReleaseLBA(int lba);
int fsmIsUsed(int lba);
int loadFSM (int blockSize, unsigned int startBlock);
#endif