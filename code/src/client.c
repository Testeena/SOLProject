#include "../includes/utils.h"
#include "../includes/optparsing.h"
#include "../includes/api.h"
#include "../includes/comms.h"

#define RETRY_TIME 5
#define DEFAULT_SOCKNAME "solsock.sk"

bool verbose = false;

void help(void);
int makeApiCall(OptNode* option, char* evictedDir, char* saveDir);
int recDirWrite(char* dirpath, int n, char* evictedDir);

int main(int argc, char *argv[]){
	if(argc < 2){
		puts("No options were given, use -h to get help message.");
		return -1;
	}

	OptList* optlist;
	PERRNULL((optlist = newOptList()));
	
	long sleeptime = 0;

	bool alreadyhelped = false;
	bool alreadyf = false;
	bool lookforD = false;
	bool lookford = false;
	bool dfound = false;
	bool Dfound = false;

	int opt;
	while((opt = getopt(argc, argv, ":hf:w:W:D:r:R:d:t:l:u:c:p")) != -1){
		switch(opt){
		case 'h':
			help();
			alreadyhelped = true;
			exit(EXIT_FAILURE);
			break;
		case 'f':
			if(!alreadyf){
				if((socketname = malloc(strlen(optarg) * sizeof(char))) == NULL){
					return -1;
				}
				strncpy(socketname, optarg, strlen(optarg));
				alreadyf = true;
			}
			break;
		case 'w':
			if(putOpt(optlist, opt, optarg) == -1){
				perror("A command line parsing error occurred during '-w'");
			}
			lookforD = true;
			break;
		case 'W':
			if(putOpt(optlist, opt, optarg) == -1){
				perror("A command line parsing error occurred during '-W'");
			}
			lookforD = true;
			break;
		case 'r':
			if(putOpt(optlist, opt, optarg) == -1){
				perror("A command line parsing error occurred during '-r'");
			}
			lookford = true;
			break;
		case 'R':
			if(putOpt(optlist, opt, optarg) == -1){
				perror("A command line parsing error occurred during '-R'");
			}
			lookford = true;
			break;
		case 'd':
			if(!lookford){
				fprintf(stderr, "Cannot use -d before using -r or -R\n");
			}
			else{
				if(dfound){
					fprintf(stderr, "Cannot use -d multiple times\n");
					break;
				}
				if(putOpt(optlist, opt, optarg) == -1){
					perror("A command line parsing error occurred during '-d'");
				}
				dfound = true;
			}
			break;
		case 'D':
			if(!lookforD){
				fprintf(stderr, "Cannot use -D before using -w or -W\n");
			}
			else{
				if(Dfound){
					fprintf(stderr, "Cannot use -D multiple times.\n");
					break;
				}
				if(putOpt(optlist, opt, optarg) == -1){
					perror("A command line parsing error occurred during '-D'");
				}
				Dfound = true;
			}
			break;
		case 't':
			if(atol(optarg) > 0){
				sleeptime = atol(optarg)/1000;
			}
			else{
				fprintf(stderr, "Sleeptime must be a positive integer.\n");
			}
			break;
		case 'l':
		case 'u':
		case 'c':
			if(putOpt(optlist, opt, optarg) == -1){
				perror("A command line parsing error occurred");
			}
			break;
		case 'p':
			verbose = true;
			break;
		default:
			if(optopt == 'D'){
				fprintf(stderr, "Cannot use -D before using -w or -W\n");
			}
			else if(optopt == 'd'){
				fprintf(stderr, "Cannot use -d before using -r or -R\n");
			}
			else{
				printf(RED "Command '-%c' isn't supported.\n\n" RESET, optopt);
			}
			help();
			exit(EXIT_FAILURE);
		}
	}

	if(!socketname){
		socketname = DEFAULT_SOCKNAME;
	}
	
	struct timespec abstime;
	abstime.tv_nsec = 0;
	abstime.tv_sec = RETRY_TIME;

	if((socketfd = openConnection(socketname, 1000, abstime)) == -1){
		perror("Unable to connect to requested socket");
		freeOptList(optlist);
		return -1;
	}
	/*
	else{
		if(verbose){
			puts(BLUE "Connection estabilished." RESET);
		}
	}
	*/
	OptNode* toget = getFirstOpt(optlist);
	while(toget != NULL){
		usleep(sleeptime);

		if(toget->opt == 'w' || toget->opt == 'W'){
			OptNode* Dtemp = getDnode(optlist);

			//maybe tokenize && make safe 'arg' before passing it (?)
			if(Dtemp != NULL){
				if(makeApiCall(toget, Dtemp->arg, NULL) == -1){
					fprintf(stderr, "makeApiCall error\n");
				}
			}
			else{
				if(makeApiCall(toget, NULL, NULL) == -1){
					fprintf(stderr, "makeApiCall error\n");
				}
			}
			free(Dtemp);
		}

		else if(toget->opt == 'r' || toget->opt == 'R'){
			OptNode* dtemp;
			if((dtemp = malloc(sizeof(OptNode))) == NULL){
				return -1;
			}
			dtemp = getdnode(optlist);
			//maybe tokenize && make safe 'arg' before passing it (?)
			if(dtemp != NULL){
				if(makeApiCall(toget, NULL, dtemp->arg) == -1){
					fprintf(stderr, "makeApiCall error\n");
				}
			}
			else{
				if(makeApiCall(toget, NULL, NULL) == -1){
					fprintf(stderr, "makeApiCall error\n");
				}
			}
			free(dtemp);
		}

		else{
			if(makeApiCall(toget, NULL, NULL) == -1){
				fprintf(stderr, "makeApiCall error\n");
			}
		}
		toget = getFirstOpt(optlist);
	}

	if(closeConnection(socketname) == -1){
		perror("closeConnection error");
	}
	/*
	else{
		if(verbose){
			puts(BLUE "Connection closed." RESET);
		}
	}
	*/
	/*
	freeOptNode(toget);
	puts("HELLO");
	freeOptList(optlist);
	puts("HIIIIIIII");
	*/
	return 0;
}

int makeApiCall(OptNode* option, char* evictedDir, char* saveDir){

	if(option == NULL){
		errno = EINVAL;
		return -1;
	}

	switch(option->opt){
		case 'w':
			;
			// recursively opening passed directory, writing n (or all) files found
			char* absdirpathw;
			if((absdirpathw = malloc(PATH_MAX * sizeof(char))) == NULL){
				perror("dirpath malloc error");
				return -1;
			}
			absdirpathw = realpath(strtok(option->arg, ","), NULL);
			int n = 0;
			char* ntokenw = strtok(NULL, ",");
			// checking if passed 'n' is > 0, if not just keep the n variable on its initialization value 0
			if(ntokenw != NULL && atoi(ntokenw) > 0){
				n = atoi(ntokenw);
			}
			recDirWrite(absdirpathw, n, evictedDir);
			break;
		case 'W':
			;
			// looping through tokenized arguments list, doing a writefile for each file argument
			char* filetokenW = strtok(option->arg, ",");
			while(filetokenW != NULL){
				char* fileabspathW;
				if((fileabspathW = malloc(PATH_MAX * sizeof(char))) == NULL){
					perror("filepath malloc error");
					return -1;
				}
				fileabspathW = realpath(filetokenW, NULL);

				if(openFile(fileabspathW, O_CREATE) == 0){
					if(writeFile(fileabspathW, evictedDir) == -1){
						perror("writeFile error");
						return -1;
					}
					
					if(closeFile(fileabspathW) == -1){
						perror("closeFile error");
						return -1;
					}
				}

				else if(openFile(fileabspathW, O_NONE) == 0){

					FILE* file;
					if((file = fopen(fileabspathW, "r")) == NULL){ 
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
				    if((data = malloc(datalen * sizeof(char))) == NULL){
				    	return -1;
				    }

				    if((fread(data, sizeof(char), datalen, file)) != datalen){
				    	return -1;
				    }
				    fclose(file);

				    if(appendToFile(fileabspathW, data, (size_t)datalen, evictedDir) == -1){
				    	perror("appendToFile error");
				    	return -1;
				    }

				    if(closeFile(fileabspathW) == -1){
						perror("closeFile error");
						return -1;
					}

					//puts("going to free data");
				    //free(data);
				}
				else{
					perror("openFile error");
					return -1;
				}
				filetokenW = strtok(NULL, ",");
			}
			break;
		case 'r':
			;
			char* filetokenr = strtok(option->arg, ",");
			while(filetokenr != NULL){
				char* fileabspathr;
				if((fileabspathr = malloc(PATH_MAX * sizeof(char))) == NULL){
					perror("filepath malloc error");
					return -1;
				}
				fileabspathr = realpath(filetokenr, NULL);

				if(openFile(fileabspathr, O_NONE) == -1){
					perror("openFile error");
					return -1;
				}

				void* buf;
				size_t bufsize;

				if(readFile(fileabspathr, &buf, &bufsize) == 0){
					if(opendir(saveDir)){
						char* newfilepath;
						if((newfilepath = calloc(MAX_PATH * sizeof(char), 0)) == NULL){
							return -1;
						}
						strncpy(newfilepath, saveDir, strlen(saveDir));
						if(newfilepath[MAX_PATH-1] != '/'){
							strcat(newfilepath, "/");
						}
						strcat(newfilepath, basename(fileabspathr));
						FILE* file;
						if((file = fopen(newfilepath, "w+")) == NULL){
							perror("fopen error");
							return -1;
						}
						fwrite((char*)buf, sizeof(char), bufsize, file);
						if(verbose){
							printf(GREEN "File %s saved in %s.\n" RESET, fileabspathr, saveDir);
						}
						fclose(file);
						free(buf);
						free(newfilepath);
					}
					else{
						if(verbose){
							printf("File: %s\nContent: %s\n", fileabspathr, (char*)buf);
						}
					}
				}
				else{
					perror("readFile error");
					return -1;
				}

				if(closeFile(fileabspathr) == -1){
					perror("closeFile error");
					return -1;
				}
				filetokenr = strtok(NULL, ",");
			}
			break;
		case 'R':
			;
			char* absdirpathR;
			if((absdirpathR = malloc(PATH_MAX * sizeof(char))) == NULL){
				perror("filepath malloc error");
				return -1;
			}
			absdirpathR = realpath(strtok(option->arg, ","), NULL);
			int nR = 0;
			char* nRtoken = strtok(NULL, " ");
			if(nRtoken != NULL && atoi(nRtoken) > 0){
				nR = atoi(nRtoken);
			}

			if(readNFiles(nR, saveDir) == -1){
				perror("readNFiles error");
				return -1;
			}

			break;
		case 'l':
			;
			char* filetokenl = strtok(option->arg, ",");
			while(filetokenl != NULL){
				char* fileabspathl;
				if((fileabspathl = malloc(PATH_MAX * sizeof(char))) == NULL){
					perror("filepath malloc error");
					return -1;
				}
				fileabspathl = realpath(filetokenl, NULL);

				if(openFile(fileabspathl, O_NONE) == -1){
					perror("openFile error");
					return -1;
				}

				if(lockFile(fileabspathl) == -1){
					perror("lockFile error");
					return -1;
				}

				if(closeFile(fileabspathl) == -1){
					perror("closeFile error");
					return -1;
				}
				filetokenl = strtok(NULL, ",");
			}
			break;
		case 'u':
			;
			char* filetokenu = strtok(option->arg, ",");
			while(filetokenu != NULL){
				char* fileabspathu;
				if((fileabspathu = malloc(PATH_MAX * sizeof(char))) == NULL){
					perror("filepath malloc error");
					return -1;
				}
				fileabspathu = realpath(filetokenu, NULL);

				if(openFile(fileabspathu, O_NONE) == -1){
					perror("openFile error");
					return -1;
				}

				if(unlockFile(fileabspathu) == -1){
					perror("lockFile error");
					return -1;
				}

				if(closeFile(fileabspathu) == -1){
					perror("closeFile error");
					return -1;
				}
				filetokenu = strtok(NULL, ",");
			}
			break;
		case 'c':
			;
			char* filetokenc = strtok(option->arg, ",");
			while(filetokenc != NULL){
				char* fileabspathc;
				if((fileabspathc = malloc(PATH_MAX * sizeof(char))) == NULL){
					perror("filepath malloc error");
					return -1;
				}
				fileabspathc = realpath(filetokenc, NULL);

				if(openFile(fileabspathc, O_NONE) == -1){
					perror("openFile error");
					return -1;
				}

				if(removeFile(fileabspathc) == -1){
					perror("lockFile error");
					return -1;
				}

				filetokenc = strtok(NULL, ",");
			}
			break;
		default:
			fprintf(stderr, "Unknown option.\n");
			return -1;
	}
	return 0;
}

// recursively opens 'dir' and his subdirectories, sending n writefiles requests to the server, one for each found file

int recDirWrite(char* dirpath, int n, char* evictedDir){
	
	struct dirent* direntry;
	int fcount = 0;

	DIR* dir;
	if((dir = opendir(dirpath)) == NULL){
		return -1;
	}

	// looping through files contained in 'dir'
	while((direntry = readdir(dir)) != NULL){

		if(n != 0 && fcount == n){
			break;
		}

		if(strcmp(direntry->d_name, ".") == 0 || strcmp(direntry->d_name, "..") == 0) {
			continue;
		}

		struct stat filestats;

		char temppath[MAX_PATH];
		if(dirpath[strlen(dirpath)-1] == '/'){
			snprintf(temppath, sizeof(temppath), "%s%s", dirpath, direntry->d_name);
		}
		else{
			snprintf(temppath, sizeof(temppath), "%s/%s", dirpath, direntry->d_name);
		}

		if(stat(temppath, &filestats) == 0){
			if(!S_ISDIR(filestats.st_mode)){
				// found file is not a directory: actual write request need to be done here
				char* fileabspath;
				if((fileabspath = malloc(PATH_MAX * sizeof(char))) == NULL){
					perror("filepath malloc error");
					return -1;
				}
				fileabspath = realpath(temppath, NULL);
				if(openFile(fileabspath, O_CREATE) == 0){
					if(writeFile(fileabspath, evictedDir) == -1){
						perror("writeFile error");
					}
				}

				else if(openFile(fileabspath, O_NONE) == 0){

					FILE* file;
					if((file = fopen(fileabspath, "r")) == NULL){ 
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
				    if((data = malloc(datalen * sizeof(char))) == NULL){
				    	return -1;
				    }

				    if((fread(data, sizeof(char), datalen, file) != datalen)){
				    	free(data);
				    	return -1;
				    }
				    fclose(file);

				    if(appendToFile(fileabspath, data, (size_t)datalen, evictedDir) == -1){
				    	perror("appendToFile error");
				    }
				    free(data);
				}

				if(closeFile(fileabspath) == -1){
					perror("closeFile error");
				}

				fcount++;
			}
			else{
				// maybe check on opendir call return before passing it
				recDirWrite(temppath, n, evictedDir);

			}
		}
	}
	return 0;
}

void help(void){
	printf("List of valid options:\n"
		"-h                            prints help message\n"\
		"-f <filename>                 sets socket name\n"\
		"-w <dirname>[,n=0]            sends n files stored 'dirname', if n=0, sends whole dirname content\n"\
		"-W <file1>[,<file2>]          sends files list\n"\
		"-D <dirname>                  sets 'dirname' as the directory to store evicted files\n"\
		"-r <file1>[,<file2>]          sends read request for the file list passed\n"\
		"-R [n=0]                      sends read request for 'n' files, if n=0, asks for whole storage content\n"\
		"-d <dirname>                  sets 'dirname' as the directory to store read files\n"\
		"-t time                       sets time between requests\n"\
		"-l <file1>[,file2]            sends lock request for the file list passed\n"\
		"-u <file1>[,file2]            sends unlock request for the file list passed\n"\
		"-c <file1>[,file2]            sends delete request for the file list passed\n"\
		"-p                            enables verbose prints for operations and errors\n");
}
















