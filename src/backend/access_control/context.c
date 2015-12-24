#include "access_control/context.h"

/* Start declarations */

ac_context_stack context_stack_init();
void ac_context_push(*ac_context_stack stack, *ac_context context);
ac_context *ac_context_pop(*ac_context_stack);

/* Stack init method */
ac_context_stack context_stack_init(){
	ac_context *array = (ac_context *)calloc(INIT_STACK_SIZE, sizeof(ac_context));
	ac_context_stack stack = {array, NULL, INIT_STACK_SIZE, INIT_STACK_SIZE};
	return stack;
}

/* Stack push method */
void ac_context_push(*ac_context_stack stack, *ac_context context){
	if (ac_context_stack->free_slots == 0){
		/* In this case the stack is full, we need to resize -> double for amortisation/performance */
		stack->array = realloc(stack->array, 2*stack->size*sizeof(ac_context));
		stack->free_slots = stack->size;
		stack->size*=2;
	}
	/* push context */
	stack->array[stack->size - stack->free_slots] = context;
	stack->top = context;
	stack->free_slots--;
}

/* Stack pop method */
ac_context *ac_context_pop(*ac_context_stack stack){
	if(stack->size == stack->free_slots){
		/* In this case the stack is empty and we return NULL */
		return NULL;
	}
	ac_context *popped = stack->top;
	stack->free_slots++;
	stack->array[stack->size - stack->free_slots] = NULL;
	stack->top = stack->array[stack->size - stack->free_slots -1];
	return popped;
}