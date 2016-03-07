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
	ViewStmt *view_stmt;
	SelectStmt *select_stmt;
	List *power_set;
	//First we need to test if we create a table
	if(query->utilityStmt->type == T_CreateStmt){
		// Add it to the map
		// We need to go through all the views/tables and see which is included/includes this one
		// Create a function that does that
	} else if(query->utilityStmt->type == T_ViewStmt){
		// Here we want to split up the query in a way that we can reuse the maps directly
		// So we 
		// First we need to cast it to a ViewStmt
		ViewStmt *view_stmt = (ViewStmt *) query->utilityStmt.query;
		// Then we need to test if it is a select statement beneath, if not it is not supported
		if(view_stmt->query->type == T_SelectStmt){
			// Again we need to cast it to a SelectStmt
			select_stmt = (SelectStmt *) view_stmt->query;
			// Now we have access to the different parts of the query

		} else {
			// Maybe we need to print an error here, but for now just do nothing
		}

	}

}

List* get_powerset(List target_list, int i){
	int *bitmask;
	int it, length;
	ListCell *to_delete;
	List *new_list;

	// First the same
	new_list = &target_list;

	// Here we get the bitmask representing the ith powerset
	bitmask = get_bitmask(i);
	length = sizeof(bitmask)/sizeof(bitmask[0]);
	for(it = 0; it < length; it++){
		if(bitmask[it] == 0){
			to_delete = list_nth_cell(new_list, it);
			new_list = list_delete_cell(new_list, to_delete, to_delete->prev);
		}
	}

	// Return
	return new_list;
}

int* get_bitmask(int num){
	int log_mask, k, mask;
	int *bitmask;

	log_mask = (int) log(num);
	bitmask = calloc(sizeof(int) * log_mask);

	while(log_mask--){
		mask = 1 << log_mask;
		mask = mask & num;
		bitmask[log_mask] = mask >> log_mask;
	}

	return bitmask;
}