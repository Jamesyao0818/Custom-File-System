/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Joseph Bowen & James Yao
* Student IDs:: 92296678 & 923978442
* GitHub-Name:: josephb333 & Jamesyao0818
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: fs_stat_simple.c
*
* Description:: Implementation of fs_stat() function declared
* in mfs.h. Works with ORIGINAL DE structure.
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mfs.h"       // For fs_stat declaration
#include "DE.h"        // For directory entry structure
#include "VCB.h"       // For getVCB()
#include "fsLow.h"     // For LBAread/LBAwrite

#define MAX_PATH_LENGTH 4096

// External functions from fs_directory.c (current working directory)
extern char* fs_getcwd(char *pathname, size_t size);

// Internal helper functions (static)
static int isAbsolutePath(const char *path);
static DE* loadDirectory(int blockNumber, int blockSize);
static DE* findInDirectory(DE *dir, const char *name, int numEntries);
static void extractFilename(const char *path, char *filename);
static void extractParentPath(const char *path, char *parentPath);
static int resolvePathToEntry(const char *path, DE **outEntry, DE **outParentDir, int *needFreeParent);

/**
 * fs_stat - Get file/directory statistics
 * Implements the function declared in mfs.h
 */
int fs_stat(const char *path, struct fs_stat *buf) {
    if (!path || !buf) {
        return -1;
    }
    
    // Get VCB for block size
    struct VCB *vcb = getVCB();
    if (!vcb) {
        return -1;
    }
    
    int blockSize = vcb->blockSize;
    DE* targetEntry = NULL;
    DE* parentDir = NULL;
    int needToFreeParent = 0;
    
    // Resolve the path to get the directory entry
    if (resolvePathToEntry(path, &targetEntry, &parentDir, &needToFreeParent) != 0) {
        return -1;
    }
    
    // Fill in the fs_stat structure
    memset(buf, 0, sizeof(struct fs_stat));
    
    // Size in bytes
    buf->st_size = targetEntry->numBytes;
    
    // Block size for I/O
    buf->st_blksize = blockSize;
    
    // Number of 512B blocks allocated
    // Calculate based on file size since we don't have numBlocks field
    int blocksNeeded = (targetEntry->numBytes + blockSize - 1) / blockSize;
    if (blocksNeeded == 0) blocksNeeded = 1; // At least 1 block
    
    int totalBytes = blocksNeeded * blockSize;
    buf->st_blocks = (totalBytes + 511) / 512;  // Convert to 512B blocks
    
    // Timestamps
    buf->st_accesstime = targetEntry->accessDate;
    buf->st_modtime = targetEntry->modifyDate;
    buf->st_createtime = targetEntry->createDate;
    
    // Clean up
    if (needToFreeParent && parentDir) {
        free(parentDir);
    }
    
    return 0;
}

// ========================================================================
// INTERNAL HELPER FUNCTIONS (static - not visible outside this file)
// ========================================================================

/**
 * Check if path is absolute (starts with /)
 */
static int isAbsolutePath(const char *path) {
    return (path && path[0] == '/');
}

/**
 * Load a directory from disk
 */
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

/**
 * Find an entry by name in a directory
 */
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

/**
 * Extract filename from path
 */
static void extractFilename(const char *path, char *filename) {
    if (!path || !filename) {
        return;
    }
    
    const char *lastSlash = strrchr(path, '/');
    
    if (lastSlash) {
        strcpy(filename, lastSlash + 1);
    } else {
        strcpy(filename, path);
    }
}

/**
 * Extract parent directory path
 */
static void extractParentPath(const char *path, char *parentPath) {
    if (!path || !parentPath) {
        return;
    }
    
    strcpy(parentPath, path);
    
    char *lastSlash = strrchr(parentPath, '/');
    
    if (lastSlash) {
        if (lastSlash == parentPath) {
            parentPath[1] = '\0';  // Root "/"
        } else {
            *lastSlash = '\0';
        }
    } else {
        strcpy(parentPath, ".");
    }
}

/**
 * Resolve a path to a directory entry
 */
static int resolvePathToEntry(const char *path, DE **outEntry, DE **outParentDir, int *needFreeParent) {
    if (!path || !outEntry || !outParentDir || !needFreeParent) {
        return -1;
    }
    
    struct VCB *vcb = getVCB();
    if (!vcb) {
        return -1;
    }
    
    int blockSize = vcb->blockSize;
    
    // Special case: root directory
    if (strcmp(path, "/") == 0) {
        *outParentDir = loadDirectory(vcb->blockIndexRootDirectory, blockSize);
        if (!*outParentDir) {
            return -1;
        }
        *outEntry = &(*outParentDir)[0];
        *needFreeParent = 1;
        return 0;
    }
    
    // Special case: current directory
    if (strcmp(path, ".") == 0) {
        char cwd[MAX_PATH_LENGTH];
        if (!fs_getcwd(cwd, sizeof(cwd))) {
            return -1;
        }
        
        // Get current directory block from path
        // For simplicity, if it's root, load root
        if (strcmp(cwd, "/") == 0) {
            *outParentDir = loadDirectory(vcb->blockIndexRootDirectory, blockSize);
            if (!*outParentDir) {
                return -1;
            }
            *outEntry = &(*outParentDir)[0];
            *needFreeParent = 1;
            return 0;
        }
        
        // Otherwise need to resolve current working directory
        // This is a simplification - in full implementation you'd track CWD block
        return -1;
    }
    
    // Extract parent path and filename
    char parentPath[MAX_PATH_LENGTH];
    char filename[256];
    extractParentPath(path, parentPath);
    extractFilename(path, filename);
    
    // If filename is empty, it's a directory path ending in /
    if (strlen(filename) == 0) {
        strcpy(filename, ".");
    }
    
    // Load parent directory
    DE* parentDir = NULL;
    
    if (strcmp(parentPath, "/") == 0) {
        // Parent is root
        parentDir = loadDirectory(vcb->blockIndexRootDirectory, blockSize);
    } else {
        // Need to resolve parent path
        // For simplicity, this implementation only handles simple paths
        // Full implementation would recursively resolve the path
        return -1;
    }
    
    if (!parentDir) {
        return -1;
    }
    
    // Find the file/directory entry in parent
    int numEntries = parentDir[0].numBytes / sizeof(DE);
    DE* entry = findInDirectory(parentDir, filename, numEntries);
    
    if (!entry) {
        free(parentDir);
        return -1;
    }
    
    *outEntry = entry;
    *outParentDir = parentDir;
    *needFreeParent = 1;
    
    return 0;
}