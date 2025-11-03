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
    
    // For discontiguous storage, we need to track the chain of blocks
    int loc;  // Starting location in block allocation structure
} DE;

//Function Declarations
void writeDirToDisk(const DE *dir, int blockSize);
DE* createDir(int minEntries, DE *parent, int blockSize);

#endif