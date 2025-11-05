/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Joseph Bowen & James Yao
* Student IDs:: 92296678 & 923978442
* GitHub-Name:: josephb333 & Jamesyao0818
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: mfscwd.c
*
* Description:: Implementation of fs_setcwd() and fs_getcwd()
* functions declared in mfs.h. This file ONLY uses mfs.h and
* your existing headers - no new dependencies required.
*
**************************************************************/

//mfscwd is mfs current working directory
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfs.h"
#include "DE.h"
#include "VCB.h"
#include "fsLow.h"

#define MAX_PATH_LENGTH 4096

// Global state for current working directory
static char currentPath[MAX_PATH_LENGTH] = "/";
static int currentDirBlock = -1;
static DE* currentDirectory = NULL;
static int fsBlockSize = 512;

// Internal helper functions (static - not exposed)
static int isAbsolutePath(const char *path);
static DE* loadDirectory(int blockNumber, int blockSize);
static DE* findInDirectory(DE *dir, const char *name, int numEntries);
static int resolvePathToBlock(const char *path, int *outBlock);

int initCWD(int rootDirBlock, int blockSize) {
    fsBlockSize = blockSize;
    currentDirBlock = rootDirBlock;
    strcpy(currentPath, "/");
    
    // Load root directory
    currentDirectory = loadDirectory(rootDirBlock, blockSize);
    if (!currentDirectory) {
        printf("Error: Failed to load root directory\n");
        return -1;
    }
    
    printf("CWD initialized: %s (block %d)\n", currentPath, currentDirBlock);
    return 0;
}

char* fs_getcwd(char *pathname, size_t size) {
    if (!pathname || size == 0) {
        return NULL;
    }
    
    if (strlen(currentPath) >= size) {
        return NULL;
    }
    
    strcpy(pathname, currentPath);
    return pathname;
}

int fs_setcwd(char *pathname) {
    if (!pathname || strlen(pathname) == 0) {
        return -1;
    }
    
    int targetBlock;
    
    // Special case: change to root
    if (strcmp(pathname, "/") == 0) {
        struct VCB* vcb = getVCB();
        if (!vcb) {
            return -1;
        }
        targetBlock = vcb->blockIndexRootDirectory;
        
        // Update current directory
        if (currentDirectory) {
            free(currentDirectory);
        }
        currentDirectory = loadDirectory(targetBlock, fsBlockSize);
        if (!currentDirectory) {
            return -1;
        }
        
        currentDirBlock = targetBlock;
        strcpy(currentPath, "/");
        return 0;
    }
    
    // Special case: stay in current directory
    if (strcmp(pathname, ".") == 0) {
        return 0;
    }
    
    // Special case: go to parent
    if (strcmp(pathname, "..") == 0) {
        if (!currentDirectory) {
            return -1;
        }
        
        // Get parent entry (..)
        if (currentDirectory[1].name[0] == '\0' || 
            strcmp(currentDirectory[1].name, "..") != 0) {
            return -1;
        }
        
        targetBlock = currentDirectory[1].blockNumber;
        
        // Load parent directory
        DE* parentDir = loadDirectory(targetBlock, fsBlockSize);
        if (!parentDir) {
            return -1;
        }
        
        // Update state
        free(currentDirectory);
        currentDirectory = parentDir;
        currentDirBlock = targetBlock;
        
        // Update path (remove last component)
        if (strcmp(currentPath, "/") != 0) {
            char *lastSlash = strrchr(currentPath, '/');
            if (lastSlash && lastSlash != currentPath) {
                *lastSlash = '\0';
            } else {
                strcpy(currentPath, "/");
            }
        }
        
        return 0;
    }
    
    // Resolve the path
    if (resolvePathToBlock(pathname, &targetBlock) != 0) {
        return -1;
    }
    
    // Load the new directory
    DE* newDir = loadDirectory(targetBlock, fsBlockSize);
    if (!newDir) {
        return -1;
    }
    
    // Verify it's a directory
    if (!newDir[0].isDir) {
        free(newDir);
        return -1;
    }
    
    // Update current directory
    if (currentDirectory) {
        free(currentDirectory);
    }
    currentDirectory = newDir;
    currentDirBlock = targetBlock;
    
    // Update path
    if (isAbsolutePath(pathname)) {
        strncpy(currentPath, pathname, MAX_PATH_LENGTH - 1);
        currentPath[MAX_PATH_LENGTH - 1] = '\0';
    } else {
        // Append to current path
        if (strcmp(currentPath, "/") != 0) {
            strncat(currentPath, "/", MAX_PATH_LENGTH - strlen(currentPath) - 1);
        }
        strncat(currentPath, pathname, MAX_PATH_LENGTH - strlen(currentPath) - 1);
    }
    
    return 0;
}

static int isAbsolutePath(const char *path) {
    return (path && path[0] == '/');
}

static DE* loadDirectory(int blockNumber, int blockSize) {
    if (blockNumber < 0) {
        return NULL;
    }
    
    DE* dir = (DE*)malloc(blockSize);
    if (!dir) {
        return NULL;
    }
    
    if (LBAread((char*)dir, 1, blockNumber) != 1) {
        free(dir);
        return NULL;
    }
    
    return dir;
}

static DE* findInDirectory(DE *dir, const char *name, int numEntries) {
    if (!dir || !name) {
        return NULL;
    }
    
    for (int i = 0; i < numEntries; i++) {
        if (dir[i].name[0] != '\0' && strcmp(dir[i].name, name) == 0) {
            return &dir[i];
        }
    }
    
    return NULL;
}

static int resolvePathToBlock(const char *path, int *outBlock) {
    if (!path || !outBlock) {
        return -1;
    }
    
    // Start from root or current directory
    DE* workingDir = NULL;
    int needToFree = 0;
    
    if (isAbsolutePath(path)) {
        // Start from root
        struct VCB* vcb = getVCB();
        if (!vcb) {
            return -1;
        }
        workingDir = loadDirectory(vcb->blockIndexRootDirectory, fsBlockSize);
        needToFree = 1;
    } else {
        // Start from current directory
        if (!currentDirectory) {
            return -1;
        }
        workingDir = currentDirectory;
        needToFree = 0;
    }
    
    if (!workingDir) {
        return -1;
    }
    
    // Make a copy of path for tokenizing
    char pathCopy[MAX_PATH_LENGTH];
    strncpy(pathCopy, path, MAX_PATH_LENGTH - 1);
    pathCopy[MAX_PATH_LENGTH - 1] = '\0';
    
    // Skip leading slash
    char *start = pathCopy;
    if (start[0] == '/') {
        start++;
    }
    
    // If path is just "/", return root block
    if (strlen(start) == 0) {
        *outBlock = workingDir[0].blockNumber;
        if (needToFree) free(workingDir);
        return 0;
    }
    
    // Tokenize by /
    char *token = strtok(start, "/");
    
    while (token != NULL) {
        // Skip "."
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }
        
        // Handle ".."
        if (strcmp(token, "..") == 0) {
            int numEntries = workingDir[0].numBytes / sizeof(DE);
            DE* parentEntry = findInDirectory(workingDir, "..", numEntries);
            
            if (!parentEntry) {
                if (needToFree) free(workingDir);
                return -1;
            }
            
            int parentBlock = parentEntry->blockNumber;
            
            if (needToFree) free(workingDir);
            
            workingDir = loadDirectory(parentBlock, fsBlockSize);
            needToFree = 1;
            
            if (!workingDir) {
                return -1;
            }
            
            token = strtok(NULL, "/");
            continue;
        }
        
        // Find the entry
        int numEntries = workingDir[0].numBytes / sizeof(DE);
        DE* entry = findInDirectory(workingDir, token, numEntries);
        
        if (!entry) {
            if (needToFree) free(workingDir);
            return -1;
        }
        
        // Check if it's a directory
        if (!entry->isDir) {
            if (needToFree) free(workingDir);
            return -1;
        }
        
        int nextBlock = entry->blockNumber;
        
        if (needToFree) free(workingDir);
        
        workingDir = loadDirectory(nextBlock, fsBlockSize);
        needToFree = 1;
        
        if (!workingDir) {
            return -1;
        }
        
        token = strtok(NULL, "/");
    }
    
    // Found the directory
    *outBlock = workingDir[0].blockNumber;
    
    if (needToFree) free(workingDir);
    
    return 0;
}