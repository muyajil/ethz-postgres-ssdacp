#ifndef CONTEXT_H
#define CONTEXT_H

#include "postgres_ext.h"
#include "executor/execdesc.h"

#define INIT_STACK_SIZE 10

/* Data structure representing context */
typedef struct ac_context{
	Oid user;
	Oid invoker;
	QueryDesc query_desc;
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

/* Stack push method */
extern void ac_context_push(ac_context *context);

/* Stack pop method */
extern ac_context *ac_context_pop();

#endif /* CONTEXT_H */