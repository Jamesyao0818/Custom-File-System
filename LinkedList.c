/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Joseph Bowen & James Yao
* Student IDs:: 92296678 & 923978442
* GitHub-Name:: josephb333 & Jamesyao0818
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: LinkedList.c
*
* Description:: Implementation of linked list data structure for
* managing discontiguous block allocation in the file system.
*
**************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "LinkedList.h"

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

LinkedList* ll_append(LinkedList* list, int blocknum){
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

int getNthBlockOfChain(LinkedList* list, int n){
    return ll_get(list, n);
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

void ll_destroy(LinkedList* list){
    if (!list) {
        return 1;
    }
    return (list->size == 0);
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

int ll_isEmpty(LinkedList* list){
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
            // Failed to append - free this block and rollback
            freeBlock(blocknum);
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
        freeBlock(curr->blocknum);
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