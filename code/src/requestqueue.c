#include "../includes/requestqueue.h"
#include "../includes/utils.h"

RequestQueue* newRequestQueue(int maxsize){

	if(maxsize <= 0){
		errno = EINVAL;
		return NULL;
	}

	RequestQueue* new;
	if((new = malloc(sizeof(RequestQueue))) == NULL){
		return NULL;
	}

	PERRNOTZERO(pthread_mutex_init(&(new->queuemtx), NULL));
	PERRNOTZERO(pthread_cond_init(&(new->empty), NULL));
	PERRNOTZERO(pthread_cond_init(&(new->full), NULL));

	new->head = new->tail = NULL;
	new->maxsize = maxsize;

	return new;
}

int freeRequestQueue(RequestQueue* queue){

	while(queue->currentsize != 0){
		ReqNode* temp = queue->head;
		queue->head = (ReqNode*)queue->head->next;
		queue->currentsize--;
		free(temp);
	}

	PERRNOTZERO(pthread_cond_destroy(&queue->empty));
	PERRNOTZERO(pthread_cond_destroy(&queue->full));
	PERRNOTZERO(pthread_mutex_destroy(&queue->queuemtx));
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

	ReqNode* temp = queue->head;
	queue->head = (ReqNode*)queue->head->next;
	queue->currentsize--;

	if(queue->head == NULL){
		queue->tail = NULL;
	}

	if(queue->currentsize == (queue->maxsize)--){
		PERRNOTZERO(pthread_cond_broadcast(&(queue->full)));
	}

	PERRNOTZERO(pthread_mutex_unlock(&(queue->queuemtx)));

	return temp->fd;
}

void printqueue(RequestQueue* queue){

	ReqNode* temp = queue->head;

	while(temp != NULL){
		printf("%d->", temp->fd);
		temp = temp->next;
	}

	puts("NULL");

}
/*
int main(int argc, char const *argv[]){
	
	RequestQueue* queue = newRequestQueue(5);

	for (int i = 1; i < 6; i++){
		printf("putRequest responsecode: %d\n", putRequest(queue, i));
	}
	printqueue(queue);
	int result = 0;
	for (int i = 1; i < 6; i++){
		result = getRequest(queue);
		printf("getRequest result: %d\n", result);
		printqueue(queue);
	}

	return 0;
}

*/





















