/**************************************************************
* Class::  CSC-415-0# Fall 2025
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "fileinfo.h"

#define MAXFCBS 20
#define BLOCK_SIZE 512

typedef struct b_fcb
    {
    fileInfo * fi;    		 //holds the low level systems file info
    // Add any other needed variables here to track the individual open file
    char * buf;
    int currentFilePosition; //index //curent position of the buffer
    int bufferStartPosition; // start position
    int numValidBufferBytes; // Holds how many valid bytes are in the buffer
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}

// GetFileInfo takes a file name (as a C string) and returns a pointer to
// a fileInfo structure that contains the name, file size in bytes and the 
// starting block number for the file.  It returns NULL if it can not find
// the specified file.
// fileInfo * GetFileInf(char *filename);



// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;
	//*** TODO ***:  Modify to save or set any information needed and AND SUPPORT OF FLAGS
	//printf("in open\n");
	if (startup == 0) 
		b_init();  //Initialize our system
	if (!filename) 
		return -1; //invalid filename
	// Get the file information from the low level system
	fileInfo *fi = GetFileInfo(filename);
	if (!fi){
		free(fi);
		return -1; //malloc failure
		// malloc failure
	}
	// if (!fi) return -1; //file not found
	if (returnFd == -1) return -1; //no free file control blocks
	//*** TODO ***///  Write open function to return your file descriptor
	//				  You may want to allocate the buffer here as well
	//				  But make sure every file has its own buffer
	fcbArray[returnFd].buf = malloc(BLOCK_SIZE);
	if (fcbArray[returnFd].buf == NULL){
		free(fi);
		fcbArray[returnFd].fi = NULL; // free the FCB
		return -1;	// memory allocation failed
	}
	fcbArray[returnFd].fi = fi;
	fcbArray[returnFd].currentFilePosition = 0;
	fcbArray[returnFd].bufferStartPosition = 0;
	fcbArray[returnFd].numValidBufferBytes = 0;
	//printf("Ending open with returnFd: %d \n", returnFd);
	//TO DO'S SUPPORT FLAGS AND ALL THAT HERE

	returnFd = b_getFCB();				// get our own file descriptor
	// check for error - all used FCB's
	return (returnFd);						// all set*/
}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{

	}
