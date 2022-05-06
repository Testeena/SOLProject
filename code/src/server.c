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
#include <time.h>

#include "../includes/utils.h"
#include "../includes/configparsing.h"
#include "../includes/comms.h"
#include "../includes/storage.h"
#include "../includes/requestqueue.h"

typedef struct workerargs{
	int pipe;
	int tid;
	Storage* storage;
	RequestQueue* requests;
} workerArgs;

volatile sig_atomic_t softexit = 0;
volatile sig_atomic_t hardexit = 0;

FILE* logfile;
pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;

int* requestsdone;

void sigHandler(int sig);
void freeSocket(char* socketname);
void* work(void* args);
void logprint(Request* req, Response* res, int clientfd);

int main(int argc, char *argv[]){
	if (argc != 2){
		fprintf(stderr, RED "Invalid arguments!\n" RESET "Usage: ./server <configfile>\n");
		return EXIT_FAILURE;
	}
	
	long maxstorage, maxnfiles, nworkers;
	char* socketname;
	char* logfilename;

	int connectedclients = 0;
	int maxconnectedclients = 0;

	configList* clist;
	PERRNULL((clist = malloc(sizeof(configList))));
	PERRNEG(parseFile(clist, argv[1]));
	getConfigs(clist, &maxstorage, &maxnfiles, &nworkers, &socketname, &logfilename);
	freeConfigList(clist);
	logfile = fopen(logfilename, "w+");
	PERRNULL(logfile);

	// signals handling :
	// SIGHUP 			 -> softexit
	// SIGINT || SIGQUIT -> hardexit
	struct sigaction signalhandler;
	memset(&signalhandler, 0, sizeof(signalhandler));
	signalhandler.sa_handler = sigHandler;
	// ignoring SIGPIPE
	PERRNEG(sigaction(SIGPIPE, &(struct sigaction) {SIG_IGN}, NULL));
	// creating a new signal mask for the signal types that need a custom handling
	sigset_t mask;
	PERRNEG(sigemptyset(&mask));
	PERRNEG(sigaddset(&mask, SIGINT));
	PERRNEG(sigaddset(&mask, SIGHUP));
	PERRNEG(sigaddset(&mask, SIGQUIT));
	signalhandler.sa_mask = mask;
	PERRNEG(sigaction(SIGINT, &signalhandler, NULL));
	PERRNEG(sigaction(SIGHUP, &signalhandler, NULL));
	PERRNEG(sigaction(SIGQUIT, &signalhandler, NULL));
	
	RequestQueue* requests = newRequestQueue(MAX_REQUESTS);
	PERRNULL(requests);

	Storage* storage = newStorage(maxnfiles, maxstorage);
	PERRNULL(storage);

	int socketfd;
	struct sockaddr_un socketaddress;
	socketaddress.sun_family = AF_UNIX;
	strncpy(socketaddress.sun_path, socketname, strlen(socketname)+1);
	PERRNEG((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)));
	PERRNEG(bind(socketfd, (struct sockaddr*) &socketaddress, sizeof(socketaddress)));
	PERRNEG(listen(socketfd, 1));

	int wpipe[2];
	char pipebuffer[MAXPIPEBUFF];
	PERRNEG(pipe(wpipe));

	int maxfd = 0;
	fd_set readset, readsetcopy;

	//maxfd = socketfd > maxfd ? socketfd : maxfd;
	if(socketfd > maxfd){
		maxfd = socketfd;
	}

	FD_ZERO(&readset);
	FD_ZERO(&readsetcopy);
	FD_SET(socketfd, &readset);
	FD_SET(wpipe[0], &readset);

	pthread_t workers[nworkers];
	workerArgs wargs[nworkers];
	PERRNULL((requestsdone = malloc(nworkers * sizeof(int))));

	for (int i = 0; i < nworkers; i++){
		wargs[i].pipe = wpipe[1];
		wargs[i].storage = storage;
		wargs[i].requests = requests;
		wargs[i].tid = i;
		PERRNOTZERO(pthread_create(&workers[i], NULL, work, (void*)(wargs+i)));
		requestsdone[i] = 0;
	}
	while(!hardexit){
		readsetcopy = readset;
		if(select(maxfd+1, &readsetcopy, NULL, NULL, NULL) == -1){
			if(hardexit || softexit){
				break;
			}
			perror("select error");
			return -1;
		}
		for (int i = 0; i <= maxfd; i++){
			if(FD_ISSET(i, &readsetcopy)){
				if(i == wpipe[0]){

					PERRNEG(read(i, pipebuffer, MAXPIPEBUFF));

					if(strncmp(pipebuffer, "exit", strlen("exit")) == 0){
						break;
					}

					if(atol(pipebuffer) != 0){
						FD_SET(atol(pipebuffer), &readsetcopy);
						if(atol(pipebuffer) > maxfd){
							maxfd = atol(pipebuffer);
						}
					}

					else{
						if(!connectedclients && softexit){
							break;
						}
					}
				}
				else if(i == socketfd){
					int newconn;
					PERRNEG((newconn = accept(socketfd, NULL, NULL)));

					if(softexit){
						PERRNEG(close(newconn));
					}
					else{
						FD_SET(newconn, &readset);
						connectedclients++;
						maxconnectedclients = (connectedclients > maxconnectedclients) ? connectedclients : maxconnectedclients;
						//printf(GREEN "Client[%d] connected.\n" RESET, newconn);

						time_t rawtime;
						struct tm* timestamp;
						time(&rawtime);
						timestamp = localtime(&rawtime);
						fprintf(logfile, "\nClient[%d]\n\tRequest:  CONNECT\n\tResponse: OK\n\tTime:     %s", newconn, asctime(timestamp));

						//maxfd = (maxfd < newconn) ? newconn : maxfd;
						if(newconn > maxfd){
							maxfd = newconn;
						}
					}
				}
				
				else{
					// already connected client case
					FD_CLR(i, &readset);
					if(i == maxfd){
						maxfd--;
					}
					//printf("putting Requestor [%d] into requestqueue\n", i);
					putRequestor(requests, i);
				}
			}
		}

	}

	for (int i = 0; i < nworkers; i++){
		putRequestor(requests, -1);
	}

	for (int i = 0; i < nworkers; i++){
		PERRNOTZERO(pthread_join(workers[i], NULL));
	}

	close(wpipe[0]);
	close(wpipe[1]);
	freeSocket(socketname);
	freeRequestQueue(requests);
	puts("\nStatistics:");
	printf("Evicted files: %d\n", storage->evicted);
	fprintf(logfile, "\n\n---------------------------- END ----------------------------\n\n");
	fprintf(logfile, "Evicted files: %d\n", storage->evicted);
	printf("Maximum number of files reached: %d/%d\n", storage->maxreachedfiles, storage->maxfiles);
	fprintf(logfile, "Maximum number of files reached: %d\n", storage->maxreachedfiles);
	printf("Maximum bytes capacity reached:  %d/%d\n", storage->maxreachedsize, storage->maxsize);
	fprintf(logfile, "Maximum Mb capacity reached:  %d\n", storage->maxreachedsize/1000000);
	puts("Number of requests served by workers:");
	fprintf(logfile, "Number of requests served by workers:\n");
	for (int i = 0; i < nworkers; ++i){
		printf("	Worker n.%d: %d\n", i, requestsdone[i]);
		fprintf(logfile, "\tWorker n.%d: %d\n", i, requestsdone[i]);
	}
	fclose(logfile);
	printFilepaths(storage->filepaths);
	freeStorage(storage);

	return 0;
}

void* work(void* args){
	
	Storage* wstorage = ((workerArgs*)args)->storage;
	RequestQueue* wrequests = ((workerArgs*)args)->requests;
	int wpipe = ((workerArgs*)args)->pipe;
	int wtid = ((workerArgs*)args)->tid;

	while(!hardexit && !softexit){

		int clientfd;
		if((clientfd = getRequestor(wrequests)) == -1){
			continue;
		}
		//printf(CYAN "------- Client[%d] is going to be served by Worker n.%d, pthread_self = %ld -------\n" RESET, clientfd, wtid, (long)pthread_self());

		char pipebuffer[MAXPIPEBUFF];

		Request* req;
		PERRNULL((req = malloc(sizeof(Request))));
		
		PERRNEG(getRequest(clientfd, req));
		requestsdone[wtid]++;

		Response* res;

		PERRNOTZERO(pthread_mutex_lock(&(wstorage->storagemtx)));

		switch(req->code){

			case REQ_OPEN: {

				File* toopen = icl_hash_find(wstorage->hashtable, req->path);

				if(req->flags == O_CREATE || req->flags == O_BOTH){

					// if file is already in the storage
					if(toopen){
						PERRNOTZERO((pthread_mutex_unlock(&(wstorage->storagemtx))));
						PERRNULL((res = newResponse(RES_ALREADYEXISTS, NULL)));
					}

					else{
						File* tocreate;
						PERRNULL((tocreate = newFile(req->path, req->data)));

						File* evicted;
						char* toevictpath;
						PERRNULL((toevictpath = malloc(MAX_PATH * sizeof(char))));
						// checking on capacity misses
						if(wstorage->currfiles+1 > wstorage->maxfiles){
							PERRNEG(popFilepathList(wstorage->filepaths, toevictpath));
							PERRNULL((evicted = icl_hash_find(wstorage->hashtable, toevictpath)));
							FileList* evictedlist;
							PERRNULL((evictedlist = malloc(sizeof(FileList))));
							PERRNEG(addFile(evictedlist, evicted));
							PERRNULL((res = newResponse(RES_OK, evictedlist)));
						}
						// there is space to store the new file
						else{
							PERRNULL((res = newResponse(RES_OK, NULL)));
						}
						if(strlen(toevictpath) != 0){
							PERRNEG(icl_hash_delete(wstorage->hashtable, toevictpath, NULL, NULL));
							free(toevictpath);
							wstorage->evicted++;
							wstorage->currfiles--;
							wstorage->currsize -= evicted->datasize;

							while(evicted->lockwaiters->head != NULL){
								Response* wres;
								PERRNULL((wres = newResponse(RES_NOTFOUND, NULL)));
								snprintf(pipebuffer, MAXPIPEBUFF, "%04d", evicted->lockwaiters->head->fd);
								PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
								FdNode* temp = evicted->lockwaiters->head;
								PERRNEG(sendResponse(temp->fd, wres));
								freeResponse(wres);
								evicted->lockwaiters->head = evicted->lockwaiters->head->next;
								free(temp);
							}

							freeFile(evicted);
						}

						tocreate->creator = clientfd;
						if(req->flags == O_BOTH){
							tocreate->locker = clientfd;
						}
						tocreate->writer = tocreate->readers = 0;
						PERRNULL((tocreate->openers = newFdList()));
						PERRNULL((tocreate->lockwaiters = newFdList()));
						PERRNEG(putFd(tocreate->openers, clientfd));
						PERRNULL(icl_hash_insert(wstorage->hashtable, tocreate->path, tocreate));
						PERRNEG(addFilepath(wstorage->filepaths, tocreate->path));

						wstorage->currfiles++;
						if(wstorage->currfiles > wstorage->maxreachedfiles){
							wstorage->maxreachedfiles = wstorage->currfiles;
						}

						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					}
				}
				else if(req->flags == O_NONE || req->flags == O_LOCK){
					// checking if requested file exists in the storage
					if(!toopen){
						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
						PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
					}
					else{
						PERRNOTZERO(pthread_mutex_lock(&(toopen->ordering)));
						PERRNOTZERO(pthread_mutex_lock(&(toopen->mtx)));
						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
						if(req->flags == O_LOCK){
							if(!toopen->locker){
								toopen->locker = clientfd;
								PERRNEG(putFd(toopen->openers, clientfd));
								PERRNULL((res = newResponse(RES_OK, NULL)));
							}
							else{
								PERRNULL((res = newResponse(RES_DENIED, NULL)));
							}
						}
						if(req->flags == O_NONE){
							if(!toopen->locker || toopen->locker == clientfd){
								PERRNEG(putFd(toopen->openers, clientfd));
								//printFdList(toopen->openers);
								PERRNULL((res = newResponse(RES_OK, NULL)));
							}
							else{
								PERRNULL((res = newResponse(RES_DENIED, NULL)));
							}
						}
						PERRNOTZERO(pthread_mutex_unlock(&(toopen->ordering)));
						PERRNOTZERO(pthread_mutex_unlock(&(toopen->mtx)));
					}
				}
				else{
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_BADREQ, NULL)));
				}
				PERRNEG(sendResponse(clientfd, res));
				logprint(req, res, clientfd);
				//PERRNOTZERO(freeResponse(res));

				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_CLOSE: {
				File* toclose = icl_hash_find(wstorage->hashtable, req->path);
				if(toclose == NULL){
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
				}
				else{
					PERRNOTZERO(pthread_mutex_lock(&(toclose->ordering)));
					PERRNOTZERO(pthread_mutex_lock(&(toclose->mtx)));
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));

					PERRNEG(removeFd(toclose->openers, clientfd));

					if(toclose->readers == 0){
						PERRNOTZERO(pthread_cond_broadcast(&(toclose->writecond)));
					}
					PERRNOTZERO(pthread_mutex_unlock(&(toclose->ordering)));
					PERRNOTZERO(pthread_mutex_unlock(&(toclose->mtx)));
					PERRNULL((res = newResponse(RES_OK, NULL)));
				}
				PERRNEG(sendResponse(clientfd, res));
				//PERRNOTZERO(freeResponse(res));
				logprint(req, res, clientfd);

				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_READ: {

				File* toread = icl_hash_find(wstorage->hashtable, req->path);

				// checking if requested file actually exists in the storage
				if(toread == NULL){
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
				}
				else{
					// if requested file exists in the storage
					PERRNOTZERO(pthread_mutex_lock(&(toread->ordering)));
					PERRNOTZERO(pthread_mutex_lock(&(toread->mtx)));
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));

					// checking if clientfd has the right to read the file
					if(checkFd(toread->openers, clientfd) == -1){
						PERRNOTZERO(pthread_mutex_unlock(&(toread->ordering)));
						PERRNOTZERO(pthread_mutex_unlock(&(toread->mtx)));
						PERRNULL((res = newResponse(RES_DENIED, NULL)));
					}
					else{
						if(toread->locker != 0 && toread->locker != clientfd){
							PERRNOTZERO(pthread_mutex_unlock(&(toread->ordering)));
							PERRNOTZERO(pthread_mutex_unlock(&(toread->mtx)));
							PERRNULL((res = newResponse(RES_DENIED, NULL)));
						}
						else{
							while(toread->writer != 0){
								PERRNOTZERO(pthread_cond_wait(&(toread->writecond), &(toread->mtx)));
							}

							toread->readers++;

							FileList* filestosend;
							PERRNULL((filestosend = malloc(sizeof(FileList))));
							PERRNOTZERO(addFile(filestosend, toread));
							PERRNOTZERO(pthread_mutex_unlock(&(toread->ordering)));
							PERRNOTZERO(pthread_mutex_unlock(&(toread->mtx)));
							PERRNULL((res = newResponse(RES_OK, filestosend)));

							PERRNOTZERO(pthread_mutex_lock(&(toread->mtx)));
							toread->readers--;
							
							if(toread->writer){
								PERRNOTZERO(pthread_cond_signal(&(toread->writecond)));
							}
							PERRNOTZERO(pthread_mutex_unlock(&(toread->mtx)));
						}
					}
				}
				PERRNEG(sendResponse(clientfd, res));
				logprint(req, res, clientfd);
				//PERRNOTZERO(freeResponse(res));

				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_READN: {

				// checking if there are files in the storage
				if(!wstorage->currfiles){
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
				}
				else{
					FilepathNode* currfilepathnode = wstorage->filepaths->head;

					int count;
					FileList* filestosend;
					PERRNULL((filestosend = malloc(sizeof(FileList))));
					while(currfilepathnode != NULL){

						if(req->params > 0 && count == req->params){
							break;
						}

						File* toread = icl_hash_find(wstorage->hashtable, currfilepathnode->filepath);

						// if current file is locked/being written by anyone
						//if((toread->locker != 0 && toread->locker != clientfd) || toread->writer != 0){
						//	continue;
						//}
						//else{
						if(toread != NULL){
							PERRNOTZERO(addFile(filestosend, toread));
						}
						//}
						
						currfilepathnode = currfilepathnode->next;
						count++;
					}
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_OK, filestosend)));
				}
				PERRNEG(sendResponse(clientfd, res));
				logprint(req, res, clientfd);
				//PERRNOTZERO(freeResponse(res));

				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_WRITE: {
				// check if data can fit or not inside the storage
				if(req->datasize > wstorage->maxsize){
					PERRNULL((res = newResponse(RES_NOMEM, NULL)));
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNEG(sendResponse(clientfd, res));
					logprint(req, res, clientfd);
				}
				else{
					File* towrite = icl_hash_find(wstorage->hashtable, req->path);
					// checking if file exists in the storage
					if(towrite == NULL){
						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
						PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
						PERRNEG(sendResponse(clientfd, res));
						logprint(req, res, clientfd);
					}
					else{
						// if the file has just been created
						if(towrite->creator != 0 && towrite->creator == clientfd && checkFd(towrite->openers, clientfd) == 1){

							if(towrite->locker && towrite->locker != clientfd){
								PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
								PERRNULL((res = newResponse(RES_DENIED, NULL)));
								PERRNEG(sendResponse(clientfd, res));
								logprint(req, res, clientfd);
							}
							else{
								File* evicted;
								FileList* evictedlist;
								PERRNULL((evictedlist = malloc(sizeof(FileList))));
								evictedlist->size = 0;

								while(wstorage->currsize + req->datasize > wstorage->maxsize){
									char* toevictpath;
									PERRNULL((toevictpath = malloc(MAX_PATH * sizeof(char))));
									PERRNEG(popFilepathList(wstorage->filepaths, toevictpath));
									PERRNULL((evicted = icl_hash_find(wstorage->hashtable, toevictpath)));

									// need to wait on readers/writers
									PERRNOTZERO(pthread_mutex_lock(&(evicted->ordering)));
									PERRNOTZERO(pthread_mutex_lock(&(evicted->mtx)));
									
									while(evicted->readers > 0 || evicted->writer != 0){
										PERRNOTZERO(pthread_cond_wait(&(evicted->writecond), &(evicted->mtx)));
									}

									PERRNEG(addFile(evictedlist, evicted));
									PERRNEG(icl_hash_delete(wstorage->hashtable, evicted->path, NULL, NULL));
									wstorage->currsize -= evicted->datasize;
									wstorage->currfiles--;
									wstorage->evicted++;
									free(toevictpath);

									while(evicted->lockwaiters->head != NULL){
										Response* wres;
										PERRNULL((wres = newResponse(RES_NOTFOUND, NULL)));
										snprintf(pipebuffer, MAXPIPEBUFF, "%04d", evicted->lockwaiters->head->fd);
										PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
										FdNode* temp = evicted->lockwaiters->head;
										PERRNEG(sendResponse(temp->fd, wres));
										freeResponse(wres);
										evicted->lockwaiters->head = evicted->lockwaiters->head->next;
										free(temp);
									}

									PERRNOTZERO(pthread_mutex_unlock(&(evicted->ordering)));
									PERRNOTZERO(pthread_mutex_unlock(&(evicted->mtx)));
									PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
								}
								PERRNULL((res = newResponse(RES_OK, evictedlist)));

								PERRNOTZERO(pthread_mutex_lock(&(towrite->ordering)));
								PERRNOTZERO(pthread_mutex_lock(&(towrite->mtx)));

								while(towrite->readers > 0){
									PERRNOTZERO(pthread_cond_wait(&(towrite->writecond), &(towrite->mtx)));
								}

								towrite->writer = clientfd;
								towrite->datasize = req->datasize;
								if((towrite->data = malloc(strlen(req->data) * sizeof(char))) == NULL){
									return (void*)-1;
								}
								memcpy(towrite->data, req->data, strlen(req->data) + 1);
								towrite->creator = 0;
								towrite->writer = 0;
								wstorage->currsize += req->datasize;
								if(wstorage->currsize > wstorage->maxreachedsize){
									wstorage->maxreachedsize = wstorage->currsize;
								}
								PERRNOTZERO(pthread_mutex_unlock(&(towrite->ordering)));
								PERRNOTZERO(pthread_mutex_unlock(&(towrite->mtx)));
								PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
								PERRNEG(sendResponse(clientfd, res));
								logprint(req, res, clientfd);
								//PERRNOTZERO(freeResponse(res));
								//PERRNOTZERO(freeFileList(evictedlist));
							}
						}
						else{
							// handling writefile error since client's not the creator of the file and cannot write it (only append data to it)
							PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
							PERRNULL((res = newResponse(RES_DENIED, NULL)));
							PERRNEG(sendResponse(clientfd, res));
							logprint(req, res, clientfd);
							//PERRNOTZERO(freeResponse(res));
						}
					}
				}

				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
				
				break;
			}
			case REQ_APPEND: {

				if(req->datasize > wstorage->maxsize){
					PERRNULL((res = newResponse(RES_NOMEM, NULL)));
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNEG(sendResponse(clientfd, res));
				}
				else{
					File* toappend = icl_hash_find(wstorage->hashtable, req->path);

					if(toappend == NULL){
						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
						PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
						PERRNEG(sendResponse(clientfd, res));
					}
					else{
						printFdList(toappend->openers);
						if(toappend->creator == 0 && checkFd(toappend->openers, clientfd) == 1){

							if(toappend->locker && toappend->locker != clientfd){
								PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
								PERRNULL((res = newResponse(RES_DENIED, NULL)));
								PERRNEG(sendResponse(clientfd, res));
							}
							else{
								File* evicted;
								FileList* evictedlist;

								while(wstorage->currsize + req->datasize > wstorage->maxsize){
									char* toevictpath;
									PERRNULL((toevictpath = malloc(MAX_PATH * sizeof(char))));
									PERRNEG(popFilepathList(wstorage->filepaths, toevictpath));
									PERRNULL((evicted = icl_hash_find(wstorage->hashtable, toevictpath)));

									// need to wait if file is being read/written
									while(evicted->readers > 0 || evicted->writer != 0){
										PERRNOTZERO(pthread_cond_wait(&(evicted->writecond), &(evicted->mtx)));
									}

									PERRNEG(addFile(evictedlist, evicted));
									PERRNEG(icl_hash_delete(wstorage->hashtable, evicted, NULL, NULL));
									wstorage->currsize -= evicted->datasize;
									wstorage->currfiles--;
									wstorage->evicted++;

									while(evicted->lockwaiters->head != NULL){
										Response* wres;
										PERRNULL((wres = newResponse(RES_NOTFOUND, NULL)));
										snprintf(pipebuffer, MAXPIPEBUFF, "%04d", evicted->lockwaiters->head->fd);
										PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
										FdNode* temp = evicted->lockwaiters->head;
										PERRNEG(sendResponse(temp->fd, wres));
										freeResponse(wres);
										evicted->lockwaiters->head = evicted->lockwaiters->head->next;
										free(temp);
									}

									free(toevictpath);
									freeFile(evicted);
								}

								PERRNULL((res = newResponse(RES_OK, evictedlist)));

								PERRNOTZERO(pthread_mutex_lock(&(toappend->ordering)));
								PERRNOTZERO(pthread_mutex_lock(&(toappend->mtx)));

								while(toappend->readers > 0){
									PERRNOTZERO(pthread_cond_wait(&(toappend->writecond), &(toappend->mtx)));
								}

								toappend->writer = clientfd;
								toappend->datasize += req->datasize;
								strcat(toappend->data, req->data);
								wstorage->currsize += req->datasize;
								if(wstorage->currsize > wstorage->maxreachedsize){
									wstorage->maxreachedsize = wstorage->currsize;
								}
								toappend->writer = 0;
								PERRNOTZERO(pthread_mutex_unlock(&(toappend->ordering)));
								PERRNOTZERO(pthread_mutex_unlock(&(toappend->mtx)));
								PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));

								PERRNEG(sendResponse(clientfd, res));
								//PERRNOTZERO(freeResponse(res));
								//PERRNOTZERO(freeFileList(evictedlist));
							}

						}
						else{
							PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
							PERRNULL((res = newResponse(RES_DENIED, NULL)));
							PERRNEG(sendResponse(clientfd, res));
							//PERRNOTZERO(freeResponse(res));
						}

					}
				}

				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_REMOVE: {

				File* toremove = icl_hash_find(wstorage->hashtable, req->path);

				if(toremove == NULL){
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
				}

				else{
					if(toremove->locker != 0 && toremove->locker != clientfd){
						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
						PERRNULL((res = newResponse(RES_DENIED, NULL)));
					}
					else{
						PERRNOTZERO(pthread_mutex_lock(&(toremove->ordering)));
						PERRNOTZERO(pthread_mutex_lock(&(toremove->mtx)));

						while(toremove->readers != 0 || toremove->writer != 0){
							PERRNOTZERO(pthread_cond_wait(&(toremove->writecond), &(toremove->mtx)));
						}

						PERRNEG(deleteFilepathNode(wstorage->filepaths, toremove->path));
						PERRNEG(icl_hash_delete(wstorage->hashtable, toremove->path, NULL, NULL));

						PERRNOTZERO(pthread_mutex_unlock(&(toremove->ordering)));
						PERRNOTZERO(pthread_mutex_unlock(&(toremove->mtx)));
						freeFile(toremove);
						PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
						PERRNULL((res = newResponse(RES_OK, NULL)));
					}
				}
				PERRNEG(sendResponse(clientfd, res));
				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_LOCK: {
				File* tolock = icl_hash_find(wstorage->hashtable, req->path);

				if(tolock == NULL){
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
				}

				else{
					PERRNOTZERO(pthread_mutex_lock(&(tolock->ordering)));
					PERRNOTZERO(pthread_mutex_lock(&(tolock->mtx)));
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));

					while(tolock->readers != 0 || tolock->writer != 0){
						PERRNOTZERO(pthread_cond_wait(&(tolock->writecond), &(tolock->mtx)));
					}

					if(tolock->locker != 0 && tolock->locker != clientfd){
						PERRNEG(putFd(tolock->lockwaiters, clientfd));
						PERRNOTZERO(pthread_mutex_unlock(&(tolock->ordering)));
						PERRNOTZERO(pthread_mutex_unlock(&(tolock->mtx)));
					}
					else{
						tolock->locker = clientfd;
						PERRNOTZERO(pthread_cond_broadcast(&(tolock->writecond)));
						PERRNOTZERO(pthread_mutex_unlock(&(tolock->ordering)));
						PERRNOTZERO(pthread_mutex_unlock(&(tolock->mtx)));
						PERRNULL((res = newResponse(RES_OK, NULL)));
					}
				}

				PERRNEG(sendResponse(clientfd, res));
				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_UNLOCK: {
				File* tounlock = icl_hash_find(wstorage->hashtable, req->path);

				if(tounlock == NULL){
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));
					PERRNULL((res = newResponse(RES_NOTFOUND, NULL)));
				}
				else{
					PERRNOTZERO(pthread_mutex_lock(&(tounlock->ordering)));
					PERRNOTZERO(pthread_mutex_lock(&(tounlock->mtx)));
					PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));

					while(tounlock->readers != 0 || tounlock->writer != 0){
						PERRNOTZERO(pthread_cond_wait(&(tounlock->writecond), &(tounlock->mtx)));
					}

					if(tounlock->locker == clientfd){
						int newlockerfd;
						if((newlockerfd = getFirstFd(tounlock->lockwaiters)) != -1){
							tounlock->locker = newlockerfd;
							PERRNOTZERO(pthread_mutex_unlock(&(tounlock->ordering)));
							PERRNOTZERO(pthread_mutex_unlock(&(tounlock->mtx)));
							Response* wres;
							PERRNULL((wres = newResponse(RES_OK, NULL)));
							PERRNEG(sendResponse(newlockerfd, wres));
							PERRNOTZERO(freeResponse(wres));
							snprintf(pipebuffer, MAXPIPEBUFF, "%04d", newlockerfd);
							PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
						}
						else{
							// if no lock waiters are present, just broadcast that the file is now readable/writable to cond waiters
							PERRNOTZERO(pthread_cond_broadcast(&(tounlock->writecond)));
    						PERRNOTZERO(pthread_mutex_unlock(&(tounlock->ordering)));
    						PERRNOTZERO(pthread_mutex_unlock(&(tounlock->mtx)));
						}
						PERRNULL((res = newResponse(RES_OK, NULL)));
					}
					else{
						PERRNOTZERO(pthread_mutex_unlock(&(tounlock->ordering)));
						PERRNOTZERO(pthread_mutex_unlock(&(tounlock->mtx)));
						PERRNULL((res = newResponse(RES_DENIED, NULL)));
					}
				}
				PERRNEG(sendResponse(clientfd, res));
				// sending clientfd back to main thread by pipe to make it available to be selected
				snprintf(pipebuffer, MAXPIPEBUFF, "%04d", clientfd);
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));

				break;
			}
			case REQ_EXIT: {

				FilepathNode* tocheckpath = wstorage->filepaths->head;
				while(tocheckpath != NULL){

					File* tocheck = icl_hash_find(wstorage->hashtable, tocheckpath->filepath);

					PERRNOTZERO(pthread_mutex_lock(&(tocheck->ordering)));
    				PERRNOTZERO(pthread_mutex_lock(&(tocheck->mtx)));

					while(tocheck->readers != 0 && tocheck->writer != 0){
						PERRNOTZERO(pthread_cond_wait(&(tocheck->writecond), &(tocheck->mtx)));
					}

					if(tocheck->locker == clientfd){
						int newlockerfd;
						if((newlockerfd = getFirstFd(tocheck->lockwaiters)) != -1){
							tocheck->locker = newlockerfd;
							PERRNOTZERO(pthread_mutex_unlock(&(tocheck->ordering)));
							PERRNOTZERO(pthread_mutex_unlock(&(tocheck->mtx)));
							Response* wres;
							PERRNULL((wres = newResponse(RES_OK, NULL)));
							PERRNEG(sendResponse(newlockerfd, wres));
							PERRNOTZERO(freeResponse(wres));
							snprintf(pipebuffer, MAXPIPEBUFF, "%04d", newlockerfd);
							PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
						}
						else{
							// if no lock waiters are present, just broadcast that the file is now readable/writable to cond waiters
							tocheck->locker = 0;
							PERRNOTZERO(pthread_cond_broadcast(&(tocheck->writecond)));
						}
					}

					PERRNEG(removeFd(tocheck->openers, clientfd));
					PERRNEG(removeFd(tocheck->lockwaiters, clientfd));

					PERRNOTZERO(pthread_mutex_unlock(&(tocheck->ordering)));
    				PERRNOTZERO(pthread_mutex_unlock(&(tocheck->mtx)));

					tocheckpath = tocheckpath->next;
				}

				PERRNOTZERO(pthread_mutex_unlock(&(wstorage->storagemtx)));

				PERRNULL((res = newResponse(RES_OK, NULL)));

				// sending exit request via pipe to master server thread
				strncpy(pipebuffer, "exit", strlen("exit"));
				PERRNEG(write(wpipe, pipebuffer, MAXPIPEBUFF));
				PERRNEG(sendResponse(clientfd, res));

				close(clientfd);
				break;
			}
		}
	}
	//printf("\nWorker %d exiting...\n", wtid);

	return NULL;	
}

void logprint(Request* req, Response* res, int clientfd){
	PERRNOTZERO(pthread_mutex_lock(&logmutex));
	//puts("------- log lock acquired -------");
	if(req == NULL || res == NULL){
		perror("wrong logfile arguments");
		return;
	}

	time_t rawtime;
	struct tm* timestamp;
	time(&rawtime);
	timestamp = localtime(&rawtime);

	fprintf(logfile, "\nClient[%d]\n\tWorker: %ld\n\tRequest: %s\n\tResponse: %s\n\tTime: %s", clientfd, (long)pthread_self(), stringifyCode(req->code), stringifyCode(res->code), asctime(timestamp));

	if(req->datasize){
		fprintf(logfile, "\tBytes Written: %d\n", req->datasize);
	}

	if(res->flistsize){
		int bytes = 0;
		FileNode* temp = res->flist->head;
		while(temp != NULL){
			bytes += temp->file->datasize;
			temp = temp->next;
		}
		fprintf(logfile, "\tBytes Read: %d\n", bytes);
	}
	if(res->flistsize != 0){
		if(req->code == REQ_OPEN || req->code == REQ_WRITE){
			fprintf(logfile, "\tEvictedFiles(%d):\n", res->flistsize);
		}
		else if (req->code == REQ_READN){
			fprintf(logfile, "\tFiles read(%d):\n", res->flistsize);
		}
		FileNode* temp = res->flist->head;
		while(temp != NULL){
			fprintf(logfile, "\t\t%s\n", temp->file->path);
			temp = temp->next;
		}
	}
	//puts("--- log lock getting released ---");
	PERRNOTZERO(pthread_mutex_unlock(&logmutex));
}

void sigHandler(int sig) {
    if (sig == SIGHUP) {
        softexit = 1;
    }
    else if (sig == SIGINT || sig == SIGQUIT){
        hardexit = 1;
    }
}

void freeSocket(char* socketname){
	unlink(socketname);
	free(socketname);
}