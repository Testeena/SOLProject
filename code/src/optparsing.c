#include "../includes/optparsing.h"

int putOpt(OptList* list, char opt, char* args){

	// node initialization
	OptNode* toput;
	if((toput = malloc(sizeof(OptNode))) == NULL){
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
	}
	else if(list->tail != NULL){
		list->tail->next = toput;
		list->tail = toput;
	}
	return 0;
}

OptNode* getFirstOpt(OptList* list){
	OptNode* toreturn;
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

void printOpts(OptList* list){
	OptNode* temp = list->head;
	while(temp != NULL){
		printf("opt: %c, args: %s\n", temp->opt, temp->arg);
		temp = temp->next;
	}
}

OptList* newOptList(void){
	OptList* list;
	if((list = malloc(sizeof(OptList))) == NULL){
		return NULL;
	}
	else{
		list->head = list->tail = NULL;
		return list;
	}
}

void freeOptNode(OptNode* node){
	if(node->arg != NULL){
		free(node->arg);
	}
	if(node != NULL){
		free(node);
	}
}

void freeOptList(OptList* list){
	if(list == NULL){
		return;
	}
	else{
		while(list->head != NULL){
			OptNode* temp = list->head;
			list->head = list->head->next;
			freeOptNode(temp);
		}
		free(list);
	}
}