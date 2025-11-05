#ifndef LINKEDLIST_H
#define LINKEDLIST_H

typedef struct LLNode{
    int blocknum;
    struct LLNode* next;
}LLNode;

typedef struct LinkedList
{
    LLNode* head;
    LLNode* tail;
    int size;
}LinkedList;

LinkedList* ll_create();

int ll_append(LinkedList* list, int blocknum);

int ll_get(LinkedList* list, int index);

int getNthBlockOfChain(LinkedList* list, int n);

int ll_removeFirst(LinkedList* list);

int ll_size(LinkedList* list);

void ll_destroy(LinkedList* list);

void ll_print(LinkedList* list);

int ll_isEmpty(LinkedList* list);

int ll_allocateBlocks(LinkedList* list, int numBlocks, int blockSize);

int ll_freeBlocks(LinkedList* list);

LinkedList* ll_createWithAllocation(int numBlocks, int blockSize);

#endif