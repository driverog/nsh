//
// Created by daniel on 2020-03-17.
//

#ifndef NSH_LIST_H
#define NSH_LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <values.h>

typedef struct List{
	int len;
	unsigned capacity;
	int* array;
}List;

struct List* createList();

struct List* cloneList(struct List* original);

void append(struct List* list, int value);

int getValueAtIndex(struct List* list,int index);

int setValueAtIndex(struct List* list,int index, int value);

int removeAtIndex(struct List* list,int index);

int addAtIndex(struct List* list, int index, int value);

#endif //NSH_LIST_H
