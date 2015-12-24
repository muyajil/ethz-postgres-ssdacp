#ifndef CONTEXT_H
#define CONTEXT_H

#include "postgres_ext.h"

#define INIT_STACK_SIZE 10

/* Data structure representing context */
typedef struct ac_context{
	Oid user;
	Oid invoker;
	char *command;
	Oid trigger;
} ac_context;

/* Context Stack Data Structure */
typedef struct ac_context_stack{
	ac_context **array;
	ac_context *top;
	int size;
	int free_slots;
} ac_context_stack;

/* The global context stack */
extern ac_context_stack context_stack;

/* Stack init method */
extern ac_context_stack context_stack_init();

/* Stack push method */
extern void ac_context_push(*ac_context_stack stack, *ac_context context);

/* Stack pop method */
extern ac_context *ac_context_pop(*ac_context_stack stack);

#endif /* CONTEXT_H */