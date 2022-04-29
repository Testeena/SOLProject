#include "../includes/configparsing.h"

int putConfig(configList* list, configNode* toput){
	if(list->head == NULL && list->tail == NULL){
		list->head = list->tail = toput;
		return 0;
	}
	else if(list->head != NULL){
		list->tail->next = toput;
		list->tail = toput;
		return 0;
	}
	
	return -1;
}

int parseFile(configList* outcome, char* toparse){
	FILE* file;
	// calloc and fopen both set errno if fail, we just return -1 to the caller
	if((file = fopen(toparse, "r")) == NULL){
		return -1;
	}

	char line[LINELENGHT];
	while(fgets(line, LINELENGHT, file)){

		if(line[0] == '\n' || line[0] == '#') continue;

		configNode* toput;
		if((toput = malloc(sizeof(configNode))) == NULL){
			return -1;
		}
		char* token = strtok(line, "=");
		if((toput->name = malloc(strlen(token) * sizeof(char))) == NULL){
			return -1;
		}
		strncpy(toput->name, token, strlen(token));
		if((token = strtok(NULL, "=")) == NULL){
			free(toput);
			return -1;
		}
		if((toput->value = malloc(strlen(token) * sizeof(char))) == NULL){
			return -1;
		}
		strncpy(toput->value, token, strlen(token));
		toput->value[strcspn(toput->value, "\n")] = '\0';
		putConfig(outcome, toput);
	}
	fclose(file);
	return 0;
}

void printList(configList* list){
	configNode* temp = list->head;
	while(temp != NULL){
		printf("name: %s, value: %s\n", temp->name, temp->value);
		temp = temp->next;
	}
}

int freeConfigList(configList* list){

	while(list->head != NULL){
		configNode* temp = list->head;
		list->head = list->head->next;
		if(temp->name != NULL){
			free(temp->name);
		}
		if(temp->name != NULL){
			free(temp->value);
		}
		if(temp != NULL){
			free(temp);
		}
	}

	if(list != NULL){
		free(list);
	}
	return 1;
}

void getConfigs(configList* parsed, long* maxstorage, long* maxnfiles, long* nworkers, char** sockname, char** logfilename){

	configNode* temp = parsed->head;

	char* end = NULL;
	if((strcmp(temp->name, "MAXSTORAGE") == 0) && strtol(temp->value, &end, 0) > 0){
		*maxstorage = strtol(temp->value, &end, 0); 
	} else{
		*maxstorage = DEFAULT_MAXSTORAGE;
	}

	temp = temp->next;
	if((strcmp(temp->name, "MAXNFILES") == 0) && strtol(temp->value, &end, 0) > 0){
		*maxnfiles = strtol(temp->value, &end, 0);
	} else{
		*maxnfiles = DEFAULT_MAXNFILES;
	}

	temp = temp->next;
	if((strcmp(temp->name, "NWORKERS") == 0) && strtol(temp->value, &end, 0) > 0){
		*nworkers = strtol(temp->value, &end, 0);
	} else{
		*nworkers = DEFAULT_NWORKERS;
	}

	temp = temp->next;
	if((strcmp(temp->name, "SOCKNAME") == 0)){
		if((*sockname = (char*)malloc(strlen(temp->value) * sizeof(char))) == NULL){
			perror("couldnt allocate enough memory");
			return;
		}
		strncpy(*sockname, temp->value, strlen(temp->value));
	} else{
		if((*sockname = (char*)malloc(strlen(DEFAULT_SOCKNAME) * sizeof(char))) == NULL){
			perror("couldnt allocate enough memory");
			return;
		}
		strncpy(*sockname, DEFAULT_SOCKNAME, strlen(DEFAULT_SOCKNAME));
	}

	temp = temp->next;
	if((strcmp(temp->name, "LOGFILENAME") == 0)){
		if((*logfilename = (char*)malloc(strlen(temp->value) * sizeof(char))) == NULL){
			perror("couldnt allocate enough memory");
			return;
		}
		strncpy(*logfilename, temp->value, strlen(temp->value));
	} else{
		if((*logfilename = (char*)malloc(strlen(DEFAULT_LOGFILENAME) * sizeof(char))) == NULL){
			perror("couldnt allocate enough memory");
			return;
		}
		strncpy(*logfilename, DEFAULT_LOGFILENAME, strlen(DEFAULT_LOGFILENAME));
	}
}
