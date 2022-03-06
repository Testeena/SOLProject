#include "../includes/optparsing.h"

int putOpt(optList* list, char opt, char* args){

	// node initialization
	optNode* toput;
	if((toput = malloc(sizeof(optNode))) == NULL){
		return -1;
	}

	toput->opt = opt;
	
	if((toput->arg = malloc(strlen(args) * sizeof(char))) == NULL){
		return -1;
	}

	strncpy(toput->arg, args, strlen(args));

	// actual put
	if(list->head == NULL && list->tail == NULL){
		list->head = list->tail = toput;
		return 0;
	}
	else if(list->tail != NULL){
		list->tail->next = toput;
		list->tail = toput;
		return 0;
	}
	return -1;
}

optNode* getFirstOpt(optList* list){
	optNode* toreturn;
	if(list->head == NULL){
		toreturn = NULL;
		return toreturn;
	}
	else{
		toreturn = list->head;
		list->head = list->head->next;
		return toreturn;
	}
}

void printOpts(optList* list){
	optNode* temp = list->head;
	while(temp != NULL){
		printf("opt: %c, args: %s\n", temp->opt, temp->arg);
		temp = temp->next;
	}
}

optList* newList(){
	optList* list;
	if((list = malloc(sizeof(optList))) == NULL){
		return NULL;
	}
	else{
		list->head = list->tail = NULL;
		return list;
	}
}

void freeOpt(optNode* node){
	if(node->arg != NULL){
		printf("node->arg = %s\n", node->arg);
		free(node->arg);
	}
	free(node);
}

void freeList(optList* list){
	while(list->head != NULL){
		optNode* temp = list->head;
		list->head = list->head->next;
		freeOpt(temp);
	}
}


int main(int argc, char* argv[]){
	if(argc < 2){
		puts("No options were given, use -h to get help message.");
		return -1;
	}

	optList* list;
	if((list = newList()) == NULL){
		return -1;
	}

	int opt;
	while((opt = getopt(argc, argv, ":hf:w:W:D:r:R:d:t:l:u:c:p")) != -1){
		switch(opt){
			case 'h':
				puts("help message:");
				break;
			case 'f':
				printf("socket = %s\n", optarg);
				break;
			case 'w':
			case 'W':
			case 'D':
			case 'r':
			case 'R':
			case 'd':
			case 't':
			case 'l':
			case 'u':
			case 'c':
				if(putOpt(list, opt, optarg) == -1){
					puts("putopt fail");
				}
				else{
					printf("putopt of %s done.\n", optarg);
				}
				break;
			case 'p':
				printf("verbose set.");
				break;
			default:
				puts("An unknown command was given.");
				puts("help message:");
		}
	}

	printOpts(list);
	freeList(list);
	return 0;
}





















