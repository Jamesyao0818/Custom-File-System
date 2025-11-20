/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Ian Kligman, Joseph Bowen, Cheng-Chieh Yao, Enrique Cabral-Mixco
* Student IDs:: 924122742, 922966782 , 923978442, 924056104
* GitHub-Name:: ikligman, josephb333, Jamesyao0818, rommix408
* Group-Name:: Don't Worry We Don't Byte.
* Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/


#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"
#include "DE.h"
#include "fileinfo.h"
#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

//Globals for access between functions.
extern DE *alreadyLoadedRootDir;
extern DE *alreadyLoadedCWD;

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct fdDir
	{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	unsigned short  d_reclen;		/* length of this record */
	unsigned short	dirEntryPosition;	/* which directory entry position, like file pos */
	unsigned short	totalEntries;		/* total number of entry slots in directory */
	DE * directory;			/* Pointer to the loaded directory you want to iterate */
	struct fs_diriteminfo * di;		/* Pointer to the structure you return from read */
	} fdDir;

// Key directory functions
int fs_mkdir(char *pathname, mode_t mode);
int fs_rmdir(char *pathname);

// Directory iteration functions
fdDir * fs_opendir(char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
// Joe
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise

// Enrique
int fs_delete(char* filename);	//removes a file

// Struct for int parsePath()
typedef struct parsePathInfo {
	DE * parent;
	char *lastElementName;
	int index; // -1 indicates it does not exist.
} parsePathInfo;

// Global variables for cached directories (defined in mfs.c)
extern DE *alreadyLoadedRootDir;
extern DE *alreadyLoadedCWD;

// Helper functions;
// Enrique
int findInDirectory(DE * directory, char * token);
// int isDEaDir();

// Joe
DE *loadDirectory(const DE *dirEntry);
int findChildByHeadBlock(const DE *parent, int entryCount, int childHeadBlock);
// Ian
int parsePath(char * pathName, parsePathInfo *ppi);

// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	};


int fs_stat(char *path, struct fs_stat *buf);
int Touch(DE * parent, char * name);
int ensureAllocation(fileInfo *fi, int newFileSize);
#endif

