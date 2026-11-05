#ifndef _FAT_H
#define _FAT_H
#include <stdint.h>

#define FAT_END -1

int fatNumBlocks(unsigned int numBlocks, unsigned int blockSize);
int initFAT(unsigned int numBlocks, unsigned int blockSize, unsigned int startLBA);
int loadFAT(unsigned int numBlocks, unsigned int blockSize, unsigned int startLBA);
int writeFAT(void);
int fatGetNext(int lba);
int fatSetNext(int lba, int next);
int fatGetNthBlock(int head, int n);
int fatFreeChain(int head);

#endif