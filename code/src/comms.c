#include "../includes/comms.h"

int addFile(FileList* flist, File* file){

	if(file == NULL){
		return 0;
	}

	if(flist == NULL){
		if((flist = malloc(sizeof(FileList))) == NULL){
			return -1;
		}
	}

	File* toadd;
	if((toadd = newcommsFile(file->path, file->data)) == NULL){
		return -1;
	}
	FileNode* nodetoadd;
	if((nodetoadd = malloc(sizeof(FileNode))) == NULL){
		return -1;
	}
	nodetoadd->file = toadd;

	if(flist->head == NULL){
		flist->head = nodetoadd;
		flist->tail = nodetoadd;
		flist->size++;
		return 0;
	}
	else{
		if(flist->head == flist->tail){
			flist->head->next = nodetoadd;
			flist->tail = nodetoadd;
			flist->size++;
			return 0;
		}
		else{
			flist->tail->next = nodetoadd;
			flist->tail = nodetoadd;
			flist->size++;
			return 0;
		}
	}

	return -1;
}

File* popFile(FileList* flist){

	if(flist == NULL || flist->head == NULL){
		return NULL;
	}
	else{
		FileNode* temp = flist->head;
		flist->head = flist->head->next;
		flist->size--;
		return temp->file;
	}
}

int freeFileList(FileList* flist){

	while(flist->head != NULL){
		FileNode* temp = flist->head;
		flist->head = flist->head->next;
		printf("flist->head == NULL? %d\n", flist->head == NULL);
		printf("going to free: %s\n", temp->file->path);
		freeFile(temp->file);
		free(temp);
	}
	return 0;
}

Request* newRequest(int code, int flags, int params, int pathlen, int datasize, char* path, char* data){

	Request* toreturn;
	if((toreturn = malloc(sizeof(Request))) == NULL){
		return NULL;
	}

	toreturn->code = code;
	toreturn->flags = flags;
	toreturn->params = params;
	toreturn->pathlen = pathlen;
	toreturn->datasize = datasize;

	if((toreturn->path = malloc(pathlen * sizeof(char))) == NULL){
		free(toreturn);
		return NULL;
	}
	else{
		toreturn->path = path;
	}

	if((toreturn->data = malloc(datasize * sizeof(char))) == NULL){
		free(toreturn->path);
		free(toreturn);
		return NULL;
	}
	else{
		toreturn->data = data;
	}

	return toreturn;
}

Response* newResponse(int code, FileList* flist){

	Response* toreturn;
	if((toreturn = malloc(sizeof(Response))) == NULL){
		return NULL;
	}

	toreturn->code = code;

	if(flist == NULL){
		toreturn->flistsize = 0;
		toreturn->flist = NULL;
	}
	else{
		toreturn->flist = flist;
		toreturn->flistsize = flist->size;
	}
	return toreturn;
}

int freeRequest(Request* request){
	if(request->path){
		free(request->path);
	}
	if(request->data){
		free(request->data);
	}
	free(request);
	return 0;
}

int freeResponse(Response* response){
	if(response != NULL){
		freeFileList(response->flist);
	}
	puts("finished freeing response->flist");
	free(response->flist);
	free(response);
	return 0;
}

int sendRequest(int sockfd, Request* request){

	if(request == NULL){
		errno = EINVAL;
		return -1;
	}

	if(write(sockfd, request, sizeof(Request)) == -1){
		return -1;
	}

	if(request->pathlen > 0){
		if(writen(sockfd, request->path, request->pathlen) == -1){
			return -1;
		}
	}

	if(request->datasize > 0){
		if(writen(sockfd, request->data, request->datasize) == -1){
			return -1;
		}
	}
	//printf(YELLOW "comms:" RESET);
	//printf(" Sent %d:%s Request to %d\n", request->code, stringifyCode(request->code), sockfd);
	return 0;
}

int sendResponse(int sockfd, Response* response){

	if(response == NULL){
		errno = EINVAL;
		return -1;
	}

	if(write(sockfd, response, sizeof(Response)) == -1){
		return -1;
	}
	if(response->flistsize > 0){
		FileNode* temp = response->flist->head;
		for (int i = 0; i < response->flistsize; i++){
			if(write(sockfd, temp->file, sizeof(File)) == -1){
				return -1;
			}
			
			if(writen(sockfd, temp->file->path, temp->file->pathlen) == -1){
				return -1;
			}

			if(writen(sockfd, temp->file->data, temp->file->datasize) == -1){
				return -1;
			}
			temp = temp->next;
		}
	}
	printf(YELLOW "comms:" RESET);
	printf(" Sent %d:%s Response to %d\n", response->code, stringifyCode(response->code), sockfd);
	return 0;
}

int getRequest(int sockfd, Request* request){

	if(read(sockfd, request, sizeof(Request)) == -1){
		return -1;
	}
	printf(YELLOW "comms:" RESET);
	printf(" Got %d:%s Request from %d\n", request->code, stringifyCode(request->code), sockfd);
	if(request->pathlen > 0){

		if((request->path = malloc(request->pathlen * sizeof(char))) == NULL){
			return -1;
		}

		if(readn(sockfd, request->path, request->pathlen) == -1){
			return -1;
		}
	}
	else{
		request->path = NULL;
	}

	if(request->datasize > 0){

		if((request->data = malloc(request->datasize * sizeof(char))) == NULL){
			return -1;
		}

		if(readn(sockfd, request->data, request->datasize) == -1){
			return -1;
		}
	}
	else{
		request->data = NULL;
	}
	
	return 0;
}

int getResponse(int sockfd, Response* response){

	if(read(sockfd, response, sizeof(Response)) == -1){
		return -1;
	}
	if(response->flistsize > 0){

		for (int i = 0; i < response->flistsize; i++){

			File* toget;
			if((toget = malloc(sizeof(File))) == NULL){
				return -1;
			}			
			if(read(sockfd, toget, sizeof(File)) == -1){
				return -1;
			}

			if((toget->path = malloc(toget->pathlen * sizeof(char))) == NULL){
				return -1;
			}

			if(readn(sockfd, toget->path, toget->pathlen) == -1){
				return -1;
			}

			if((toget->data = malloc(toget->datasize * sizeof(char))) == NULL){
				return -1;
			}

			if(readn(sockfd, toget->data, toget->datasize) == -1){
				return -1;
			}

			addFile(response->flist, toget);
		}
	}
	//printf(YELLOW "comms:" RESET);
	//printf(" Got %d:%s Response from %d\n", response->code, stringifyCode(response->code), sockfd);
	return 0;
}

char* stringifyFlags(int flags){
	switch(flags){
		case O_NONE:
			return "O_NONE";
		case O_CREATE:
			return "O_CREATE";
		case O_LOCK:
			return "O_LOCK";
		case O_BOTH:
			return "O_BOTH";
	}
	return "UNKNOWN";
}

char* stringifyCode(int code){
	switch(code){
		case RES_OK:
			return "OK";
		case RES_DENIED:
			return "DENIED";
		case RES_BADREQ:
			return "BADREQ";
		case RES_NOTFOUND:
			return "NOTFOUND";
		case RES_ALREADYEXISTS:
			return "ALREADYEXISTS";
		case RES_NOMEM:
			return "NOMEM";
		case REQ_OPEN:
			return "OPEN";
		case REQ_CLOSE:
			return "CLOSE";
		case REQ_READ:
			return "READ";
		case REQ_READN:
			return "READN";
		case REQ_WRITE:
			return "WRITE";
		case REQ_APPEND:
			return "APPEND";
		case REQ_REMOVE:
			return "REMOVE";
		case REQ_LOCK:
			return "LOCK";
		case REQ_UNLOCK:
			return "UNLOCK";
		case REQ_EXIT:
			return "EXIT";
	}
	return "UNKNOWN";
}

void printRequest(Request* req){
	if(req == NULL){
		puts("Request is empty.");
	}

	else{
		printf("***Request***\ncode: %d->%s\nflags = %d\nparams = %d\npathlen = %d\ndatasize = %d\n", req->code, stringifyCode(req->code), req->flags, req->params, req->pathlen, req->datasize);
		if(req->pathlen){
			printf("path: %s\n", req->path);
		}
		if(req->data){
			printf("data: %s\n", req->data);
		}
	}
}

void printResponse(Response* res){
	if(res == NULL){
		puts("Response is empty.");
	}

	else{
		printf("***Response***\ncode: %d->%s\n", res->code, stringifyCode(res->code));
		printf("flistsize: %d\n", res->flistsize);
	}
}

