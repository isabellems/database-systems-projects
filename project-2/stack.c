#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

Stack* Stack_Create(){
	Stack *st = malloc(sizeof(Stack));
	st->count = 0;
	st->front = NULL;
	st->rear = NULL;
	return st;
}

void Stack_push(int id,Stack *s){
	Node *add = malloc(sizeof(Node));
	add->id = id;
	if(s->front==NULL){
		s->front = add;
		s->rear = (s->front);
	}
	else{
		(s->rear)->next = add;
		add->previous = (s->rear);
		add->next = NULL;
		s->rear = (add);
	}
	s->count++;
}

int Stack_pop(Stack *s){
	if(s->front == NULL)
		return -1; //Error : empty stack
	Node *pop = malloc(sizeof(Node));
	int id;
	pop = s->rear;
	s->count--;
	if((s->front)->next != NULL){
		s->rear = (s->rear)->previous;
	}
	else{
		s->front = s->rear = NULL;
	}
	id = pop->id;
	free(pop);
	return id;
}

void Stack_Destroy(Stack *s){
	int i = s->count;
	int j;
	for(j=1; j<=i; j++)
		Stack_pop(s);
	free(s);
}