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

} DE;

//Globals for access between functions.
extern DE *alreadyLoadedRootDir;
extern DE *alreadyLoadedCWD;

//Function Declarations
void writeDirToDisk(const DE *dir, int blockSize);
DE* createDir(int childLBA, int parentLBA, int minEntries, DE * parent, int blockSize);


#endif