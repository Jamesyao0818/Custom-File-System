#ifndef _FSM_H
#define _FSM_H
//Function Declarations.
void fsmWrite();
int allocate(int blockSize);
int initFSM(int blockSize);
static void setBit(int bitToSet);
static void clearBit(int bitToClear);
static int getBit(int bitToGet);

#endif