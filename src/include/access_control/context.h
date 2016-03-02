#ifndef CONTEXT_H
#define CONTEXT_H

#include "postgres_ext.h"
#include "nodes/parsenodes.h"
#include "executor/execdesc.h"

#define INIT_STACK_SIZE 10

/* Data structure representing context */
typedef struct ac_context_struct{
	Oid user;
	Oid invoker;
	Query *query;
} ac_context;

/* Context Stack Data Structure */
typedef struct ac_context_stack_struct{
	ac_context **array;
	ac_context *top;
	int size;
	int free_slots;
} ac_context_stack;

/* global 2d - map: view -> included in */

/* global 2d - map: view -> includes */

/* save maps command */

/* load maps command */

/* The global context stack */
extern ac_context_stack context_stack;

/* Stack push method */
extern void ac_context_push(ac_context *context);

/* Stack pop method */
extern ac_context *ac_context_pop();

#endif /* CONTEXT_H */