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
#include "vector.h"
#include "FSM.h"
#include "fileinfo.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//Globals for access between functions.
//DE *alreadyLoadedRootDir = NULL;
//DE *alreadyLoadedCWD = NULL;

//IMPLEMENT THESE FUNCTIONS
// Ian
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
// Joe
int findChildByHeadBlock(const DE *parent, int entryCount, int childHeadBlock) {
    for (int i = 2; i < entryCount; i++) {                 // skip '.' and '..'
        if (parent[i].name[0] == '\0') continue;           // empty slot
        if (parent[i].isDir && parent[i].blockNumber == childHeadBlock) {
            return i;
        }
    }
    return -1;
}

// Key directory functions

// Joe
DE *loadDirectory(const DE *dirEntry){
    if (!dirEntry){
        printf("Error: LoadDirectory called a NULL DE.\n");
        return NULL;
    }
    // Print what we were asked to load (helps you spot the bad caller)
    //printf("loadDirectory(): name='%s' isDir=%d blockNumber(logical)=%d\n",
           //dirEntry->name, dirEntry->isDir, dirEntry->blockNumber-1);
    if (dirEntry->isDir != 1){
        printf("Error: LoadDirectory called on DE that is not a directory.\n");
        return NULL;
    }
    // Use VCB to get block size.
    struct VCB* v = getVCB();
    int blockSize = v->blockSize;
    // Calculate how many blocks we need to read for this directory.
    int bytesToRead = (int)dirEntry->numBytes;
    int blocksToRead = (bytesToRead + (blockSize - 1)) / blockSize;
    if (blocksToRead <= 0) blocksToRead = 1;
    /*if (bytesToRead <= 0){
        printf("Error: Invalid numBytes (%d) in DE\n", bytesToRead);
        return NULL;
    }*/
    // Allocate memory for directory contents
    size_t bytesToMalloc = (size_t)blocksToRead * (size_t)blockSize;
    //int lba = dirEntry->blockNumber;
    char *buffer = (char *)malloc(bytesToMalloc);
    if (!buffer){
        printf("Error: malloc failue in LoadDirectory().\n");
    }
    // Convert from logical block index to physical
    // Double check this part
    int physBlock = dirEntry->blockNumber;
    printf("In loadDirectory(): lbaReading block number : %d\n", physBlock);
    // Make sure we also handle discontiguous instances. Pending James's Linked List implementation.
    u_int64_t blocksRead = LBAread(buffer, blocksToRead, physBlock);
    if (blocksRead != 1) {
        printf("Error: LoadDirectory failed to read 1 block (got %lu blocks).\n", blocksRead);
        free (buffer);
        return NULL;
    }
    // If read was done correctly, now we can just return the raw bytes of
    // the dirEntry we want to load. Should work.
    //printf("In loadDirectory(): Root logical=%d physical=%d\n",
       //v->blockIndexRootDirectory, v->blockIndexRootDirectory + 1);
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

// Enrique
// Directory iteration functions
// Opens a directory for reading its contents. This function prepares the
// directory for iteration by loading it into memory or using a cached
// version and creating a directory descriptor fdDirthat tracks our
// position as we read through entries.
fdDir * fs_opendir(char *pathname){
    // We need to check for NULL before calling strlen, which would crash
    // if pathname is NULL. parsePath also checks for NULL, but we need
    // to validate here first since we use strlen before calling parsePath.
    if (pathname == NULL) {
        return NULL;
    }
    
    // Parse the pathname to break it down into components and find the
    // target directory entry. The parsePathInfo structure will contain
    // information about where the directory is located.
    parsePathInfo ppi;
    int parseResult = parsePath(pathname, &ppi);
    if (parseResult != 0) {
        return NULL; // Path doesn't exist or is invalid
    }
    
    DE *targetDir = NULL;
    int isCached = 0; // Track whether we're using a cached directory
    
    // When index is -2, it means we're opening the parent directory itself
    // like opening "/" for root. This is how parsePath indicates that
    // the path refers to a directory rather than something inside it.
    if (ppi.index == -2) {
        // This means we're opening the parent directory itself like "/" for root.
        // Root directory should have been loaded in initFileSystem so ppi.parent
        // should point to alreadyLoadedRootDir. If NULL, indicates a
        // root wasn't loaded during initialization.
        if (ppi.parent == NULL) {
            printf("Error: fs_opendir - root directory not loaded. "
                   "initFileSystem must be called first.\n");
            return NULL;
        }
        
        // Use the already loaded directory (cached in initFileSystem)
        targetDir = ppi.parent;
        // Mark as cached if it matches our global cache pointers
        if (targetDir == alreadyLoadedRootDir || 
            targetDir == alreadyLoadedCWD) {
            isCached = 1;
        }
    } else if (ppi.index >= 0) {
        // A valid index means we found a directory entry inside a parent
        // directory. We need to get a pointer to that entry.
        DE *dirEntry = &ppi.parent[ppi.index];
        //int isDEaDir();
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
        // Find the child’s entry in parent by matching the child’s head block.
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

// Closes a directory and frees all resources associated with it. 
// Every opendir should have a matching closedir. However, we are 
// careful not to free cached directories (root and CWD)
// since they're shared and may still be in use.
int fs_closedir(fdDir *dirp){
    // Input validation: can't close something that doesn't exist
    if (dirp == NULL) {
        return -1;
    }
    
    // Check if this directory is one of our cached directories. Cached
    // directories root and current working directory are kept in memory
    // for performance and may be used by multiple operations. We should
    // never free them here because other parts of the code might still
    // be using them.
    int isCached = 0;
    if (dirp->directory == alreadyLoadedRootDir || 
        dirp->directory == alreadyLoadedCWD) {
        isCached = 1;
    }
    
    // Only free the directory data if its not cached. If its cached,
    // we leave it in memory.
    if (!isCached && dirp->directory != NULL) {
        free(dirp->directory);
    }
    
    // Free the directory item info structure if it was allocated during
    // readdir operations. This structure holds information about individual
    // entries that we return to the caller.
    if (dirp->di != NULL) {
        free(dirp->di);
    }
    
    // Free the directory descriptor structure itself. This was
    // allocated in opendir and tracks our iteration state.
    free(dirp);
    
    return 0; 
}

// Joe 
// Misc directory functions
// PWD
char * fs_getcwd(char *pathname, size_t size){
    // Invalid parameter checks.
    if (!pathname || size == 0){
        return NULL;
    }
    if (!alreadyLoadedCWD || !alreadyLoadedRootDir) {
        return NULL;
    }

    // /Home/Documents/HW/csc..filesystem/
    // Edge case: if he CWD is the root, return "/"
    DE * cwd = alreadyLoadedCWD;
    if (cwd[0].blockNumber == cwd[1].blockNumber){
        if (size < 2) return NULL;
        pathname[0] = '/';
        pathname[1] = '\0';
        return pathname;
    }

    stringBuilder parts; 
    initVector(&parts);
    DE * currentDir = cwd;
    // Walk back recursively along directory entries tree.
    while (1) {
        DE *parent = loadDirectory(&currentDir[1]);
        if (!parent){
            freeVector(&parts);
            return NULL;
        }
        int index = (int)findChildByHeadBlock(parent, parent[0].numBytes/ sizeof(DE),currentDir[0].blockNumber);
        if( index < 0){
            free(parent);
            freeVector(&parts);
            return NULL;
        }
        if (vectorAdd(&parts, parent[index].name) != 0){
            free(parent);
            freeVector(&parts);
            return NULL;
        }
        // Only instance where "." and ".." share blockNumber will be root.
        // So we know we're done.
        if (parent[0].blockNumber == parent[1].blockNumber){
            free(parent);
            break;
        }
        // Free current only if it wasn't dynamically loaded.
        // With this:
        if (currentDir != alreadyLoadedCWD && currentDir != alreadyLoadedRootDir){
            free(currentDir);
        }
        currentDir = parent;
    } 

    // Now that we have added to the vector starting from cwd working up to root.
    // The vector's substrings are in reverse order. So now we loop and reverse.
    // Them to get them in our desired order.
    if (parts.size > 1) {
       size_t endIndex = parts.size - 1;
       size_t startIndex = 0;
       while (startIndex < endIndex){
        char *tmp = parts.subStrings[startIndex];
        parts.subStrings[startIndex] = parts.subStrings[endIndex];
        parts.subStrings[endIndex] = tmp;
        startIndex++;
        endIndex--;
       }
    }
    char *returnString = build(&parts, '/');
    // Get length of string to return
    size_t returnLength = strlen(returnString);
    // So we can compare it to sized passed in.
    // '/' "returnStringLength" '/' for instance that's why we add two.
    if (returnLength + 2 > size){
        free(returnString);
        freeVector(&parts);
        return NULL; //Buffer too small
    }
    // Now that we have correct return string and verified sufficien
    // Space and pathname length. We just memcpy to buffer (pathname).
    pathname[0] = '/';
    memcpy(pathname +1, returnString, returnLength + 1);
    // Free everthing now that we are done.
    free(returnString);
    freeVector(&parts);
    return pathname;
}
// Joe
// CD
// linux chdir
int fs_setcwd(char *pathname){
    //printf("Path name prior in setCwd():%s\n", pathname);
    //char * cwdPath = fs_getcwd(pathname, sizeof(pathname));
    //printf("Path name of cwd setCwd():%s\n", cwdPath);
    if (!pathname || strlen(pathname) == 0) return 0;
    DE *child = NULL;
    parsePathInfo ppi;
    if (parsePath(pathname, &ppi) != 0) {
        printf("Error in fs_setcwd: Invalid path\n");
        return -1;
    }
    int index = findInDirectory(ppi.parent, ppi.lastElementName);
    if (!ppi.lastElementName){
        //path already points to cwd
        alreadyLoadedCWD = ppi.parent;
        return 0;
    }
    if (ppi.parent[index].isDir != 1){
        printf("fs_setcwd: %s is not a directory.\n", ppi.lastElementName);
        return -1;
    }
    // Final element is dir and exists.
    child = &ppi.parent[index];
    printf("DEBUG cd: found '%s' at idx=%d, isDir=%d, startLBA=%d size=%d\n",
       ppi.lastElementName, index, child->isDir,
       child->blockNumber, child->numBytes);
    if (!child){
        printf("fs_setcwd: failed to load directory '%s'\n", ppi.lastElementName);
    }
    DE *newCWD = loadDirectory(child);
    alreadyLoadedCWD = newCWD;
    return 0;
}   
// Joe
// Returns 1 if file, 0 for dir, and -2 if called with bad parameters.
int fs_isFile(char * filename){
    if (strcmp(filename, ".") == 0) return 0;
    if (strcmp(filename, "..") == 0) return 0;
    if (!filename || strlen(filename) == 0) return -2;
    printf("File name in isFile():%s\n", filename);
    DE *cwd = alreadyLoadedCWD;
    int index = findInDirectory(cwd, filename);
    if (cwd[index].isDir == 1)
        return 0;
    return 1;
}
// Joe
// Returns 1 if directory, 0 otherwisepathname
int fs_isDir(char * pathname){
    if (!pathname || strlen(pathname) == 0) return 0;

    parsePathInfo ppi;
    if (parsePath(pathname, &ppi) != 0){
        printf("Error in fs_isDir: Invalid path : %s \n", pathname);
        return -1;
    }    
    return ppi.parent[ppi.index].isDir;
}

// James
int fs_stat(char *path, struct fs_stat *buf){
    if(path == NULL || buf == NULL){
        return -1; // Invalid arguments
    }

    parsePathInfo ppi;
    int parseResult = parsePath(path, &ppi);
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

// Ian / Joe
// Returns 0 for valid path, any non-zero number reflects a specific error.
int parsePath(char * pathName, parsePathInfo * ppi) { // ppi is a buffer we will write to
    // is this a relative or absolute path?
    //printf("In parsePath() pathname is :%s\n", pathName);

    // copy pathName so that the caller does not have to worry
    // about their pathname being altered
    char * pathCopy = (char *)malloc(strlen(pathName) + 1);
    if (pathCopy == NULL) {
        return -1;
    }
    strcpy(pathCopy, pathName);
    //printf("Pathname in parsepath is :%s\n", pathCopy);
    DE * startParent;
    DE * parent;
    char * saveptr;
    char * token1;
    char * token2;
        
    if (pathCopy[0] == '/') { // absolute path
        startParent = alreadyLoadedRootDir;
    }
    else {
        startParent = alreadyLoadedCWD;
    }
    parent = startParent;

    token1 = strtok_r(pathCopy, "/", &saveptr);
    if (token1 == NULL) {
        if (parent == alreadyLoadedRootDir) { // i.e. path[0]='/'
            ppi->parent = parent;
            ppi->lastElementName = NULL;
            ppi->index = -2;
            free(pathCopy);
            return 0;
        }
        else {
            free(pathCopy);
            return -1;
        }
    }

    while (1)
    {
        int idx = findInDirectory(parent, token1);
        token2 = strtok_r(NULL, "/", &saveptr);
        if (token2 == NULL) { // we are done, token 1 is last element
            ppi->parent = parent;
            // AFTER PARSEPATH REFACTOR: implimented the strdup the professor mentioned
            // might be needed depending on our implimentation!!
            ppi->lastElementName = strdup(token1);
            ppi->index = idx;
            free(pathCopy);
            return 0;
        }
        if (idx == -1) {
            free(pathCopy);
            return -2; // invalid path
        } 

        if (!parent[idx].isDir) {
            free(pathCopy);
            return -3; // invalid path (can't be file in middle)
        } 
        DE *tempParent = loadDirectory(&parent[idx]);

        if (tempParent == NULL) {
            free(pathCopy);
            return -4; // failed to load dir
        }

        if (parent != startParent) {
            free(parent);
        }
        parent = tempParent;
        token1 = token2;
    }
}

// Ian / Joe
int fs_mkdir(char *pathname, mode_t mode) {
    printf("DEBUG: fs_mkdir called with path: %s\n", pathname);

    // get ppi
    parsePathInfo ppi;
    printf("DEBUG: Calling parsePath...\n");
    if (parsePath(pathname, &ppi) != 0) {
        printf("DEBUG: parsePath failed!\n");
        return -1;
    }
    printf("DEBUG: parsePath succeeded. parent=%p, lastElementName=%s, index=%d\n", 
           ppi.parent, ppi.lastElementName, ppi.index);

    // get vcb for blockSize
    printf("DEBUG: Getting VCB...\n");
    struct VCB * vcb = getVCB();
    if (!vcb) {
        printf("DEBUG: getVCB returned NULL!\n");
        if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
            free(ppi.parent);
        }
        return -1;
    }
    printf("DEBUG: VCB obtained, blockSize=%d\n", vcb->blockSize);
    
    printf("DEBUG: Calling createDir...\n");
    int childLBA = allocate(vcb->blockSize);
    int parentLBA = ppi.parent[0].blockNumber;
    DE * newDir = createDir(childLBA, parentLBA, 2, ppi.parent, vcb->blockSize);
    if (!newDir) {
        printf("DEBUG: createDir failed!\n");
        if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
            free(ppi.parent);
        }
        return -1;
    }
    printf("DEBUG: createDir succeeded. newDir=%p\n", newDir);

    // update the new dir's . entry
    //printf("DEBUG: Updating directory name from '.' to '%s'\n", ppi.lastElementName);

    //newdir[0].isDir = 1;
    // find a free index to put this entry in the parent
    printf("DEBUG: Finding free index in parent...\n");
    int freeIndex = -1;
    int numEntries = ppi.parent[0].numBytes / sizeof(DE);
    printf("DEBUG: Parent has %d entries\n", numEntries);
    for (int i = 2; i < numEntries; i++) {
        if (ppi.parent[i].name[0] == '\0') {
            freeIndex = i;
            printf("DEBUG: Found free index at %d\n", i);
            break;
        }
    }

    if (freeIndex == -1) {
        printf("DEBUG: No free index found in parent!\n");
        free(newDir);
        if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
            free(ppi.parent);
        }
        return -2;
    }

    time_t now = time(NULL);
    printf("DEBUG: Adding entry to parent at index %d\n", freeIndex);


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
    // THOUGHT FOR FUTURE
    // SEE IF WE NEED TO ADD CHECK FOR FILE/DIR
    // ASSIGN ISDIR APPROPRIATELY...

        if (ppi.parent[freeIndex].isDir == 1)
        printf("New entry is a directory\n");
    else printf("New entry is file\n");
    // this modified the parent, so update its mod time
    ppi.parent[0].modifyDate = now;

    printf("DEBUG: Writing directories to disk...\n");
    // commit changes to disk
    writeDirToDisk(newDir, vcb->blockSize);
    writeDirToDisk(ppi.parent, vcb->blockSize);
    
    printf("DEBUG: Cleaning up memory...\n");
    free(newDir);

    if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
        free(ppi.parent);
    }

    printf("DEBUG: fs_mkdir completed successfully!\n");
    return 0;
}
// Ian
int fs_rmdir(char *pathname) {
    printf("DEBUG: fs_rmdir called with path: %s\n", pathname);
    
    // get ppi
    parsePathInfo ppi;
    printf("DEBUG: (rmdir) Calling parsePath...\n");
    if (parsePath(pathname, &ppi) != 0) {
        printf("DEBUG: parsePath failed!\n");
        return -1;
    }
    printf("DEBUG: (rmdir) parsePath succeeded. parent=%p, lastElementName=%s, index=%d\n", 
           ppi.parent, ppi.lastElementName, ppi.index);

    // ensure this is a dir
    if (!ppi.parent[ppi.index].isDir) {
        printf("DEBUG: %s is not a directory!\n", ppi.lastElementName);

        if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
            free(ppi.parent);
        }
        return -2; // not a dir
    }

    // load the dir to be deleted
    printf("DEBUG: rmdir loading dir...\n");
    DE * dirToDelete = loadDirectory(&ppi.parent[ppi.index]);

    if (!dirToDelete) {
        printf("DEBUG: rmdir Failed to load!!\n");

        if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
            free(ppi.parent);
        }
        return -3; // failed to load
    }

    // ensure dir is empty except for . and .. (2 DEs)
    int isEmpty = 1;
    int numEntrySlots = dirToDelete[0].numBytes / sizeof(DE);
    printf("DEBUG: Parent has %d entry slots\n", numEntrySlots);
    for (int i = 2; i < numEntrySlots; i++) {
        if (dirToDelete[i].name[0] != '\0') {
            isEmpty = 0;
            printf("DEBUG: Directory not empty, contains %s at index %d\n",dirToDelete[i].name, i);
            
            free(dirToDelete);
            if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
                free(ppi.parent);
            }
        return -4; // not empty
        }
    }

    // get vcb for blockSize
    printf("DEBUG: Getting VCB...\n");
    struct VCB * vcb = getVCB();
    if (!vcb) {
        printf("DEBUG: getVCB returned NULL!\n");
        if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
            free(ppi.parent);
            free(dirToDelete);
        }
        return -1;
    }
    printf("DEBUG: VCB obtained, blockSize=%d\n", vcb->blockSize);
    
    // update fsm
    fsmReleaseLBA(ppi.parent[ppi.index].blockNumber);

    // clear entry in the parent
    memset(&ppi.parent[ppi.index], 0, sizeof(DE)); 

    time_t now = time(NULL);
    // add entry to parent's free directory entry index
    
    ppi.parent[0].modifyDate = now;

    printf("DEBUG: (rmdir) Writing updated parent to disk...\n");
    // commit changes to disk
    writeDirToDisk(ppi.parent, vcb->blockSize);
    
    printf("DEBUG: Cleaning up memory...\n");
    free(dirToDelete);

    if (ppi.parent != alreadyLoadedRootDir && ppi.parent != alreadyLoadedCWD) {
        free(ppi.parent);
    }

    printf("DEBUG: fs_rmdir completed successfully!\n");
    return 0;
}

// Ian & Joe
int fs_delete(char* filename) {
    if (!filename)
        return -1; //Bad call
    DE * cwd = alreadyLoadedCWD;
    // Files does not exist
    int index = findInDirectory(cwd, filename);
    if (index < 0)
        return -2; // File not found
    if (cwd[index].isDir != 0)
        return -3; // Called on directory
    DE * fileFileToRemove = &cwd[index];
    fsmReleaseLBA(cwd[index].blockNumber);
    struct VCB *v = getVCB();
    memset(fileFileToRemove, 0, sizeof(DE));
    return 0;
}
    // Joe + Ian
    // Returns block of new file on success else returns -1 for bad call. -2 insufficient entries, -3 no free entries, -4 LBAwrite failure.
    int Touch(DE * parent, char * name) {
        if (!name){
        printf("Bad call of touch passed null name\n");
        return -1;
        }
        struct VCB * v = getVCB();
        // call allocate to reflect this new file in fsm
        int freeBlock = allocate(v->blockSize);
        if (freeBlock < 0) {
            return freeBlock;
        }
        
        int numEntries = parent[0].numBytes / sizeof(DE);
        if (numEntries <= 2){
            printf("Touch: Not enough entries found.\n");
            return -2; // Error not enough entries.
        }
        int freeIndex = -1;
        printf("DEBUG: Parent has %d entries\n", numEntries);
        for (int i = 2; i < numEntries; i++) {
            if (parent[i].name[0] == '\0') {
            freeIndex = i;
            printf("DEBUG: Found free index at %d\n", i);
            break;
            }
        }
        if (freeIndex == -1){
            printf("DEBUG: No free entry slots in directory\n");
            return -3; // Error no free entries.
        }
        time_t now = time(NULL);
        DE * newEntry = &parent[freeIndex];
        memset(newEntry, 0, sizeof(DE));
        strcpy(newEntry->name, name);
        newEntry->isDir = 0;
        newEntry->numBytes = 0;
        newEntry->blockNumber = freeBlock;
        newEntry->accessDate = now;
        newEntry->createDate = now;
        newEntry->modifyDate = now;
        parent[0].modifyDate = now;

        if(!LBAwrite(parent, 1, parent[0].blockNumber)) {
            printf("DEBUG: (touch) Failed to LBAwrite to update parent!");
            return -4; // LBA write failure
        }

        printf("DEBUG: touch success! made empty file %s at block %d\n", name, freeBlock);
        return freeBlock;
     }

fileInfo * getFileInfo (char * filename, int flags){
    if (!filename)
        return NULL; //Bad call
    parsePathInfo ppi;
    int ppResult = parsePath(filename, &ppi);
    if (ppResult != 0)
        return NULL; // Invalid path
    // File name exists and is found path.
    if (ppi.index >= 0){
        DE *Entry = &ppi.parent[ppi.index];
        printf("In getFileInfo() Entry populated with isDir value of :%d \n", Entry->isDir);
        if (Entry->isDir==1)
            return NULL; //File name is directory invalid call
        fileInfo *fi = malloc(sizeof(fileInfo));
        if (!fi){
            printf("In getFileInfo() malloc failure \n");
            return NULL;
        }
        strcpy(fi->fileName, Entry->name);
        fi->fileSize = Entry->numBytes;
        fi->location = Entry->blockNumber;
        return fi;
    }
    if ((flags & O_CREAT) != 0){
        // File name does NOT exist and is flagged O_CREATE
        DE * cwd = alreadyLoadedCWD;
        if (!cwd){
            printf("In getFileInfo(): CWD is NULL, cannout create file.\n");
            return NULL;
        }
        // Files does not exist
        int location = Touch(cwd, filename);
        if (location < 0){
            printf("In getFileInfo(): Touch failed with result %d.\n", location);
            return NULL;
        }
        int index = findInDirectory(cwd, filename);
        if (index < 0){
            printf("In getFileInfo(): created file but could not find it in directory.\n");
            return NULL;
        }
        fileInfo *fi = malloc(sizeof(fileInfo));
        if (!fi){
            printf("In getFileInfo() malloc failure \n");
            return NULL;
        }
        strcpy(fi->fileName,filename);
        fi->fileSize = 0;
        fi->location = location;
        return fi;
    }
    // Non-existent file with lack of O_CREAT flag
    return NULL;
}

int updateFileSizeOnDisk(char *pathname, int newSize){
    if (!pathname)
        return -1;
    parsePathInfo ppi;
    int ppResult = parsePath(pathname, &ppi);
    if (ppResult != 0){
        printf("In updateFileSizeOnDisk: parsepath called with invalid path.\n");
        return -2;
    }    
    if (ppi.index < 0){
        printf("In updateFileSizeOnDisk: DE not found for:%s\n", pathname);
        return -3;
    }
    ppi.parent[ppi.index].numBytes = newSize;
    ppi.parent[ppi.index].modifyDate = time(NULL);
    LBAwrite(ppi.parent, 1, ppi.parent[0].blockNumber);
    return 0;
}
