/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman, Joseph Bowen, Cheng-Chieh Yao, Enrique Cabral-Mixco
* Student IDs:: 924122742, 922966782 , 923978442, 924056104
* GitHub-Name:: ikligman, josephb333, Jamesyao0818, rommix408
* Group-Name:: Don't Worry We Don't Byte.
* Project:: Basic File System
*
* File:: mfs.c
*
* Description:: 
*	Function implementations for milestone two.
*
**************************************************************/

#include "mfs.h"
#include "VCB.h"
#include "fsLow.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//Globals for access between functions.
DE *alreadyLoadedRootDir = NULL;
DE *alreadyLoadedCWD = NULL;

//IMPLEMENT THESE FUNCTIONS
int findInDirectory(DE * directory, char * token) {
    // input validation
    if (directory == NULL || token == NULL) {
        return -1;
    }

    // calculate number of entries based on numBytes (which may change)
    int numEntries = directory[0].numBytes / sizeof(DE);

    // iterate through each used entry, if it matches that is the index!
    for (int i = 0; i < numEntries; i++) {
        if (!strcmp(directory[i].name, token)) {
            return i;
        }
    }

    // not found
    return -1;
}

// Key directory functions
// Ian
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Enrique
// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Joe 
// Misc directory functions
char * fs_getcwd(char *pathname, size_t size){
    /*// /Home/Documents/HW/csc..filesystem/
    if (!pathname || size == 0)
        return NULL;
    if (!alreadyLoadedCWD || !alreadyLoadedRootDir)
        return NULL;
    if (alreadyLoadedCWD[0].blockNumber == alreadyLoadedRootDir[0]){
        if (size < 2) return NULL;
        pathname[0] = '/';
        pathname[1] = '/0';
        return pathname;
    }
    // csc-filesystem...
    // Home/..
    // ^ length of this
    // add to start buffer becasue just a pointer
    // move lenght into that 
    // /Documents
    // ./../foo/bar/bla/.. 
    // Home/bar
    char *buf = malloc(size);
    memset(buf, 0, size);
    DE * currentDir = alreadyLoadedCWD; 
    strcat(buf, alreadyLoadedCWD->name);
    // csc-filesystem
    strcat(buf, "/");
    // csc-filesystem/
    while (currentDir[1].name != alreadyLoadedRootDir->name){
        strcat(buf, currentDir[1].name);
        // csc-filesystem/HW
        strcat(buf, "/");
        // csc-filesystem/HW/
        currentDir = currentDir[1];
        strcat(buf, currentDir[1].name);
        // csc-filesystem/HW/Documents
        strcat(buf, "/");
        // csc-filesystem/HW/Documents/
    }
    char ** arrayOfSubStrings = malloc(size * 8);
    int index = 0;
    if (arrayOfSubStrings[i] != "." || arrayOfSubStrings[i] != ".." ) index++;
    char * token = strtok(buf, "/");
    // "/" "csc-filesystem" "/" .....
    while (!token){
        arrayOfSubStrings[index] = token;
    }

    // buffer cscfilesystem/HW/Documents/Home/
    // buffer cscfilesystem*HW*Documents*Home*/
    //return buf;
    return NULL; 
}

//linux chdir
int fs_setcwd(char *pathname){
    if (!pathname || strlen(pathname) == 0) return 0;
    parsePathInfo ppi;
    if (parsePath(pathname, &ppi) != 0); 
    printf("Error in fs_setcwd: Invalid path\n");
    if (fs_isFile(ppi.parent->name) == 0)
    alreadyLoadedCWD = ppi.parent;
}   

// Returns 1 if file, 0 otherwise;
int fs_isFile(char * filename){
    if (!filename || strlen(filename) == 0) return 0;
    parsePathInfo ppi;
    if (parsePath(filename, &ppi) != 0); 
    printf("Error in fs_isFile: Invalid path\n");
    int index = findInDir(ppi.parent, filename);
    if (ppi.parent[index].isDir != 1) return 1;
    return 0;
}

// Returns 1 if directory, 0 otherwisepathname
int fs_isDir(char * pathname){
    if (!pathname || strlen(pathname) == 0) return 0;
    parsePathInfo ppi;
    if (parsePath(pathname, &ppi) != 0); 
    printf("Error in fs_isDir: Invalid path\n");
    return ppi.parent->isDir;
}

// Ian 
int fs_delete(char* filename);	//removes a file

// James
int fs_stat(const char *path, struct fs_stat *buf){
    if(path == NULL || buf == NULL){
        return -1; // Invalid arguments
    }

    parsePathInfo ppi;
    int parseResult = parsePath((char *)path, &ppi);
    if(parseResult != 0){
        return -2; // Path parsing error
    }

    DE *targetDE;
    if(ppi.index >= 0){
        targetDE = &ppi.parent[ppi.index];
    } else {
        return -3; // File or directory not found
    }

    buf->st_size = (off_t)targetDE->numBytes;
    struct VCB* vcb = getVCB();
    buf->st_blksize = (blksize_t)vcb->blockSize;
    buf->st_blocks = (blkcnt_t)((targetDE->numBytes + 511) / 512); // Number of 512B blocks allocated
    buf->st_accesstime = targetDE->accessDate;
    buf->st_modtime = targetDE->modifyDate;
    buf->st_createtime = targetDE->createDate;
    return 0; // Success
}

// Joe
DE *loadDirectory(const DE *dirEntry){
    if (!dirEntry){
        printf("Error: LoadDirectory called a NULL DE.\n");
        return NULL;
    }
    if (dirEntry->isDir != 1){
        printf("Error: LoadDirectory called on DE that is not a directory.\n");
    }
    // Use VCB to get block size.
    struct VCB* v = getVCB();
    int blockSize = v->blockSize;
    // Calculate how many blocks we need to read for this directory.
    int bytesToRead = (int)dirEntry->numBytes;
    int blocksToRead = (bytesToRead + (blockSize - 1)) / blockSize;
    int bytesToMalloc = blocksToRead * blockSize;
    if (bytesToRead <= 0){
        printf("Error: Invalid numBytes (%d) in DE\n", bytesToRead);
        return NULL;
    }
    // Allocate memory for directory contents
    char *buffer = (char *)malloc(blockSize * blocksToRead);
    if (!buffer){
        printf("Error: malloc failue in LoadDirectory().\n");
    }int isDEaDir();
    // Convert from logical block index to physical
    // Double check this part
    int physBlock = dirEntry->blockNumber - 1;
    // Make sure we also handle discontiguous instances. Pending James's Linked List implementation.
    u_int64_t blocksRead = LBAread(buffer, blocksToRead, physBlock);
    if (blocksRead != (u_int64_t)blocksToRead) {
        printf("Error: LoadDirectory failed to read: %d blocks (needed to read: %lu blocks).\n", blocksToRead, blocksRead);
        free (buffer);
        return NULL;
    }
    // If read was done correctly, now we can just return the raw bytes of
    // the dirEntry we want to load. Should work.
    return (DE *)buffer;
    // IN CASE THIS DOES NOT WORK TRY THIS.
    /*
    int numEntries = bytesToRead / sizeof(DE);
    DE *dirArray = malloc(numEntries * sizeof(DE));
    memcpy(dirArray, buffer, numEntries * sizeof(DE));
    free(buffer);
    return dirArray;
    */
}

// Ian
// Returns 0 for valid path, any non-zero number reflects a specific error.
int parsePath(char * pathName, parsePathInfo * ppi) { // ppi is a buffer we will write to
    // is this a relative or absolute path?
    DE * startParent;
    DE * parent;
    char * saveptr;
    char * token1;
    char * token2;

    if (pathName == NULL)
        return -1;
    if (pathName[0] == '/') { // absolute path
        startParent = alreadyLoadedRootDir;
    }
    else {
        startParent = alreadyLoadedCWD;
    }
    parent = startParent;

    token1 = strtok_r(pathName, "/", &saveptr);
    if (token1 == NULL) {
        if (parent == alreadyLoadedRootDir) { // i.e. path[0]='/'
            ppi->parent = parent;
            ppi->lastElementName = NULL;
            ppi->index = -2;
            return 0;
        }
        else {
            return -1;
        }
    }

    while (1)
    {
        int idx = findInDirectory(parent, token1);
        token2 = strtok_r(NULL, "/", &saveptr);
        if (token2 == NULL) { // we are done, token 1 is last element
            ppi->parent = parent;
            ppi->lastElementName = token1; // may need to do a strdup here depending
            ppi->index = idx;
            return 0;
        }
        if (idx == -1) {
            return -2; // invalid path
        } 

        if (!isDEaDir(&parent[idx])) {
            return -3; // invalid path (can't be file in middle)
        } 
        DE *tempParent = loadDirectory(&parent[idx]);

        if (tempParent == NULL) {
            return -4; // failed to load dir
        }

        if (parent != startParent) {
            free(parent);
        }
        parent = tempParent;
        token1 = token2;
    }
}

int fs_mkdir(const char *pathname, mode_t mode) {
    // working copy of string
    char * pathCopy = strdup(pathname);

    // get ppi
    parsePathInfo ppi;
    if (parsePath(pathCopy, &ppi) != 0) {
        return -1;
    }

    // get vcb for blockSize
    struct VCB * vcb = getVCB();
    
    DE * newDir = createDir(2, ppi.parent, vcb->blockSize);

    // update the new dir's . entry
    strcpy(newDir[0].name, ppi.lastElementName);

    // find a free index to put this entry in the parent
    int freeIndex = -1;
    int numEntries = ppi.parent[0].numBytes / sizeof(DE);
    for (int i = 2; i < numEntries; i++) {
        if (ppi.parent[i].name[0] == '\0') {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex == -1) {
        return -2; // parent has no more room for entries
    }

    time_t now = time(NULL);

    // add entry to parent's free directory entry index
    strcpy(ppi.parent[freeIndex].name, ppi.lastElementName);
    // update the parent's directory entry of this new dir
    ppi.parent[freeIndex].isDir = 1;
    ppi.parent[freeIndex].numBytes = newDir[0].numBytes;
    ppi.parent[freeIndex].blockNumber = newDir[0].blockNumber;
    ppi.parent[freeIndex].loc = newDir[0].loc;
    ppi.parent[freeIndex].createDate = now;
    ppi.parent[freeIndex].accessDate = now;
    ppi.parent[freeIndex].modifyDate = now;

    // this modified the parent, so update its mod time
    ppi.parent[0].modifyDate = now;

    // commit changes to disk
    writeDirToDisk(ppi.parent, vcb->blockSize);
    writeDirToDisk(newDir, vcb->blockSize);
    
    free(newDir);
    free(pathCopy);

    if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
        free(ppi.parent);
    }

    return 0;
}