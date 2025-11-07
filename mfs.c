#include "mfs.h"
#include <string.h>
//IMPLEMENT THESE FUNCTIONS

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file
int fs_stat(const char *path, struct fs_stat *buf);
int parsePath(char *pathName, ppinfo *ppi)
{ 
    // ppi is a buffer we will write to
    // is this a relative or absolute path?
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
