#ifndef COMMS_H
#define COMMS_H

#include "storage.h"
#include "utils.h"

// openFile flags
#define O_NONE	 0
#define O_CREATE 1
#define O_LOCK 	 2
#define O_BOTH   3

// client's Request Codes

#define REQ_OPEN	11
#define REQ_CLOSE	12
#define REQ_READ 	13
#define REQ_READN	14
#define REQ_WRITE	15
#define REQ_APPEND	16
#define REQ_REMOVE	17
#define REQ_LOCK 	18
#define REQ_UNLOCK	19
#define REQ_EXIT	20

// server's Response Codes

#define RES_OK				1
#define RES_DENIED			2
#define RES_BADREQ			3
#define RES_NOTFOUND		4
#define RES_ALREADYEXISTS	5
#define RES_NOMEM			6

typedef struct filenode{
	File* file;
	struct filenode* next;
} FileNode;

typedef struct filelist{
	int size;
	FileNode* head;
	FileNode* tail;
}FileList;

typedef struct request{
	int code;		// REQ_?
	int flags;		// 0 = NONE, 1 = O_CREATE, 2 = O_LOCK, 3 = O_CREATE | O_LOCK
	int params;		// 'N' of readNFiles will be stored here
	int pathlen;
	int datasize;
	char* path;
	char* data;
} Request;

typedef struct response{
	int code; 		// RES_?
	int flistsize;
	FileList* flist;
} Response;

int addFile(FileList* flist, File* file);
File* popFile(FileList* flist);
int freeFileList(FileList* flist);

Request* newRequest(int code, int flags, int params, int pathlen, int datasize, char* path, char* data);
Response* newResponse(int code, FileList* flist);
int freeRequest(Request* request);
int freeResponse(Response* response);
int sendRequest(int sockfd, Request* request);
int sendResponse(int sockfd, Response* response);
int getRequest(int sockfd, Request* request);
int getResponse(int sockfd, Response* response);
char* stringifyCode(int code);
char* stringifyFlags(int flags);
void printRequest(Request* req);
void printResponse(Response* res);

#endif