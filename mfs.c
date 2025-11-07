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
#include <string.h>

//Globals for access between functions.
DE *alreadyLoadedRootDir = NULL;
DE *alreadyLoadedCWD = NULL;

//IMPLEMENT THESE FUNCTIONS

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
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise

// Ian 
int fs_delete(char* filename);	//removes a file

// James
int fs_stat(const char *path, struct fs_stat *buf);


// Helper functions;
// Enrique
int findInDir(DE *dir, const char *name);
int isDEaDir();

// Joe
DE *loadDirectory(const DE *dirEntry){

}

// Ian
// Returns 0 for valid path, any non-zero number reflects a specific error.
int parsePath(char *pathName, parsePathInfo *ppi)
{ 
    // ppi is a buffer we will write to
    // Determining if this a relative or absolute path.
    DE *startParent;
    DE *parent;
    char *saveptr;
    char *token1;
    char *token2;
    if (pathName == NULL) return -1;
    if (pathName[0] == '/')
    { 
        // absolute path
        startParent = alreadyLoadedRootDir;
    } else {
        startParent = alreadyLoadedCWD;
    }
    // If path is just '/' obviously we are just at the root.
    parent = startParent;
    token1 = strtok_r(path, "/", &saveptr);
    if (token1 == NULL)
    {
        if (parent == alreadyLoadedRootDir)
        { 
            // i.e. path[0]="/""
            ppi->parent = parent;
            ppi->lastElementName = NULL;
            ppi->index = -2;
            return 0;
    } else { return -1;}
    }
    while (1)
    {
        int idx = FindInDirectory(parent, token1);
        token2 = strtok_r(NULL, "/", &saveptr);
        if (token2 == NULL)
        { // we are done, token 1 is last element
            ppi->parent = parent
            ppi->lastElementName = token1; // may need to do a strdup here depending
            ppi->index = idx;
            return 0;
        }
        if (idx == -1) return -2; // invalid path
        if (!isDEaDir(&parent[idx])) return -3; // invalid path (can't be file in middle)
        DE *tempParent = LoadDirectory(&parent[idx]);
        if (tempParent == NULL) return -4;
        if (parent != startParent) free(parent);
        parent = tempParent;
        token1 = token2;
    }
}
