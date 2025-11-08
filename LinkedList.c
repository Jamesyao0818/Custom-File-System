#include <stdlib.h>
#include <stdio.h>
#include "LinkedList.h"
#include "FSM.h"

//helpers from FSM.h
//extern int allocate(int blockSize);        // setbit in FSM
//extern void releaseBlock(int blocknum);    // clearbit in FSM
//extern void fsmWrite();

/*create an empty linked list*/
LinkedList* ll_create(){
    LinkedList* list = (LinkedList*)malloc(sizeof(LinkedList));
    if(!list){
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

//return type should be int, not LinkedList*
int ll_append(LinkedList* list, int blocknum){
    if(!list){
        return -1;
    }

    LLNode* newNode = (LLNode*)malloc(sizeof(LLNode));
    if(!newNode){
        return -1;
    }

    newNode->blocknum = blocknum;
    newNode->next = NULL;

    if(list->head == NULL){
        list->head = newNode;
        list->tail = newNode;
    }else{
        list->tail->next = newNode;
        list->tail = newNode;
    }

    list->size++;
    return 0;
}

int ll_get(LinkedList* list, int index){
    if (!list || index < 0 || index >= list->size) {
        return -1;
    }
    
    LLNode* curr = list->head;
    for (int i = 0; i < index; i++) {
        curr = curr->next;
    }
    
    return curr->blocknum;
}


int ll_removeFirst(LinkedList* list){
    if (!list || list->head == NULL) {
        return -1;
    }
    
    LLNode* temp = list->head;
    int blocknum = temp->blocknum;
    
    list->head = list->head->next;
    
    // If list is now empty, update tail
    if (list->head == NULL) {
        list->tail = NULL;
    }
    
    free(temp);
    list->size--;
    
    return blocknum;
}

int ll_size(LinkedList* list){
    if (!list) {
        return 0;
    }
    return list->size;
}

//ll_destroy should free all nodes and the list itself
void ll_destroy(LinkedList* list){
    if (!list) {
        return;
    }
    
    LLNode* curr = list->head;
    while (curr != NULL) {
        LLNode* temp = curr;
        curr = curr->next;
        free(temp);
    }
    
    free(list);
}

void ll_print(LinkedList* list){
    if (!list) {
        printf("List is NULL\n");
        return;
    }
    
    printf("LinkedList (size=%d): ", list->size);
    
    if (list->head == NULL) {
        printf("(empty)\n");
        return;
    }
    
    LLNode* curr = list->head;
    while (curr != NULL) {
        printf("%d", curr->blocknum);
        if (curr->next != NULL) {
            printf(" -> ");
        }
        curr = curr->next;
    }
    printf("\n");
}

// ll_isEmpty should return whether the list is empty
int ll_isEmpty(LinkedList* list){
    if (!list) {
        return 1;  // NULL list is considered empty
    }
    return (list->size == 0);
}


int ll_allocateBlocks(LinkedList* list, int numBlocks, int blockSize) {
    if (!list || numBlocks <= 0) {
        return -1;
    }
    
    // Allocate the requested number of blocks
    for (int i = 0; i < numBlocks; i++) {
        int blocknum = allocate(blockSize);
        
        if (blocknum < 0) {
            // Allocation failed - we need to rollback
            printf("Failed to allocate block %d of %d\n", i + 1, numBlocks);
            // Free all blocks we've allocated so far
            ll_freeBlocks(list);
            return -1;
        }
        
        // Add the block to our linked list
        if (ll_append(list, blocknum) != 0) {
            releaseBlock(blocknum);
            ll_freeBlocks(list);
            return -1;
        }

    }
    
    // Commit FSM changes to disk
    fsmWrite();
    
    return 0;
}

int ll_freeBlocks(LinkedList* list) {
    if (!list) {
        return 0;
    }
    
    int freedCount = 0;
    LLNode* curr = list->head;
    
    while (curr != NULL) {
        releaseBlock(curr->blocknum);
        freedCount++;
        curr = curr->next;
    }
    
    // Commit FSM changes to disk
    if (freedCount > 0) {
        fsmWrite();
    }
    
    return freedCount;
}

LinkedList* ll_createWithAllocation(int numBlocks, int blockSize) {
    if (numBlocks <= 0) {
        return NULL;
    }
    
    // Create the linked list
    LinkedList* list = ll_create();
    if (!list) {
        return NULL;
    }
    
    // Allocate the blocks
    if (ll_allocateBlocks(list, numBlocks, blockSize) != 0) {
        ll_destroy(list);
        return NULL;
    }
    
    return list;
}

void ll_addLast(LinkedList* list, int value) {
    ll_append(list, value);
}

int getNthBlockOfChain(LinkedList* list, int n) {
    return ll_get(list, n);
}