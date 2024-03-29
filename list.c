//
// Created by daniel on 2020-03-17.
//

#include "list.h"

struct List* createList(){
	struct List* list = (struct List*)malloc(sizeof(struct List));
	list->capacity = 32;
	list->len = 0;
	list->array = (int*)malloc(list->capacity * sizeof(int));
	return list;
}

struct List* cloneList(struct List* original){
	struct List* list = (struct List*)malloc(sizeof(struct List));
	list->capacity = original->capacity;
	list->len = original->len;
	list->array = (int*)malloc(list->capacity * sizeof(int));
	for (int i = 0; i < original->len; ++i) {
		list->array[i] = original->array[i];
	}
	return list;
}

void append(struct List* list, int value){
	if (list->len == list->capacity){
		list->capacity *= 2;
		list->array = realloc(list->array,list->capacity * sizeof(int));
	}
	list->array[list->len++] = value;
}

int getValueAtIndex(struct List* list,int index){
	if (index >= list->len || index < 0){
		return INT_MIN;
	}
	return list->array[index];
}

int setValueAtIndex(struct List* list,int index, int value){
	if (index >= list->len || index < 0){
		return INT_MIN;
	}
	list->array[index] = value;
	return 0;
}

int removeAtIndex(struct List* list,int index){
	if (index >= list->len || index < 0){
		return INT_MIN;
	}
	for (int i = index; i < list->len - 1; ++i) {
		list->array[i] = list->array[i + 1];
	}
	list->len--;
	return 0;
}

int addAtIndex(struct List* list, int index, int value){
	if (index >= list->len || index < 0){
		return INT_MIN;
	}
	if (list->len == list->capacity){
		list->capacity *= 2;
		list->array = realloc(list->array,list->capacity * sizeof(int));
	}
	for (int i = list->len + 1; i > index; --i) {
		list->array[i] = list->array[i - 1];
	}
	list->array[index] = value;
	list->len++;
	return 0;
}