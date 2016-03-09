#include <stdlib.h>
#include "access_control/context.h"


/* Start declarations */

void ac_context_push(ac_context *context);
ac_context *ac_context_pop();
bool perform_mapping(void);
List* get_powerset(List target_list, int i);
bool* get_bitmask(int num, int length);

/* Stack push method */
void ac_context_push(ac_context *context){
	if (context_stack.free_slots == 0){
		/* In this case the stack is full, we need to resize -> double for amortisation/performance */
		context_stack.array = realloc(context_stack.array, 2*context_stack.size*sizeof(ac_context*));
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

bool perform_mapping(void){
	ViewStmt *view_stmt;
	CreateStmt *create_stmt;
	SelectStmt *select_stmt;
	List *next_list;
	List *target_list;
	Node *where_clause_node;
	char *relation_name;
	char *select_query_string;
	BoolExpr *where_clause;
	Query query;
	int num_sets;
	List *parsetree_list;

	// Get the query from the stack (we get a copy, so we can modify it as we wish)
	query = context_stack.top->query;
	//First we need to test if we create a table
	if(query.utilityStmt->type == T_CreateStmt){
		/* Add it to the map
		 * We need to go through all the views/tables and see which is included/includes this one
		 * Create a SELECT statement that selects everything from this table
		 * This would be better after the CreateStatement was executed
		 * Probably we should call perform_mapping just before popping from the contex stack
		 */

		 // First we want to get the name of the relation that was created
		 create_stmt = (CreateStmt *) query.utilityStmt;
		 relation_name = create_stmt->relation->relname;

		 // Next we will construct a string representing a SELECT that collects all data from the new table
		 select_query_string = malloc(strlen(relation_name)+14);
		 strcpy(select_query_string, "SELECT * FROM");
		 strcat(select_query_string, relation_name);
		 strcat(select_query_string, ";");
		 free(relation_name);

		 //Now we will parse this
		 parsetree_list = pg_parse_query(select_query_string);

	} else if(query.utilityStmt->type == T_ViewStmt){
		/* Here we want to split up the query in a way that we can reuse the maps directly
		 * So we 
		 * First we need to cast it to a ViewStmt
		 */
		view_stmt = (ViewStmt *) query.utilityStmt;
		// Then we need to test if it is a select statement beneath, if not it is not supported
		if(view_stmt->query->type == T_SelectStmt){
			// Again we need to cast it to a SelectStmt
			select_stmt = (SelectStmt *) view_stmt->query;
			// Now we have access to the different parts of the query


			/* In the following section we will address all possible projections of the target list
			 * i.e. the coloumns of the view that is to be created
			 */
			target_list = select_stmt->targetList;

			num_sets = pow(2, target_list->length) - 1;
			/*
			while(num_sets--){
				// here we go through all the powersets (in reverse order i.e. start with all elements end with 0
				// which we actually do not want, therefore -1 above)
				next_list = get_powerset(*target_list, num_sets);

				// This represents one of all possible projections of this target list
				// We could use this for defining a new view or sth else
			}
			*/
			/* In the following section we will handle the different rules for the where clause
			 * ATM we only split the outermost boolean function up
			 * 
			 * What about arithmetic expressions? We need to check for that!
			 */

			 where_clause_node = select_stmt->whereClause;
			 if(where_clause_node->type == T_BoolExpr){
			 	/* then we know we have some boolean expression
			 	 * We have where_clause -> args which is a List * holding arguments
			 	 * BoolExpr *arg1 = (BoolExpr *) where_clause -> args -> head -> data
			 	 * BoolExpr *arg2 = (BoolExpr *) where_clause -> args -> tail -> data
			 	 */
			 	where_clause = (BoolExpr *) where_clause_node;

			 	if(where_clause->boolop == AND_EXPR){
			 		// case AND
			 		// Here we can create two views that include the new view
			 	} else if(where_clause->boolop == OR_EXPR){
			 		// case OR
			 		// Here we can create two views that are included by the new view
			 	} else if(where_clause->boolop == NOT_EXPR){
			 		// case NOT
			 		// Here we can use the NOT trick
			 	} else {
			 		// invalid
			 		return FALSE;
			 	}
			 } else {
			 	// then we do not have a boolean expression here
			 	// probably arithmetic
			 }

		} else {
			// Neither T_CreateStmt nor T_ViewStmt
			// Maybe returning false is not so good since it is unsupported not wrong
			return FALSE;
		}

	}

}

List* get_powerset(List target_list, int i){
	bool *bitmask;
	int it, length;
	ListCell *to_delete;
	ListCell *prev;
	List *new_list;

	// First the same
	new_list = &target_list;
	length = new_list->length;

	// Here we get the bitmask representing the ith powerset
	bitmask = get_bitmask(i, length);
	for(it = length; it > 0; it++){ 
		if(!bitmask[it]){
			to_delete = list_nth_cell(new_list, it);
			prev = list_nth_cell(new_list, it-1);
			new_list = list_delete_cell(new_list, to_delete, prev);
		}
	}

	// Return
	return new_list;
}

bool* get_bitmask(int num, int length){
	int log_mask, mask;
	bool *bitmask;

	log_mask = (int) log(num);
	bitmask = calloc(length, sizeof(bool));

	while(log_mask--){
		mask = 1 << log_mask;
		mask = mask & num;
		bitmask[log_mask] = (bool) mask >> log_mask;
	}

	return bitmask;
}