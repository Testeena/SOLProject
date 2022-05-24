#include "../includes/optparsing.h"

int putOpt(OptList* list, char opt, char* args){

	// node initialization
	OptNode* toput;
	if((toput = malloc(sizeof(OptNode))) == NULL){
		return -1;
	}

	toput->opt = opt;
	toput->next = NULL;
	
	if((toput->arg = calloc(strlen(args)+1 * sizeof(char), 1)) == NULL){
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
	if(list->head == NULL){
		return NULL;
	}
	else{
		OptNode* toreturn;
		toreturn = list->head;
		list->head = list->head->next;
		return toreturn;
	}
}

OptNode* getdnode(OptList* list){

	if(list == NULL || list->head == NULL){
		return NULL;
	}
	else{
		OptNode* toreturn;
		OptNode* temp = list->head;
		if(temp->opt == 'd'){
			toreturn = temp;
			list->head = list->head->next;
			return toreturn;
		}

		while(temp->next != NULL){
			if(temp->next->opt == 'd'){
				toreturn = temp->next;
				temp->next = temp->next->next;
				return toreturn;
			}
			if(temp->next == NULL){
				break;
			}
			temp = temp->next;
		}
		
		return NULL;
	}
} 

OptNode* getDnode(OptList* list){
	if(list == NULL || list->head == NULL){
		return NULL;
	}
	else{
		OptNode* toreturn;
		OptNode* temp = list->head;
		if(temp->opt == 'D'){
			toreturn = temp;
			list->head = list->head->next;
			return toreturn;
		}

		while(temp->next != NULL){
			if(temp->next->opt == 'D'){
				toreturn = temp->next;
				temp->next = temp->next->next;
				return toreturn;
			}
			if(temp->next == NULL){
				break;
			}
			temp = temp->next;
		}

		return NULL;
	}
}

void printOpts(OptList* list){
	OptNode* temp = list->head;
	while(temp != NULL){
		printf("%c:%s -> ", temp->opt, temp->arg);
		temp = temp->next;
	}
	puts("NULL");
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
	free(node->arg);
	free(node);
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

