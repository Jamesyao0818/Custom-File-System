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
// Opens a directory for reading its contents. This function prepares the
// directory for iteration by loading it into memory or using a cached
// version and creating a directory descriptor fdDirthat tracks our
// position as we read through entries.
fdDir * fs_opendir(const char *pathname){
    // We need to check for NULL before calling strlen, which would crash
    // if pathname is NULL. parsePath also checks for NULL, but we need
    // to validate here first since we use strlen before calling parsePath.
    if (pathname == NULL) {
        return NULL;
    }
    
    // parsePath uses strtok_r which modifies the input string, so we need
    // to create a modifiable copy. This prevents corrupting the original
    // pathname that the caller provided.
    char *pathCopy = (char *)malloc(strlen(pathname) + 1);
    if (pathCopy == NULL) {
        return NULL;
    }
    strcpy(pathCopy, pathname);
    
    // Parse the pathname to break it down into components and find the
    // target directory entry. The parsePathInfo structure will contain
    // information about where the directory is located.
    parsePathInfo ppi;
    int parseResult = parsePath(pathCopy, &ppi);
    free(pathCopy); // Clean up our temporary copy
    if (parseResult != 0) {
        return NULL; // Path doesn't exist or is invalid
    }
    
    DE *targetDir = NULL;
    int isCached = 0; // Track whether we're using a cached directory
    
    // When index is -2, it means we're opening the parent directory itself
    // like opening "/" for root. This is how parsePath indicates that
    // the path refers to a directory rather than something inside it.
    if (ppi.index == -2) {
        // Check if the parent directory is already loaded in memory
        if (ppi.parent != NULL) {
            // Use the already-loaded directory either cached or loaded
            // by parsePath during path traversal
            targetDir = ppi.parent;
            // Mark as cached if it matches our global cache pointers
            if (targetDir == alreadyLoadedRootDir || 
                targetDir == alreadyLoadedCWD) {
                isCached = 1;
            }
        } else {
            // Root directory hasn't been loaded yet, so we need to read
            // it from disk. We get the block location from the VCB
            // (Volume Control Block) which stores filesystem metadata.
            struct VCB* vcb = getVCB();
            if (vcb == NULL) {
                return NULL;
            }
            
            // Read the root directory block from disk using LBAread
            // (Logical Block Address read). The root directory is stored
            // at a specific block location defined in the VCB.
            int blockSize = vcb->blockSize;
            int rootBlock = vcb->blockIndexRootDirectory;
            char *buffer = (char *)malloc(blockSize);
            if (buffer == NULL) {
                return NULL;
            }
            
            u_int64_t blocksRead = LBAread(buffer, 1, rootBlock);
            if (blocksRead != 1) {
                free(buffer);
                return NULL;
            }
            
            // Cast the buffer to a directory entry array and cache it
            // for future use (so we don't have to read from disk again)
            targetDir = (DE *)buffer;
            alreadyLoadedRootDir = targetDir;
            isCached = 1;
        }
    } else if (ppi.index >= 0) {
        // A valid index means we found a directory entry inside a parent
        // directory. We need to get a pointer to that entry.
        DE *dirEntry = &ppi.parent[ppi.index];
        
        // Safety check: make sure this entry is actually a directory and
        // not a regular file. You can't iterate through a file!
        if (dirEntry->isDir != 1) {
            return NULL;
        }
        
        // Before loading from disk, check if this directory is already
        // cached as the current working directory. We compare block
        // numbers because that's how we uniquely identify directories.
        if (alreadyLoadedCWD != NULL && 
            dirEntry->blockNumber == alreadyLoadedCWD->blockNumber) {
            targetDir = alreadyLoadedCWD;
            isCached = 1;
        } else {
            // Not cached, so load the directory from disk. This reads the
            // directory's data blocks and returns a pointer to the array
            // of directory entries.
            targetDir = loadDirectory(dirEntry);
            if (targetDir == NULL) {
                return NULL;
            }
        }
    } else {
        // Index is -1 meaning the directory wasn't found in the parent
        return NULL;
    }
    
    // Final validation: ensure we have a valid directory pointer and that
    // it's actually marked as a directory not corrupted data
    if (targetDir == NULL || targetDir->isDir != 1) {
        return NULL;
    }
    
    // Calculate how many directory entries can fit in this directory.
    // The first entry index 0 stores the total size in numBytes, so
    // we divide by the size of one directory entry to get the count.
    int totalEntries = targetDir[0].numBytes / sizeof(DE);
    if (totalEntries <= 0) {
        return NULL; // Invalid directory size
    }
    
    // Allocate memory for the directory descriptor structure. This is
    // like a file descriptor but for directories it keeps track of
    // which entry we're currently looking at.
    fdDir *dirp = (fdDir *)malloc(sizeof(fdDir));
    if (dirp == NULL) {
        // If allocation fails clean up the directory we loaded but only
        // if it's not cached since we shouldn't free cached directories
        if (!isCached && targetDir != alreadyLoadedRootDir && 
            targetDir != alreadyLoadedCWD) {
            free(targetDir);
        }
        return NULL;
    }
    
    // Initialize the directory descriptor. We start at position 0 the
    // first entry, point to our loaded directory, and set the record
    // length. The di pointer will be allocated when we first read an
    // entry (lazy allocation to save memory if the directory is never
    // actually read).
    dirp->dirEntryPosition = 0;
    dirp->directory = targetDir;
    dirp->d_reclen = sizeof(DE);
    dirp->totalEntries = totalEntries; // Store for efficiency in readdir
    dirp->di = NULL;
    
    return dirp;
}

// Returns the next directory entry each time it's called. When there
// are no more entries it returns NULL to signal the end.
struct fs_diriteminfo *fs_readdir(fdDir *dirp){
    // Input validation: need a valid directory descriptor and directory
    if (dirp == NULL || dirp->directory == NULL) {
        return NULL;
    }
    
    // Use the totalEntries we calculated and stored in opendir. This
    // avoids recalculating it every time readdir is called, which is
    // more efficient
    
    // Loop through entries starting from our current position. We keep
    // going until we've checked all possible entry slots
    while (dirp->dirEntryPosition < dirp->totalEntries) {
        // Get a pointer to the current directory entry we are examining
        DE *currentEntry = &dirp->directory[dirp->dirEntryPosition];
        
        // Skip empty entries. When a directory entry is deleted or never
        // used the name field starts with a null terminator. We want to
        // skip these "holes" in the directory and only return valid entries.
        if (currentEntry->name[0] == '\0') {
            dirp->dirEntryPosition++;
            continue;
        }
        
        // Only allocate the return structure when we
        // actually find a valid entry. This saves memory if someone opens
        // a directory but never reads from it.
        if (dirp->di == NULL) {
            dirp->di = (struct fs_diriteminfo *)
                malloc(sizeof(struct fs_diriteminfo));
            if (dirp->di == NULL) {
                return NULL;
            }
        }
        
        // Copy the entrys name into our return structure. We use strncpy
        // to safely copy up to 255 characters leaving room for the null
        // terminator in the 256-character buffer.
        strncpy(dirp->di->d_name, currentEntry->name, 255);
        dirp->di->d_name[255] = '\0'; // Ensure null termination
        
        // Set the file type based on the isDir flag. This tells the caller
        // whether this entry is a directory can contain other files or
        // a regular file (contains data).
        if (currentEntry->isDir == 1) {
            dirp->di->fileType = FT_DIRECTORY;
        } else {
            dirp->di->fileType = FT_REGFILE;
        }
        
        // Set the record length to the size of a directory entry. This
        // is useful information for the caller.
        dirp->di->d_reclen = sizeof(DE);
        
        // Advance our position counter so the next call to readdir will
        // return the next entry in the directory.
        dirp->dirEntryPosition++;
        
        // Return the information about this entry. Note: we return "." and
        // ".." entries too 
        return dirp->di;
    }
    
    // Reached the end of the directory ... no more entries to return
    return NULL;
}

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
