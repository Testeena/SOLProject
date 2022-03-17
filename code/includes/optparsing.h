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
} OptNode;

typedef struct optlist{
	OptNode* head;
	OptNode* tail;
} OptList;

OptList* newOptList(void);
int putOpt(OptList* list, char opt, char* args);
OptNode* getFirstOpt(OptList* list);
void printOpts(OptList* list);
void freeOptNode(OptNode* node);
void freeOptList(OptList* list);

#endif