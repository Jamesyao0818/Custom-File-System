#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

void initVector(stringBuilder* v){
    v->subStrings = NULL;
    // How many elements vector can hold
    v->capacity = 0;
    // Length of vector.
    v->size = 0;
}

// Adds token (substring) passed in to vector, returns -1 incorrect call with nulls;
// -2 for realloc failure; -3 for strdup failure.
// Else it returns 0 on success.
int vectorAdd(stringBuilder* v, const char * substring){
    // Check for nulls
    if (!v || !substring) return -1;

    // Grow vector if needed.
    if (v->size == v->capacity){
        // In edge case; 1st step only set initial capacity of 0 to 8.
        // Otherwise double capacity.
        int newCapacity = (v->capacity == 0) ? 8 : v->capacity * 2;
        // Make temp to copy from
        char **tmp = (char **)realloc(v->subStrings, newCapacity * sizeof(*tmp));
        if (!tmp) return -2;

        v->subStrings = tmp;
        v->capacity = newCapacity;
    }
    // Add to vector making sure to increment size while 
    v->subStrings[v->size] = strdup(substring);
    if (!v->subStrings[v->size]) return -3;
    v->size++;
    return 0;
} 

char * build(const stringBuilder *v, char separator){
    // Null check
    if (!v) return NULL;

    // Empty vector
    if (v->size == 0){
        char *emptyVector = (char *)malloc(1);
        if (!emptyVector) return NULL;
        emptyVector[0]='\0';
        return emptyVector;
    }

    // Get total length = sum of subStrings length + separators + '\0'
    size_t length = 1; //Start at one for atleast '\0'
    
    // Iterate along vector of substrings adding up their length in bytes.
    for (size_t i = 0; i < v->size; i++) {
        length += strlen(v->subStrings[i]);
    }

    // If we had "home" "joe" "documents"
    // size of the vector would be 3.
    // BUT we need to add separators
    // eg. "home" "/" "joe" "/" "documents
    // so we need two more spaces. i.e. o.g. size - 1
    length += (v->size - 1);

    // Create an out buffer returnString
    // to return with appropriate length
    char * returnString = (char *)malloc(length);
    // Check for malloc error
    if (!returnString) return NULL;
    // Create a string which we will use to iterate along vector's substrings
    // and add sepator: '/'
    // [string]
    // home joe documents 
    char * position = returnString;
    for (size_t i = 0; i < v->size; i++){
        // So long as we aren't at the very first substring we add '/' first.
        if (i > 0) *position++ = separator;
        //Get length next substring to copy from.
        size_t ssLength = strlen(v->subStrings[i]);
        // Copy from substing to position string.
        memcpy(position, v->subStrings[i], ssLength);
        // Move the length of last substring forward.
        position += ssLength;
    }

    // Put null at end of position string.
    *position = '\0';
    
    return returnString;
}

void freeVector(stringBuilder *v){
    if (!v) return;
    // Free each substring
    for (size_t i = 0; i < v->size; i++){
        free(v->subStrings[i]);
    }
    // Free substring pointer
    free(v->subStrings);
    // Reset 
    v->subStrings = NULL;
    v->size = 0;
    v->capacity = 0;
}