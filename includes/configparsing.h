#ifndef CONFIGPARSING_H
#define CONFIGPARSING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>	

#define LINELENGHT 50

#define DEFAULT_MAXSTORAGE 100000
#define DEFAULT_MAXNFILES 100
#define DEFAULT_NWORKERS 10
#define DEFAULT_SOCKNAME "solsock.sk"
#define DEFAULT_LOGFILENAME "log.json"

// config info will be parsed into a linked list with tail insert
// each node has a config name and its value
typedef struct confignode{
	char* name;
	char* value;
	struct confignode* next;
} configNode;

typedef struct configlist{
	configNode* head;
	configNode* tail;
} configList;

int parseFile(configList* outcome, char* toparse);
int putConfig(configList* list, configNode* toput);

#endif