#include "../includes/utils.h"
#include "../includes/storage.h"

FdList* newFdList(){
	FdList* toreturn;
	if((toreturn = malloc(sizeof(FdList))) == NULL){
		return NULL;
	}
	else{
		toreturn->head = toreturn->tail = NULL;
		return toreturn;
	}
}

int putFd(FdList* list, int fd){
	if(list != NULL){
		FdNode* newfd;
		if((newfd = malloc(sizeof(FdNode))) == NULL){
			return -1;
		} 
		newfd->fd = fd;
		newfd->next = NULL;

		if(list->head == NULL){
			list->head = list->tail = newfd;
		}
		else if(list->tail != NULL){
			list->tail->next = newfd;
			list->tail = newfd;
		}
		return 0;
	}
	else{
		errno = EINVAL;
		return -1;
	}
}

int getFirstFd(FdList* list){
	if(list == NULL){
		errno = EINVAL;
		return -1;
	}

	if(list->head == NULL){
		return -1;
	}
	else{
		FdNode* got = list->head;
		list->head = list->head->next;
		int toreturn = got->fd;
		free(got);
		return toreturn;
	}
}

int removeFd(FdList* list, int fd){
	if(list == NULL || list->head == NULL){
		return 0;
	}
	else{
		FdNode* temp = list->head;
		if(temp->fd == fd){
			list->head = list->head->next;
			free(temp);
			return 0;
		}
		else{
			while(temp->next->fd != fd && temp != NULL){
				temp = temp->next;
			}
			if(temp != NULL){
				FdNode* tofree = temp->next;
				temp->next = temp->next->next;
				free(tofree);
			}
			return 0;
		}
	}
	return -1;
}

int checkFd(FdList* list, int fd){
	if(list == NULL || list->head == NULL){
		return -1;
	}
	else{
		FdNode* temp = list->head;
		while(temp != NULL){
			if(temp->fd == fd){
				return 1;
			}
			temp = temp->next;
		}
	}
	return -1;
}

void printFdList(FdList* list){
	FdNode* temp = list->head;
	printf("FdList: ");
	while(temp != NULL){
		printf("%d->", temp->fd);
		temp = temp->next;
	}
	puts("NULL");
}

int freeFdList(FdList* list){
	while(list->head != NULL){
		FdNode* temp = list->head;
		list->head = list->head->next;
		free(temp);
	}
	return 0;
}

// ----------------------------------------------------------------------------------

File* newFile(char* filepath, char* data){

	File* new;
	if((new = malloc(sizeof(File))) == NULL){
		return NULL;
	}

	if((new->path = malloc(MAX_PATH * sizeof(char))) == NULL){
		free(new);
		return NULL;
	}

	new->pathlen = strlen(filepath);
	strncpy(new->path, filepath, MAX_PATH);
	strcat(new->path, "\0");

	if(data == NULL){
		new->datasize = 0;
		new->data = NULL;
	}
	else{
		new->datasize = strlen(data);
		if((new->data = malloc(strlen(data) * sizeof(char))) == NULL){
			free(new->path);
			free(new);
			return NULL;
		}
		strncpy(new->data, data, strlen(data));
	}

	new->lockwaiters = newFdList();
	new->openers = newFdList();
	PERRNOTZERO(pthread_mutex_init(&(new->mtx), NULL));
	PERRNOTZERO(pthread_mutex_init(&(new->ordering), NULL));
	PERRNOTZERO(pthread_cond_init(&(new->writecond), NULL));

	return new;
}

File* newcommsFile(char* filepath, char* data){
	File* new;
	if((new = malloc(sizeof(File))) == NULL){
		return NULL;
	}

	new->pathlen = strlen(filepath);
	if((new->path = malloc(MAX_PATH * sizeof(char))) == NULL){
		free(new);
		return NULL;
	}
	strncpy(new->path, filepath, strlen(filepath));
	if(data == NULL){
		new->datasize = 0;
		new->data = NULL;
	}
	else{
		new->datasize = strlen(data);
		if((new->data = malloc(strlen(data) * sizeof(char))) == NULL){
			free(new->path);
			free(new);
			return NULL;
		}
		strncpy(new->data, data, strlen(data));
	}
	new->lockwaiters = NULL;
	new->openers = NULL;

	return new;
}

void freeFile(File* file){
	if(file->path != NULL){
		free(file->path);
	}

	if(file->data != NULL){
		free(file->data);
	}

	if(file != NULL && file->lockwaiters != NULL){
		freeFdList(file->lockwaiters);
		freeFdList(file->openers);
		PERRNOTZERO(pthread_cond_destroy(&(file->writecond)));
		PERRNOTZERO(pthread_mutex_destroy(&(file->mtx)));
		PERRNOTZERO(pthread_mutex_destroy(&(file->ordering)));
	}
	free(file);
}

// ----------------------------------------------------------------------------------

int addFilepath(FilepathList* list , char* filepath){

	FilepathNode* new;

	if((new = malloc(sizeof(FilepathNode))) == NULL){
		return -1;
	}

	if((new->filepath = malloc(strlen(filepath) * sizeof(char))) == NULL){
		free(new);
		return -1;
	} 

	strncpy(new->filepath, filepath, strlen(filepath));

	if(list->head == NULL && list->tail == NULL){
		list->head = list->tail = new;
		return 0;
	}
	else{
		list->tail->next = new;
		list->tail = new;
		return 0;
	}

	return -1;
}

int popFilepathList(FilepathList* list, char* poppedpath){

	if(list == NULL || list->head == NULL){
		return 1;
	}
	else{
		FilepathNode* temp = list->head;
		list->head = list->head->next;
		if(list->head == NULL){
			list->tail = NULL;
		}
		strncpy(poppedpath, temp->filepath, strlen(temp->filepath));
		return 0;
	}

	return -1;
}

int deleteFilepathNode(FilepathList* list, char* path){
	if(list == NULL || list->head == NULL){
		return 0;
	}
	else{
		FilepathNode* temp = list->head;
		if(strcmp(temp->filepath, path) == 0){
			list->head = list->head->next;
			if(list->head == NULL){
				list->tail = NULL;
			}
			free(temp->filepath);
			free(temp);
			return 0;
		} 
		else{
			while(temp->next != NULL && strcmp(temp->next->filepath, path) != 0){
				temp = temp->next;
			}
			if(temp != NULL){
				FilepathNode* tofree = temp->next;
				temp->next = temp->next->next;
				free(tofree->filepath);
				free(tofree);
				return 0;
			}
		}
	}
	return -1;
}

int freeFilepathList(FilepathList* list){

	if(list == NULL || list->head == NULL){
		return 0;
	}

	else{
		while(list->head != NULL){
			FilepathNode* temp = list->head;
			list->head = list->head->next;
			free(temp->filepath);
			free(temp);
		}
		return 0;
	}

	return -1;
}

void printFilepaths(FilepathList* list){
	if(list->head != NULL){
		printf("List of stored files:\n");
		FilepathNode* temp = list->head;
		while(temp != NULL){
			if(temp->filepath != NULL){
				printf("%s\n",temp->filepath);
			}
			temp = temp->next;
		}
	}
	else{
		printf("\nStorage was empty at quit time.\n");
	}
}

// ----------------------------------------------------------------------------------

Storage* newStorage(int maxfiles, int maxsize){

	Storage* storage;
	if((storage = malloc(sizeof(Storage))) == NULL){
		// errno is set to ENOMEM by malloc error
		return NULL;
	}

	// filepath fifo queue initialization
	if((storage->filepaths = malloc(sizeof(FilepathList))) == NULL){
		free(storage);
		return NULL;
	}

	if((storage->filepaths->head = malloc(sizeof(FilepathNode))) == NULL){
		free(storage->filepaths);
		free(storage);
		return NULL;
	}

	if((storage->filepaths->tail = malloc(sizeof(FilepathNode))) == NULL){
		free(storage->filepaths);
		free(storage);
		return NULL;
	}
	storage->filepaths->head = storage->filepaths->tail = NULL;

	// hashtable initialization

	if((storage->hashtable = icl_hash_create(BUCKETS, NULL, NULL)) == NULL){
		free(storage->filepaths->head);
		free(storage->filepaths->tail);
		free(storage->filepaths);
		return NULL;
	}

	// mutex initialization
	PERRNOTZERO(pthread_mutex_init(&(storage->storagemtx), NULL));

	// internal variables set
	storage->maxfiles = maxfiles;
	storage->maxsize = maxsize;

	return storage;
}


int freeStorage(Storage* storage){

	if(storage == NULL){
		errno = EINVAL;
		return -1;
	}

	PERRNEG(freeFilepathList(storage->filepaths));
	PERRNEG(icl_hash_destroy(storage->hashtable, NULL, NULL));
	PERRNEG(pthread_mutex_destroy(&(storage->storagemtx)));
	free(storage);

	return 0;
}