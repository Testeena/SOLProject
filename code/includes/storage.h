#ifndef STORAGE_H
#define STORAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#include "icl_hash.h"

// big prime number used by hashtable 
#define BUCKETS 1009
#define MAXPATHLEN 256

typedef struct fdnode{
	int fd;
	struct fdnode* next;
} FdNode;

typedef struct fdlist{
	FdNode* head;
	FdNode* tail;
} FdList;

typedef struct filepathnode{
	char* filepath;
	struct filepathnode* next;
} FilepathNode;

typedef struct filepathlist{
	FilepathNode* head;
	FilepathNode* tail;
} FilepathList;

typedef struct file{

	int pathlen;
	char* path;

	int datasize;
	char* data;

	int writer;
	int readers;
	int creator;			// client who successfully did openfile with O_CREATE
	int locker;	 			// client who successfully did lockFile
	FdList* lockwaiters;	// clients waiting to do lockFile
	FdList* openers;		// clients that successfully opened the file

	pthread_mutex_t mtx;
	pthread_mutex_t ordering;
	pthread_cond_t writecond;

} File;

typedef struct storage{

	int maxfiles;
	int maxsize;

	int currfiles;
	int currsize;

	int maxreachedfiles;
	int maxreachedsize;

	int evicted;

	FilepathList* filepaths;

	icl_hash_t* hashtable;

	pthread_mutex_t storagemtx;
} Storage;

File* newFile(char* filepath, char* data);
int freeFile(File* file);

FdList* newFdList();
int putFd(FdList* list, int fd);
int getFirstFd(FdList* list);
int removeFd(FdList* list, int fd);
int checkFd(FdList* list, int fd)
int freeFdList(FdList* list);

int addFilepath(FilepathList* list , char* filepath);
int popFilepathList(FilepathList* list, char* poppedpath);
int deleteFilepathNode(FilepathList* list, char* path);
int freeFilepathList(FilepathList* list);

Storage* newStorage(int maxfiles, int maxsize);
int freeStorage(Storage* storage);

#endif