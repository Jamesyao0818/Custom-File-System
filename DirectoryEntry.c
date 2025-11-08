    /**************************************************************
    * Class::  CSC-415-01 Fall 2025
    * Name:: Joseph Bowen & James Yao
    * Student IDs:: 92296678 & 923978442
    * GitHub-Name:: josephb333 & Jamesyao0818
    * Group-Name:: Don't Worry We Don't Byte
    * Project:: Basic File System
    *
    * File:: DirectoryEntry.c
    *
    * Description:: This file contains all functions related to 
    * creating root directory and subsequent directories,
    * as well as writing them to the disk.
    *
    **************************************************************/

    #include <time.h>
    #include <stdlib.h>
    #include <string.h>
    #include "fsLow.h"
    #include "FSM.h"
    #include "DE.h"

    DE* createDir(int minEntries, DE * parent, int blockSize) {
        //4a) How much memory neeeded
        //4b) Number of entries wanted is minEntries parameter.
        //4c) Multiply size of your directory entry by number of entry to get number of bytes needed.
        // Make sure we never have less then 2 directories.
        if (minEntries < 2) { minEntries = 2;}
        int bytesNeeded = minEntries * (int)sizeof(DE);
        //4d) How many blocks needed.
        int blocksNeeded = (bytesNeeded + (blockSize - 1)) / blockSize;
        int bytesToMalloc = blocksNeeded * blockSize;
        // How many entries could fit in that space
        int actualEntriesCount = bytesToMalloc / (int)sizeof(DE);
        if (blocksNeeded != 1) {
            //Multiple Block Dirs not implemented yet.
            return NULL;
        }

        DE * dir = (DE *)malloc(bytesToMalloc);
        if (!dir) { return NULL;}
        //Make clean slate.
        memset(dir, 0, bytesToMalloc);
        
        //4e) Initiliaze entries
        // Initialize all entries as empty
        for (int i = 2; i < actualEntriesCount; i++) {
            dir[i].name[0] = '\0';
            dir[i].isDir = 0;
            dir[i].numBytes = 0;
            dir[i].blockNumber = -1;
            dir[i].loc = -1;
        }
        
        time_t now = time(NULL);
        //4f) Allocate blocks (one block for root directory)
        int rootPhys = allocate(blockSize);
        if (rootPhys < 0) { free (dir); return NULL;}

        // Setup "." entry (current directory)
        strcpy(dir[0].name, ".");
        dir[0].numBytes = actualEntriesCount * (int)sizeof(DE);
        dir[0].isDir = 1;
        dir[0].createDate = now;
        dir[0].modifyDate = now;
        dir[0].accessDate = now;
        dir[0].loc = rootPhys;
        dir[0].blockNumber = rootPhys;
        
        // Setup ".." entry (parent directory)
        if (!parent) {
            // Root directory - parent points to itself
            parent = dir;
        }

        //4h)
        int parentPhys = parent->blockNumber;
        
        strcpy(dir[1].name, "..");
        dir[1].numBytes = parent->numBytes;
        dir[1].isDir = 1;
        dir[1].createDate = dir[0].createDate;
        dir[1].modifyDate = dir[0].modifyDate;
        dir[1].accessDate = dir[0].accessDate;
        dir[1].blockNumber = parentPhys;
        dir[1].loc = parentPhys;
        //4i)
        writeDirToDisk(dir, blockSize);
        //4j)
        return dir;
    }

    void writeDirToDisk(const DE * dir, int blockSize) {
        //This version of writing directory is only writing one block.
        //For next milestone we will implement linked functionality to make it discontiguous.
        //Remember to refactor this function LATER!!!!
        int blocksToWrite = (dir[0].numBytes + blockSize - 1) / blockSize; // Should just be one.
        int blockIndex = dir[0].blockNumber;
        //printf("Dir block number: %d\n", dir[0].blockNumber);
        LBAwrite((char*)dir, 1, dir->blockNumber - 1);
        /*for (int i = 0; i < blocksToWrite; i++){
            //int blockIndex = getNthBlockOfChain(dir->loc,i);
            
            LBAwrite((char*)dir + i*blockSize, 1, blockIndex);
            //char * [numBytes...loc....blocknumber...]
        }*/
        fsmWrite();
    }

    void writeDirToDiskDiscontiguous(const DE *dir, int blockSize, void* blockList) {
        if (!blockList || ll_isEmpty(blockList)) {
            return;
        }
    
        int numBlocks = ll_size(blockList);
        int totalBytes = numBlocks * blockSize;
        int blocksToWrite = (dir[0].numBytes + blockSize - 1) / blockSize;
        if (blocksToWrite > numBlocks) {
            // Not enough blocks allocated
            return;
        }
    
        // Write each block from the chain
        for (int i = 0; i < blocksToWrite; i++) {
            int blockNum = getNthBlockOfChain(blockList, i);  // ← Gets actual block number
            char *dataPtr = (char*)dir + (i * blockSize);
            LBAwrite(dataPtr, 1, blockNum);
        }
    
        fsmWrite();
    }

    DE* readDirFromDiskDiscontiguous(int startBlock, int numBlocks, int blockSize){
        if (numBlocks <= 0) {
            return NULL;
        }
    
        int bytesToRead = numBlocks * blockSize;
        DE *dir = (DE *)malloc(bytesToRead);
        if (!dir) {
            return NULL;
        }
    
        // Read each block from the chain
        for (int i = 0; i < numBlocks; i++) {
            int blockNum = startBlock + i;  // Assuming contiguous for simplicity
            char *dataPtr = (char*)dir + (i * blockSize);
            LBAread(dataPtr, 1, blockNum);
        }
    
        return dir;
    }

    DE* createDirDiscontiguous(int minEntries, DE *parent, int blockSize){
        //4a) How much memory neeeded
        //4b) Number of entries wanted is minEntries parameter.
        //4c) Multiply size of your directory entry by number of entry to get number of bytes needed.
        // Make sure we never have less then 2 directories.
        if (minEntries < 2) { minEntries = 2;}
        int bytesNeeded = minEntries * (int)sizeof(DE);
        //4d) How many blocks needed.
        int blocksNeeded = (bytesNeeded + (blockSize - 1)) / blockSize;
        int bytesToMalloc = blocksNeeded * blockSize;
        // How many entries could fit in that space
        int actualEntriesCount = bytesToMalloc / (int)sizeof(DE);

        DE * dir = (DE *)malloc(bytesToMalloc);
        if (!dir) { return NULL;}
        //Make clean slate.
        memset(dir, 0, bytesToMalloc);
        
        //4e) Initiliaze entries
        // Initialize all entries as empty
        for (int i = 2; i < actualEntriesCount; i++) {
            dir[i].name[0] = '\0';
            dir[i].isDir = 0;
            dir[i].numBytes = 0;
            dir[i].blockNumber = -1;
            dir[i].loc = -1;
        }
        
        time_t now = time(NULL);
        //4f) Allocate blocks (one block for root directory)
        void* blockList = ll_create();
        if (!blockList) { free(dir); return NULL;}
        for (int i = 0; i < blocksNeeded; i++) {
            int physBlock = allocate(blockSize);
            if (physBlock < 0) {
                ll_destroy(blockList);
                free(dir);
                return NULL;
            }
            ll_addLast(blockList, physBlock);
        }

        // Setup "." entry (current directory)
        strcpy(dir[0].name, ".");
        dir[0].numBytes = actualEntriesCount * (int)sizeof(DE);
        dir[0].isDir = 1;
        dir[0].createDate = now;
        dir[0].modifyDate = now;
        dir[0].accessDate = now;
        dir[0].loc = getNthBlockOfChain(blockList, 0);
        dir[0].blockNumber = getNthBlockOfChain(blockList, 0);
        // Setup ".." entry (parent directory)
        if (!parent) {
            // Root directory - parent points to itself
            parent = dir;
        }
        int parentPhys = parent->blockNumber;
        strcpy(dir[1].name, "..");
        dir[1].numBytes = parent->numBytes;
        dir[1].isDir = 1;
        dir[1].createDate = dir[0].createDate;
        dir[1].modifyDate = dir[0].modifyDate;
        dir[1].accessDate = dir[0].accessDate;
        dir[1].blockNumber = parentPhys;
        dir[1].loc = parentPhys;
        //4i)
        writeDirToDiskDiscontiguous(dir, blockSize, blockList);
        ll_destroy(blockList);
        //4j)
        return dir;
    }