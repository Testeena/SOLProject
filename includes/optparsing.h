#ifndef OPTPARSING_H
#define OPTPARSING_H

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// option node containing a single char (option) and its arguments
// used to parse commands given by clients
typedef struct optnode {
    char opt;
    char* arg;
    struct optnode* next;
} optNode;

typedef struct optlist{
	optNode* head;
	optNode* tail;
} optList;

int putOpt(optList* list, char opt, char* args);
optNode* getFirstOpt(optList* list);
void printOpts(optList* list);
void freeOpt(optNode* node);
void freeList(optList* list);

#endif