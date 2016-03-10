#ifndef CONTEXT_H
#define CONTEXT_H

#include "c.h"
#include "postgres.h"
#include "postgres_ext.h"
#include "nodes/parsenodes.h"
#include "nodes/primnodes.h"
#include "tcop/tcopprot.h"
#include <math.h>

#define INIT_STACK_SIZE 10

/* Data structure representing context */
struct ac_context_struct{
	Oid user;
	Oid invoker;
	Query query;
	const char *query_string;
};

typedef struct ac_context_struct ac_context;

/* Context Stack Data Structure */
struct ac_context_stack_struct{
	ac_context **array;
	ac_context *top;
	int size;
	int free_slots;
};

typedef struct ac_context_stack_struct ac_context_stack;

/* global 2d - map: view -> included in */

/* global 2d - map: view -> includes */

/* global 1d - map: all created tables and views (even helper views) */

/* save maps function */

/* load maps function */

/* perform mapping function */
extern bool perform_mapping();

/* Stack push method */
//extern void ac_context_push(ac_context *context, ac_context_stack *context_stack);
extern void ac_context_push(ac_context *context);

/* global context stack */
extern ac_context_stack context_stack;

/* Stack pop method */
extern ac_context *ac_context_pop();

/* bool telling if frontend is connected */
extern bool frontend_connected;

#endif /* CONTEXT_H */