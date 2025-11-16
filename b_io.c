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
#include "fsLow.h"

#define MAXFCBS 20
#define BLOCK_SIZE 512

typedef struct b_fcb
    {
    fileInfo * fi;    		 //holds the low level systems file info
    // Add any other needed variables here to track the individual open file
    char * buf;
    int currentFilePosition; // logical position like a pointer i.e. current byte offset
    int bufferStartPosition; // basically buffer[0]
    int numValidBufferBytes; // Holds how many valid bytes are in the buffer
	int flags;				 // Holds the flags used on that file.
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
	if (startup == 0) 
		b_init();  //Initialize our system
	if (!filename) 
		return -1; //invalid filename
	b_io_fd fcbIndex = b_getFCB();
	//*** TODO ***:  Modify to save or set any information needed and AND SUPPORT OF FLAGS
	//printf("in open\n");
	if (fcbIndex == -1){
		printf("In b_open: No free fcb index found.\n");
		return -1;
	}
	// Get the file information from the low level system
	fileInfo *fi = getFileInfo(filename, flags);
	if (!fi){
		printf("In b_open : getFileInfo failed \n");
		return -1; //malloc failure
		// malloc failure
	}
	// if (!fi) return -1; //file not found
	if (fcbIndex == -1) return -1; //no free file control blocks
	//*** TODO ***///  Write open function to return your file descriptor
	//				  You may want to allocate the buffer here as well
	//				  But make sure every file has its own buffer
	fcbArray[fcbIndex].buf = malloc(BLOCK_SIZE);
	if (fcbArray[fcbIndex].buf == NULL){
		free(fi);
		fcbArray[fcbIndex].fi = NULL; // free the FCB
		return -1;	// memory allocation failed
	}
	fcbArray[fcbIndex].fi = fi;
	fcbArray[fcbIndex].currentFilePosition = 0;
	fcbArray[fcbIndex].bufferStartPosition = 0;
	fcbArray[fcbIndex].numValidBufferBytes = 0;
	fcbArray[fcbIndex].flags = flags;
	//printf("Ending open with fcbIndex: %d \n", fcbIndex);
	//TO DO'S SUPPORT FLAGS AND ALL THAT HERE

			// get our own file descriptor
	// check for error - all used FCB's
	printf("In b_open free fcb index found at : %d\n", fcbIndex);
	return fcbIndex;						// all set*/
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
    // TODO //
    // Write buffered read function to return the data and # bytes read
    // You must use LBAread and you must buffer the data in BLOCK_SIZE byte chunks.

    if (startup == 0) b_init();  //Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS))
        {
        return (-1);                     //invalid file descriptor
        }

    // and check that the specified FCB is actually in use
    if (fcbArray[fd].fi == NULL)        //File not open for this descriptor
        {
        return -1;
        }


    b_fcb * fcb = &fcbArray[fd]; // Get the current fcb

    int bytesRead = 0; // Keep track of bytes read per user call to ensure
                       // no more bytes than the user wants (count) are put into user buffer
	// Main read loop
    while (bytesRead < count && fcb->currentFilePosition < fcb->fi->fileSize) {
        // Skip reading a block if the current position is already within the buffer,
        // the data is already immediatly accessable! So only perform the read
        // when current position is OUTSIDE the bounds of our buffer.
        if (fcb->currentFilePosition < fcb->bufferStartPosition ||
            fcb->currentFilePosition >= fcb->bufferStartPosition + fcb->numValidBufferBytes) {

            int blockNumber = fcb->currentFilePosition / BLOCK_SIZE;

            // Read 1 block
            LBAread(fcb->buf, 1, fcb->fi->location + blockNumber);

            // Update buffer position to reflect the read
            fcb->bufferStartPosition = blockNumber * BLOCK_SIZE;
            fcb->numValidBufferBytes = BLOCK_SIZE;
            // If the read has gone past EOF, reduce valid buffer bytes
            if (fcb->bufferStartPosition + BLOCK_SIZE > fcb->fi->fileSize) {
                fcb->numValidBufferBytes = fcb->fi->fileSize - fcb->bufferStartPosition;
            }
        }

        // numValidBufferBytes 512 or less if EOF. From this total valid amount,
        // Only copy starting from the current position, therefore subtract
        // from it the difference between current position and buffer start
        int bytesToCopy = (fcb->numValidBufferBytes - 
            (fcb->currentFilePosition - fcb->bufferStartPosition));
        // If this^^ amount of bytes is greater than the amount of bytes
        // the user asked for, do not use that amount because it will
        // copy unwanted bytes into the users buffer.
        if ((count - bytesRead) < bytesToCopy) {
            bytesToCopy = (count - bytesRead);
		}
        // Now with the appropriate number of bytes to copy, perform the copy
        // from our buffer into the users buffer
        memcpy(buffer, fcb->buf + (fcb->currentFilePosition - fcb->bufferStartPosition), 
                bytesToCopy);

        // Update trackers for the next loop
        bytesRead += bytesToCopy;
        fcb->currentFilePosition += bytesToCopy;
    }
    return bytesRead;
    // Your Read code here - the only function you call to get data is LBAread.
    // Track which byte in the buffer you are at, and which block in the file
    }

	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
	//printf("In close\n");
	//*** TODO ***//  Release any resources
	if ((fd < 0) || (fd >= MAXFCBS)) return -1;
    if (fcbArray[fd].fi == NULL) return -1;

    // free per-file buffer
    if (fcbArray[fd].buf)
        {
        free(fcbArray[fd].buf);
        fcbArray[fd].buf = NULL;
        }

    // reset FCB so it can be reused
    fcbArray[fd].fi = NULL;
	fcbArray[fd].currentFilePosition = 0;
	fcbArray[fd].bufferStartPosition = 0;
	fcbArray[fd].numValidBufferBytes = 0;
	//printf("Ending close\n");
    return 0;
	}
