/**************************************************************
* Class::  CSC-415-01 Fall 2025
* Name:: Joseph Bowen
* Student IDs:: 922966782
* GitHub-Name:: josephb333
* Group-Name:: Don't Worry We Don't Byte
* Project:: Basic File System
*
* File:: Vector.h
*
* Description:: This file contains all necessary functions prototypes
* for our stringBuilder vector.
*
**************************************************************/

#ifndef VECTOR_H
#define VECTOR_H

typedef struct stringBuilder{
    char **subStrings;
    // Length of vector
    int size;
    // How many elements vector can hold
    int capacity;
} stringBuilder;

//Vector functions
void initVector(stringBuilder* v);
int vectorAdd(stringBuilder* v, const char * substring);
char * build(const stringBuilder *v, char separator);
void freeVector(stringBuilder *v);

#endif