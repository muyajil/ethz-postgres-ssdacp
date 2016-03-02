#include <stdlib>
#include "access_control/context.h"

/* Start declarations */
/*
void ac_context_push(ac_context *context);
ac_context *ac_context_pop();*/

/* Stack push method */
void ac_context_push(ac_context *context){
	if (context_stack.free_slots == 0){
		/* In this case the stack is full, we need to resize -> double for amortisation/performance */
		context_stack.array = realloc(context_stack.array, 2*context_stack.size*sizeof(ac_context));
		context_stack.free_slots = context_stack.size;
		context_stack.size*=2;
	}
	/* push context */
	context_stack.array[context_stack.size - context_stack.free_slots] = context;
	context_stack.top = context;
	context_stack.free_slots--;
}

/* Stack pop method */
ac_context *ac_context_pop(){
	ac_context *popped;
	if(context_stack.size == context_stack.free_slots){
		/* In this case the stack is empty and we return NULL */
		return NULL;
	}
	popped = context_stack.top;
	context_stack.free_slots++;
	context_stack.array[context_stack.size - context_stack.free_slots] = NULL;
	context_stack.top = context_stack.array[context_stack.size - context_stack.free_slots -1];
	return popped;
}