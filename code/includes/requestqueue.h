#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

typedef struct reqnode{
	int fd;
	void* next;
} ReqNode;

typedef struct requestqueue{

	// size values
	int maxsize;
	int currentsize;

	// First Request reference
	ReqNode* head;
	// Last Request reference
	ReqNode* tail;

	// Concurrence Variables
	pthread_mutex_t queuemtx;
	pthread_cond_t empty;
	pthread_cond_t full;

} RequestQueue;

RequestQueue* newRequestQueue(int maxsize);
int freeRequestQueue(RequestQueue* queue);
int putRequestor(RequestQueue* queue, int fd);
int getRequestor(RequestQueue* queue);