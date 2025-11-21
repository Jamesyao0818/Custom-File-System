#ifndef _FILEINFO_H
#define _FILEINFO_H


typedef struct fileInfo {
	char fileName[64];		//filename
	int fileSize;			//file size in bytes
	int location;			//starting lba (block number) for the file data
	int allocatedBlocks;		//Number of blocks currently allocated in FSM.
} fileInfo;

fileInfo * getFileInfo (char * filename, int flags);
int updateFileSizeOnDisk(char *pathname, int newSize, int newLocation);
int ensureAllocation(int fileSize, int location);
#endif