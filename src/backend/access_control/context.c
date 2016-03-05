#include <stdlib.h>
#include "access_control/context.h"


/* Start declarations */

void ac_context_push(ac_context *context, ac_context_stack *context_stack);
ac_context *ac_context_pop(ac_context_stack *context_stack);
bool perform_mapping(Query query);

/* Stack push method */
void ac_context_push(ac_context *context, ac_context_stack *context_stack){
	if (context_stack->free_slots == 0){
		/* In this case the stack is full, we need to resize -> double for amortisation/performance */
		context_stack->array = realloc(context_stack->array, 2*context_stack->size*sizeof(ac_context*));
		context_stack->free_slots = context_stack->size;
		context_stack->size*=2;
	}
	/* push context */
	context_stack->array[context_stack->size - context_stack->free_slots] = context;
	context_stack->top = context;
	context_stack->free_slots--;
}

/* Stack pop method */
ac_context *ac_context_pop(ac_context_stack *context_stack){
	ac_context *popped;
	if(context_stack->size == context_stack->free_slots){
		/* In this case the stack is empty and we return NULL */
		return NULL;
	}
	popped = context_stack->top;
	context_stack->free_slots++;
	context_stack->array[context_stack->size - context_stack->free_slots] = NULL;
	context_stack->top = context_stack->array[context_stack->size - context_stack->free_slots -1];
	return popped;
}

bool perform_mapping(Query query){
	//First we need to test if we create a table
	if(query->utilityStmt->type == T_CreateStmt){
		// Add it to the map
		// We need to go through all the views/tables and see which is included/includes this one
	} else if(query->utilityStmt->type == T_ViewStmt){
		//Perform splits of the underlying select statement and compute maps
	}

}