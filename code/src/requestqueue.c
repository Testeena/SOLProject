#include "../includes/requestqueue.h"
#include "../includes/utils.h"

RequestQueue* newRequestQueue(int maxsize){

	if(maxsize <= 0){
		errno = EINVAL;
		return NULL;
	}

	RequestQueue* new;
	if((new = calloc(sizeof(RequestQueue), sizeof(RequestQueue))) == NULL){
		return NULL;
	}

	PERRNOTZERO(pthread_mutex_init(&(new->queuemtx), NULL));
	PERRNOTZERO(pthread_cond_init(&(new->empty), NULL));
	PERRNOTZERO(pthread_cond_init(&(new->full), NULL));

	new->head = new->tail = NULL;
	new->maxsize = maxsize;
	new->currentsize = 0;

	return new;
}

int freeRequestQueue(RequestQueue* queue){
	PERRNOTZERO((pthread_mutex_lock(&(queue->queuemtx))));
	while(queue->head != NULL){
		ReqNode* temp = queue->head;
		queue->head = queue->head->next;
		queue->currentsize--;
		temp->next = NULL;
		free(temp);
	}
	PERRNOTZERO((pthread_mutex_unlock(&(queue->queuemtx))));

	PERRNOTZERO(pthread_cond_destroy(&(queue->empty)));
	PERRNOTZERO(pthread_cond_destroy(&(queue->full)));
	PERRNOTZERO(pthread_mutex_destroy(&(queue->queuemtx)));
	free(queue);
	return 0;
}


int putRequestor(RequestQueue* queue, int fd){

	if(queue == NULL){
		errno = EINVAL;
		return -1;
	}

	ReqNode* toput;
	if((toput = (ReqNode*)malloc(sizeof(ReqNode))) == NULL){
		return -1;
	}

	toput->fd = fd;
	toput->next = NULL;

	PERRNOTZERO(pthread_mutex_lock(&(queue->queuemtx)));

	while(queue->currentsize == queue->maxsize){
		PERRNOTZERO(pthread_cond_wait(&(queue->full), &(queue->queuemtx)));
	}

	if(queue->head == NULL || queue->tail == NULL){
		queue->head = queue->tail = (ReqNode*)toput;
	}
	else{
		queue->tail->next = (ReqNode*)toput;
		queue->tail = (ReqNode*)toput;
	}
	queue->currentsize++;

	if(queue->currentsize == 1){
		PERRNOTZERO(pthread_cond_broadcast(&(queue->empty)));
	}

	PERRNOTZERO(pthread_mutex_unlock(&(queue->queuemtx)));
	return 0;
}

int getRequestor(RequestQueue* queue){

	if(queue == NULL){
		errno = EINVAL;
		return -1;
	}

	PERRNOTZERO(pthread_mutex_lock(&(queue->queuemtx)));

	while(queue->currentsize == 0){
		PERRNOTZERO(pthread_cond_wait((&queue->empty), &(queue->queuemtx)));
	}

	int req;
	ReqNode* temp = queue->head;
	queue->head = (ReqNode*)queue->head->next;
	queue->currentsize--;
	req = temp->fd;
	free(temp);

	if(queue->head == NULL){
		queue->tail = NULL;
	}

	if(queue->currentsize == (queue->maxsize)--){
		PERRNOTZERO(pthread_cond_broadcast(&(queue->full)));
	}

	PERRNOTZERO(pthread_mutex_unlock(&(queue->queuemtx)));

	return req;
}

void printqueue(RequestQueue* queue){
	ReqNode* temp = queue->head;
	while(temp != NULL){
		printf("%d->", temp->fd);
		temp = temp->next;
	}
	puts("NULL");
}