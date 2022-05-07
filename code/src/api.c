#include "../includes/api.h"
#include "../includes/comms.h"

int saveFiles(char* dirname, FileList* flist){

	char* dirpath;
	if((dirpath = calloc(MAX_PATH, sizeof(char))) == NULL){
		return -1;
	}
	dirpath = realpath(dirname, NULL);
	if(dirpath[MAX_PATH-1] != '/'){
		strcat(dirpath, "/");
	}
	while(flist->size > 0){
		File* temp;
		temp = popFile(flist);
		char* filepath;
		if((filepath = calloc(sizeof(char) * MAX_PATH, 0)) == NULL){
			return -1;
		}
		strncpy(filepath, dirpath, strlen(dirpath));
		strcat(filepath, basename(temp->path));
		FILE* towrite;
		if((towrite = fopen(filepath, "w")) == NULL){
			return -1;
		}

		fwrite(temp->data, sizeof(char), temp->datasize, towrite);
		fclose(towrite);
		if(verbose){
			printf(GREEN "File '%s' saved in '%s'.\n" RESET, temp->path, basename(dirname));
		}
	}
	return 0;
}

void printResult(Request* req, Response* res){
	if(verbose){
		puts("--------------------------------------------------------------------------------------------------------");
		printf(" Client PID: %d\n", getpid());
		printf("    Request: %s\n", stringifyCode(req->code));
		if(req->code == REQ_OPEN){
			printf("    Flags:   %s\n", stringifyFlags(req->flags));
		}
		if(req->code != REQ_READN && req->code != REQ_EXIT){
			printf("    of file: %s\n", req->path);
		}
		printf("   Response: ");

		if(res->code == RES_OK){
			printf(GREEN);
		}
		else if(res->code == RES_ALREADYEXISTS){
			printf(YELLOW);
		}
		else{
			printf(RED);
		}
		printf("%s\n" RESET, stringifyCode(res->code));
		puts("--------------------------------------------------------------------------------------------------------");
	}
}

int openConnection(const char* sockname, int msec, const struct timespec abstime){

	PERRNEG((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)));

	struct sockaddr_un socketaddress;
	// change 104 to UNIX_PATH_MAX
	strncpy(socketaddress.sun_path, socketname, 104);
	socketaddress.sun_family = AF_UNIX;

	time_t begin = time(0);

	while(connect(socketfd, (struct sockaddr*)&socketaddress, sizeof(socketaddress)) == -1){

		time_t now = time(0);

		if(abstime.tv_sec >= (begin - now)){
			errno = EAGAIN;
			return -1;
		}
		fprintf(stderr, "Unable to connect to requested socked, trying again in %d ms.", msec);
		usleep(1000 * msec);
	}
	return socketfd;
}

int closeConnection(const char* sockname){
	if(sockname == NULL){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_EXIT, 0, 0, 0, 0, NULL, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;
	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}
	printResult(req, res);

	if(res->code == RES_OK){
		if(close(socketfd) == -1){
			return -1;
		}
	}

	return 0;
}

int openFile(const char* pathname, int flags){

	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}
	Request* req;
	if((req = newRequest(REQ_OPEN, flags, 0, strlen(pathname), 0, (char*)pathname, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;
	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	printResult(req, res);
	//freeRequest(req);

	if(res->flistsize > 0 && verbose){
		printf("Server Eviction of %s done.\n", res->flist->head->file->path);
	}

	if(res->code != RES_OK){
		return 1;
	}

	return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){

	if(pathname == NULL || strlen(pathname) == 0){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_READ, 0, 0, strlen(pathname), 0, (char*)pathname, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;
	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	printResult(req, res);
	if(res->code == RES_OK){
		*size = res->flist->head->file->datasize;
		*buf = res->flist->head->file->data;
	}
	//freeRequest(req);
	if(res->code != RES_OK){
		return 1;
	}

	return 0;
}

int readNFiles(int N, const char* dirname){

	if(N < 0){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_READN, 0, N, 0, 0, NULL, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;
	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	if(res->code == RES_OK){
		if(dirname == NULL){
			if(verbose){
				printResult(req, res);
				puts("Read Files were thrashed since no directory was passed.");
				puts("Use -d <directory path> to store them.");
				FileNode* temp = res->flist->head;
				while(temp != NULL){
					printf("File: %s\nContent: %s\n", temp->file->path, temp->file->data);
					temp = temp->next;
				}
			}
		}
		else{
			saveFiles((char*)dirname, res->flist);
		}
	}
	else{
		printResult(req,res);
	}
	freeRequest(req);
	return 0;
}

int writeFile(const char* pathname, const char* dirname){


	if(pathname == NULL || strlen(pathname) == 0){
		errno = EINVAL;
		return -1;
	}

	FILE* file;
	if((file = fopen(pathname, "r")) == NULL){ 
		return -1;
	}

	// going to find out file's data lenght
	if(fseek(file, 0L, SEEK_END) == -1) {
        return -1;
    }

    int datalen;
    if((datalen = ftell(file)) == -1){
    	return -1;
    }
    // resetting file pointer to file's beginning
    rewind(file);

    char* data;
    if(((data = malloc(datalen * sizeof(char))) == NULL)){
    	return -1;
    }

    if(((fread(data, sizeof(char), datalen, file)) != datalen)){
    	free(data);
    	return -1;
    }
    fclose(file);

	Request* req;
	if((req = newRequest(REQ_WRITE, 0, 0, strlen(pathname), datalen, (char*)pathname, data)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;

	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	if(res->code == RES_OK){
		if(dirname == NULL){
			if(verbose){
				printResult(req, res);
				if(res->flistsize > 0){
					puts(YELLOW "Evicted Files were thrashed since no directory was passed." RESET);
					puts("Use -D <directory path> to store them.");
				}
			}
		}
		else{
			if(verbose){
				printResult(req, res);
			}
			saveFiles((char*)dirname, res->flist);
		}
	}
	else{
		printResult(req,res);
	}
	freeRequest(req);
	return 0;
}

// prende i dati in 'buf' e li salva in 'pathname' in append (utilizzato se si fa la -W di un file che è già presente in memoria)
// N.B. non sono possibili sovrascritture dei dati salvati (necessaria cancellazione e ri-craezione)
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){

	if(pathname == NULL || buf == NULL || size == 0 || strlen(pathname) == 0){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_APPEND, 0, 0, strlen(pathname), size, (char*)pathname, buf)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;

	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	if(res->code == RES_OK){
		if(dirname == NULL){
			if(verbose){
				printResult(req, res);
				if(res->flistsize > 0){
					puts("Evicted Files were thrashed since no directory was passed.");
					puts("Use -D <directory path> to store them.");
				}
			}
		}
		else{
			if(verbose){
				printResult(req, res);
			}
			saveFiles((char*)dirname, res->flist);
		}
	}
	else{
		printResult(req,res);
	}
	freeRequest(req);
	return 0;
}

int lockFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_LOCK, 0, 0, strlen(pathname), 0, (char*)pathname, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;
	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	printResult(req, res);

	freeRequest(req);
	return 0;
	
}

int unlockFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_UNLOCK, 0, 0, strlen(pathname), 0, (char*)pathname, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;

	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	printResult(req, res);

	freeRequest(req);
	return 0;
}

int closeFile(const char* pathname){

	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_CLOSE, 0, 0, strlen(pathname), 0, (char*)pathname, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;

	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	printResult(req, res);

	//freeRequest(req);
	return 0;
}

int removeFile(const char* pathname){
	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}

	if(pathname == NULL){
		errno = EINVAL;
		return -1;
	}

	Request* req;
	if((req = newRequest(REQ_REMOVE, 0, 0, strlen(pathname), 0, (char*)pathname, NULL)) == NULL){
		errno = ENOMEM;
		return -1;
	}

	if(sendRequest(socketfd, req) == -1){
		return -1;
	}

	Response* res;

	if((res = malloc(sizeof(Response))) == NULL){
		return -1;
	}

	if(getResponse(socketfd, res) == -1){
		return -1;
	}

	printResult(req, res);

	freeRequest(req);
	return 0;
}


