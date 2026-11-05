#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fsLow.h"
#include "FAT.h"
#include "FSM.h"

// In-memory copy of the FAT. fat[b] = next block after b, or FAT_END.
static int32_t *fat = NULL;
static unsigned int fatEntries = 0;    // one entry per block in the volume
static unsigned int fatBlockSize = 0;
static unsigned int fatStartLBA = 0;
static int fatBlocks = 0;              // how many disk blocks the FAT occupies

// How many blocks are needed to store one int32 per volume block.
int fatNumBlocks(unsigned int numBlocks, unsigned int blockSize) {
    size_t bytes = (size_t)numBlocks * sizeof(int32_t);
    return (int)((bytes + blockSize - 1) / blockSize);
}

static int fatSetup(unsigned int numBlocks, unsigned int blockSize, unsigned int startLBA) {
    fatEntries = numBlocks;
    fatBlockSize = blockSize;
    fatStartLBA = startLBA;
    fatBlocks = fatNumBlocks(numBlocks, blockSize);
    free(fat);
    fat = malloc((size_t)fatBlocks * blockSize);
    if (!fat) {
        printf("Error: malloc failed in fatSetup\n");
        return -1;
    }
    return 0;
}

static int validLBA(int lba) {
    return lba >= 0 && (unsigned int)lba < fatEntries;
}

// Called when formatting a new volume.
int initFAT(unsigned int numBlocks, unsigned int blockSize, unsigned int startLBA) {
    if (fatSetup(numBlocks, blockSize, startLBA) != 0) return -1;
    // 0xFF in every byte makes every int32 equal -1 (FAT_END)
    memset(fat, 0xFF, (size_t)fatBlocks * blockSize);
    // The FAT's own blocks must be marked used in the free space map
    for (int i = 0; i < fatBlocks; i++) {
        fsmReserveLBA(startLBA + i);
    }
    printf("FAT initialized: %d blocks starting at LBA %u\n", fatBlocks, startLBA);
    return writeFAT();
}

// Called when an existing volume is found.
int loadFAT(unsigned int numBlocks, unsigned int blockSize, unsigned int startLBA) {
    if (fatSetup(numBlocks, blockSize, startLBA) != 0) return -1;
    uint64_t blocksRead = LBAread(fat, fatBlocks, startLBA);
    if (blocksRead != (uint64_t)fatBlocks) {
        printf("Error: loadFAT read %lu of %d blocks\n", blocksRead, fatBlocks);
        free(fat);
        fat = NULL;
        return -1;
    }
    printf("FAT loaded: %d blocks starting at LBA %u\n", fatBlocks, startLBA);
    return 0;
}

// Writes the whole FAT to disk.
int writeFAT(void) {
    if (!fat) return -1;
    uint64_t written = LBAwrite(fat, fatBlocks, fatStartLBA);
    return (written == (uint64_t)fatBlocks) ? 0 : -1;
}

int fatGetNext(int lba) {
    if (!fat || !validLBA(lba)) return FAT_END;
    return fat[lba];
}

// Links lba -> next, then writes only the one FAT block that changed.
int fatSetNext(int lba, int next) {
    if (!fat || !validLBA(lba)) return -1;
    fat[lba] = next;
    int entriesPerBlock = fatBlockSize / sizeof(int32_t);
    int blockOffset = lba / entriesPerBlock;
    char *blockStart = (char *)fat + (size_t)blockOffset * fatBlockSize;
    LBAwrite(blockStart, 1, fatStartLBA + blockOffset);
    return 0;
}

// Returns the LBA of the n-th block (0-based) of the chain starting at head,
// or -1 if the chain is shorter than that.
int fatGetNthBlock(int head, int n) {
    if (!fat || !validLBA(head) || n < 0) return -1;
    int cur = head;
    for (int i = 0; i < n; i++) {
        cur = fat[cur];
        if (cur == FAT_END || !validLBA(cur)) return -1;
    }
    return cur;
}

// Releases every block in the chain. Returns how many blocks were freed.
int fatFreeChain(int head) {
    if (!fat || !validLBA(head)) return 0;
    int cur = head;
    int freed = 0;
    // freed < fatEntries protects against an accidental loop in the chain
    while (cur != FAT_END && validLBA(cur) && (unsigned int)freed < fatEntries) {
        int next = fat[cur];
        fat[cur] = FAT_END;
        fsmReleaseLBA(cur);
        freed++;
        cur = next;
    }
    writeFAT();
    return freed;
}