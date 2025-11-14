/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Joseph Bowen & James Yao
* Student IDs:: 92296678
* GitHub-Name:: josephb333 & Jamesyao0818
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: DE.h
*
* Description:: This file contains struct layout for our 
* directory entries, necessary inclusion of time.h for time_t variables,
* and necessary function prototypes.
*
**************************************************************/
#ifndef _DE_H
#define _DE_H
#include <time.h>

typedef struct LinkedList LinkedList;
//Directory Entry Struct
typedef struct DE {
    // Starting head of the file's location (for discontiguous files)
    int blockNumber;
    
    // File name        
    char name[32];  
    
    // File size in bytes
    unsigned int numBytes;
    
    // Flag to distinguish directory vs file
    int isDir;  // 1 for directory, 0 for file
    
    // Relevant Dates and Times (using time_t instead of char arrays)
    time_t createDate;
    time_t modifyDate;
    time_t accessDate;
    
    // For discontiguous storage, we need to track the chain of blocks
    int loc;  // Starting location in block allocation structure

    LinkedList* blockList; // List of blocks allocated to this directory or file
} DE;

//Function Declarations
void writeDirToDisk(const DE *dir, int blockSize);
DE* createDir(int childLBA, int parentLBA, int minEntries, DE * parent, int blockSize);

//New function prototypes for discontiguous directory handling
//blocklist-> linked list or array of block numbers allocated to the directory
void writeDirToDiskDiscontiguous(const DE *dir, int blockSize, LinkedList* blockList);
//Read directory from disk given starting block and number of blocks
DE* readDirFromDiskDiscontiguous(LinkedList* blockList, int blockSize);
//Create directory that can span multiple non-contiguous blocks
DE* createDirDiscontiguous(int minEntries, DE *parent, int blockSize);
// Joe & Ian
DE* Touch(int childLBA, DE * parent, char * name);

#endif