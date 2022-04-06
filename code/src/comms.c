#include "../includes/comms.h"

int addFile(FileList* flist, File* file){

	if(flist == NULL){
		if((flist = malloc(sizeof(FileList))) == NULL){
			return -1;
		}
	}
	else{

		File* toadd;
		if((toadd = newcommsFile(file->path, file->data)) == NULL){
			return -1;
		}

		if(flist->head == flist->tail == NULL){
			flist->head = flist->tail = toadd;
			flist->size++;
			return 0;
		}
		else{
			if(flist->head == flist->tail){
				flist->head->next = toadd;
				flist->tail = toadd;
				flist->size++;
				return 0;
			}
			else{
				flist->tail->next = toadd;
				flist->tail = toadd;
				flist->size++;
				return 0;
			}
		}
	}
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
		if((toreturn->flist = malloc(sizeof(FileList))) == NULL){
			free(toreturn);
			return NULL;
		}
	}
	else{
		toreturn->flist = flist;
		toreturn->flistsize = flist->size;
	}
	return toreturn;
}

int freeRequest(Request* request){
	if(request != NULL){
		freeFile(request->file);
	}
	free(request->file);
	free(request);
	return 0;
}

int freeResponse(Response* response){
	if(response != NULL){
		freeFileList(response->flist);
	}
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

	if(request->pathlen){
		if(writen(sockfd, request->path, request->pathlen) == -1){
			return -1;
		}
	}

	if(request->datasize){
		if(writen(sockfd, request->data, request->datasize) == -1){
			return -1;
		}
	}

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

	if(response->flist != NULL){
		FileList* temp = flist->head;
		// maybe cycle with flistsize?
		while(temp != NULL){
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
	return 0;
}

int getRequest(int sockfd, Request* request){
	if((request = malloc(sizeof(Request))) == NULL){
		return -1;
	}

	if(read(sockfd, request, sizeof(Request)) == -1){
		return -1;
	}

	if(request->pathlen > 0){
		if(readn(sockfd, request->path, request->pathlen) == -1){
			return -1;
		}
	}
	else{
		request->path = NULL;
	}

	if(request->datasize > 0){
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
	if((response = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(read(sockfd, response, sizeof(Response)) == -1){
		return -1;
	}


	File* toget;
	if((toget = malloc(sizeof(File))) == NULL){
		return -1;
	}

	for (int i = 0; i < response->flistsize; i++){
		
		if(read(sockfd, toget, sizeof(File)) == -1){
			return -1;
		}

		if(readn(sockfd, toget->path, toget->pathlen) == -1){
			return -1;
		}

		if(readn(sockfd, toget->data, toget->datasize) == -1){
			return -1;
		}
		// use temp variables + newfile instead?
		addFile(response->flist, toget);
	}

	return 0;
}

char* stringifyCode(int code){
	switch(code){
		case RES_OK:
			return "OK"
		case RES_DENIED:
			return "DENIED"
		case RES_BADREQ:
			return "BADREQ"
		case RES_NOTFOUND:
			return "NOTFOUND"
		case RES_ALREADYEXISTS:
			return "ALREADYEXISTS"
		case RES_NOMEM:
			return "NOMEM"
		case REQ_OPEN:
			return "OPEN"
		case REQ_CLOSE:
			return "CLOSE"
		case REQ_READ:
			return "READ"
		case REQ_READN:
			return "READN"
		case REQ_WRITE:
			return "WRITE"
		case REQ_APPEND:
			return "APPEND"
		case REQ_REMOVE:
			return "REMOVE"
		case REQ_LOCK:
			return "LOCK"
		case REQ_UNLOCK:
			return "UNLOCK"
	}
	return "UNKNOWN";
}





