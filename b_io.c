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

	// TODO 
	// MAKE SURE ORDER OF FLAG ASSIGNMENT IS FIXED

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


// GIVEN PROPER FLAG CONDITIONS
// Interface to seek function	
// This function moves the file position pointer to a new location in the file
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	// Also need to make sure this file is actually open
	// If fi is NULL, then this file descriptor isn't being used
	if (fcbArray[fd].fi == NULL)
		{
		return (-1); 					// File isn't open, can't seek
		}
	
	// Get a pointer to the file control block so I can access the file's info
	b_fcb *fcb = &fcbArray[fd];
	off_t newPosition;  // This will hold where we want to move to
	
	// Now I need to figure out the new position based on whence
	// whence tells us where to start counting from, and offset tells us how far
	switch (whence)
		{
		case SEEK_SET:
			// SEEK_SET means start from the beginning of the file
			// So the new position is just the offset value
			newPosition = offset;
			break;
		case SEEK_CUR:
			// SEEK_CUR means start from where we currently are
			// So add the offset to our current position
			// Offset can be positive (move forward) or negative (move backward)
			newPosition = fcb->currentFilePosition + offset;
			break;
		case SEEK_END:
			// SEEK_END means start from the end of the file
			// So add offset to the file size
			// Offset 0 would put us at the end, negative would go back from end
			newPosition = fcb->fi->fileSize + offset;
			break;
		default:
			// If whence isn't one of the three valid options, that's an error
			return (-1);
		}
	
	// Can't seek to a negative position - that would be before the file starts
	if (newPosition < 0)
		{
		return (-1); 					// Invalid position
		}
	
	// Now I need to check if we're moving outside the current buffer window
	// The buffer only holds one block (512 bytes) at a time
	// If we're seeking to a different block, we need to handle the buffer
	int outsideWindow = (newPosition < fcb->bufferStartPosition ||
	                     newPosition >= fcb->bufferStartPosition + BLOCK_SIZE);
	
	// If we're leaving the current buffer window, we need to deal with the buffer
	if (outsideWindow)
		{
		// If there's data in the buffer that hasn't been written to disk yet,
		// we need to flush it first or we'll lose it
		if (fcb->numValidBufferBytes > 0)
			{
			// Figure out which block on disk this buffer corresponds to
			// bufferStartPosition tells us where in the file the buffer starts
			// Divide by BLOCK_SIZE to get which block number that is
			int blockIndex = fcb->bufferStartPosition / BLOCK_SIZE;
			// Add the file's starting location to get the actual disk block
			blockIndex += fcb->fi->location;
			// Write the buffer to disk so we don't lose any changes
			LBAwrite(fcb->buf, 1, blockIndex);
			}
		// Mark the buffer as empty/invalid so the next read or write will
		// know it needs to load the correct block for the new position
		fcb->numValidBufferBytes = 0;
		}
	
	// Update the current file position to where we're seeking to
	fcb->currentFilePosition = newPosition;
	
	// Return the new position
	return (newPosition);
	}

// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	// TO DO 
	// CHECK FOR PROPER FLAGS
	// MAKE SURE SEEK IS PUT IF AND WHERE NECESSARY
	if (!buffer || count <= 0)
		return -2; // Bad call
	if (startup == 0) b_init();  //Initialize our system
	// Step 1: Validate fd.
	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	// Step 2: Get pointer to the file's FCB entry;
	b_fcb *fcb = &fcbArray[fd];
	if (!fcb->fi)
		return -3; // Bad file info
	// Step 3: Check for Append flag.
	// Move the buffer forward to end of file.
	if (fcb->flags & O_APPEND)
		fcb->currentFilePosition = fcb->fi->fileSize;	
	// Step 4: Write Loop
	// Write until we reach number (count) passed in.
	size_t bytesWritten = 0;
	while (bytesWritten < count){
		// Step 5: Check to see if current file position
		// is in the current buffer window.
		// Our file pointer is behind the start of block			  // Is our file position ahead of buffer
		// file starts at 700 ; buf[0] = byte 1024; WRONG FLUSH 	  // buf[0] = byte 0; file starts at 513; WRONG FLUSH
		// In both this cases buffer does NOT contain byte we want to write so we mush flush.
		int outsideWindow = (fcb->currentFilePosition < fcb->bufferStartPosition ||
		fcb->currentFilePosition >= fcb->bufferStartPosition + BLOCK_SIZE);

		if(outsideWindow && fcb->numValidBufferBytes > 0){
			// Flush buffer
			int blockIndex = fcb->bufferStartPosition / BLOCK_SIZE;
			int previousLocation = fcb->fi->location;
			blockIndex +=  previousLocation;
			LBAwrite(fcb->buf, 1, blockIndex);
			fcb->numValidBufferBytes = 0;
		}

		if (outsideWindow && fcb->numValidBufferBytes == 0){
			// Realign the buffer
			fcb->bufferStartPosition = (fcb->currentFilePosition / BLOCK_SIZE) * BLOCK_SIZE;
			
			if (fcb->bufferStartPosition < fcb->fi->fileSize){
				// There is existing data in this block so we need to read it.
				int blockIndex = fcb->bufferStartPosition / BLOCK_SIZE;
				LBAread(fcb->buf, 1, fcb->fi->location + blockIndex);
				int remainingFileBytes = fcb->fi->fileSize - fcb->bufferStartPosition;
				fcb->numValidBufferBytes = (remainingFileBytes < BLOCK_SIZE) ? remainingFileBytes : BLOCK_SIZE;
			} else {
				// We're righting past EOF so we need to start with a 0'd block
				memset(fcb->buf, 0, BLOCK_SIZE);
				fcb->numValidBufferBytes = 0;
			}
		} 
		// Step 6: Compute current offset.
		size_t offset = fcb->currentFilePosition - fcb->bufferStartPosition;
		int spaceInBlock = BLOCK_SIZE - offset;

		// Step 7: Determine number of bytes we can copy into buffer before end of block.
		int remaining = count - bytesWritten;
		int bytesToCopy = (remaining < spaceInBlock) ? remaining : spaceInBlock;

		// Step 8 : Update values
		// Take the minimum of space in block if only one block is left.
		// else use total amount of bytes left to copy
		memcpy(fcb->buf + offset, buffer + bytesWritten, bytesToCopy);
		fcb->currentFilePosition += bytesToCopy;
		bytesWritten += bytesToCopy;
		// Take the max of current valid bytes or offset + bytestocopy
		fcb->numValidBufferBytes = fcb->numValidBufferBytes > offset + bytesToCopy ? fcb->numValidBufferBytes : offset + bytesToCopy;
		// Step 9: Check for full buffer. If full flush
		int blockIndex = fcb->bufferStartPosition / BLOCK_SIZE;
		if (fcb->numValidBufferBytes == BLOCK_SIZE || offset + bytesToCopy == BLOCK_SIZE){
			int previousLocation = fcb->fi->location;
			blockIndex += previousLocation;
			LBAwrite(fcb->buf, 1, blockIndex);
			fcb->numValidBufferBytes=0;
		}
		// Step 10: If writing past old file size
		// Update fileInfo correctly
		if (fcb->currentFilePosition > fcb->fi->fileSize){
			fcb->fi->fileSize = fcb->currentFilePosition;
		}
	}
	return bytesWritten;
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
	
	
	// TO DO 
	// CHECK FOR PROPER FLAGS
	// MAKE SURE SEEK IS PUT IF AND WHERE NECESSARY



	
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
        memcpy(buffer + bytesRead, fcb->buf + (fcb->currentFilePosition - fcb->bufferStartPosition), 
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

	// Flush partial block
	if (fcbArray[fd].numValidBufferBytes > 0) {
        int blockIndex = fcbArray[fd].bufferStartPosition / BLOCK_SIZE;
        LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].fi->location + blockIndex);
    }
	
	updateFileSizeOnDisk(fcbArray[fd].fi->fileName, 
	fcbArray[fd].fi->fileSize, fcbArray[fd].fi->location);

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
	fcbArray[fd].flags = 0;
	//printf("Ending close\n");
    return 0;
	}
