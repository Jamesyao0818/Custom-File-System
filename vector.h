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
static void initVector(stringBuilder* v);
int vectorAdd(stringBuilder* v, const char * substring);
char * build(const stringBuilder *v, char separator);
void freeVector(stringBuilder *v);

#endif