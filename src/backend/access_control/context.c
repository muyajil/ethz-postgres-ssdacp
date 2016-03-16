#include <stdlib.h>
#include "access_control/context.h"


/* Start declarations */

void ac_context_push(ac_context *context);
ac_context *ac_context_pop(void);
bool perform_mapping(void);
ac_map *ac_map_init(ac_map *map);
int ac_map_append(ac_map *map, char* value);
int ac_map_get_index(ac_map *map, char* value);
char* ac_map_get_value(ac_map *map, int index);
void ac_map_insert_at(ac_map *map, int index, char* value);
void ac_map_double_capacity_if_full(ac_map *map);
//int add_to_map(const char *relname);
//int find_in_map(const char *relation_name);
//List* get_powerset(List target_list, int i);
//bool* get_bitmask(int num, int length);

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
ac_context *ac_context_pop(void){
	ac_context *popped;
	if(context_stack.size == context_stack.free_slots){
		/* In this case the stack is empty and we return NULL */
		return NULL;
	}
	popped = context_stack.top;
	context_stack.free_slots++;
	context_stack.array[context_stack.size - context_stack.free_slots] = NULL;
	context_stack.top = context_stack.array[context_stack.size - context_stack.free_slots -1];
	if(context_stack.size == context_stack.free_slots){
		context_stack.top = NULL;
	}
	return popped;
}

bool perform_mapping(void){
	ViewStmt *view_stmt;
	CreateStmt *create_stmt;
	SelectStmt *select_stmt;
	//List *next_list;
	//List *target_list;
	List *from_list;
	//Node *where_clause_node;
	char *relation_name;
	RangeVar *relation;
	//char *select_query_string;
	//BoolExpr *where_clause;
	Query query;
	//int num_sets;
	//List *parsetree_list;
	//Node *parsetree;
	//Query *select_query;
	int index_table;
	int index_view;

	// Get the query from the stack (we get a copy, so we can modify it as we wish)
	query = *(context_stack.top->query);
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

		 index_table = ac_map_append(all_relations, relation_name);

		 // Next we will construct a string representing a SELECT that collects all data from the new table
		 //select_query_string = malloc(strlen(relation_name)+15);
		 //strcpy(select_query_string, "SELECT * FROM ");
		 //strcat(select_query_string, relation_name);
		 //strcat(select_query_string, ";");
		 //free(relation_name);

		 //Now we will parse this
		 //parsetree_list = pg_parse_query(select_query_string);
		 //parsetree = (Node *)lfirst(parsetree_list->head);
		 //select_query = parse_analyze(parsetree, select_query_string, NULL, 0);
		 
		 return TRUE;

		 //Maybe try with raw_parser and then pg_analyze, probably better results.

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

			from_list = select_stmt->fromClause;
			if(from_list->length == 1){
				//ATM we only support this type of view
				relation = (RangeVar *) from_list->head->data.ptr_value;
				relation_name = relation->relname;
				// Add the new view to the map
				index_view = ac_map_append(all_relations, view_stmt->view->relname);
				index_table = ac_map_get_index(all_relations, relation_name);
				ac_map_insert_at(includes, index_table, view_stmt->view->relname);
				ac_map_insert_at(included_in, index_view, relation_name);
				//*(includes+index_table) = view_stmt->view->relname;
				//*(included_in+index_view) = relation_name;
			}
			return TRUE;
			/* In the following section we will address all possible projections of the target list
			 * i.e. the coloumns of the view that is to be created
			 */
			//target_list = select_stmt->targetList;

			//num_sets = pow(2, target_list->length) - 1;
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

			 /*
			 where_clause_node = select_stmt->whereClause;
			 if(where_clause_node->type == T_BoolExpr){
			 	// then we know we have some boolean expression
			 	// We have where_clause -> args which is a List * holding arguments
			 	// BoolExpr *arg1 = (BoolExpr *) where_clause -> args -> head -> data
			 	// BoolExpr *arg2 = (BoolExpr *) where_clause -> args -> tail -> data
			 	 
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
			 */

		} else {
			// Neither T_CreateStmt nor T_ViewStmt
			// Maybe returning false is not so good since it is unsupported not wrong
			return FALSE;
		}

	}
	return FALSE;
}

ac_map* ac_map_init(ac_map *map){
  	// initialize size and capacity
  	if(map == NULL){
  		map = (ac_map *) malloc(sizeof(ac_map));
  	}
  	map->size = 0;
  	map->capacity = INIT_MAP_SIZE;

  	// allocate memory for vector->data
  	map->data = calloc(map->capacity, sizeof(char *));
  	return map;
}

int ac_map_append(ac_map *map, char* value){
	if(map == NULL){
		map = ac_map_init(map);
	}
	// make sure there's room to expand into
  	ac_map_double_capacity_if_full(map);

  	// append the value and increment vector->size
  	map->data[map->size++] = value;
  	return map->size-1;
}

int ac_map_get_index(ac_map *map, char* value){
	int index = -1;
	int iterator = 0;
	while(iterator < map->capacity && index < 0){
		if(!strcmp(value, map->data[iterator])){
			index = iterator;
		}
	}
	return index;
}

char* ac_map_get_value(ac_map *map, int index){
	if (index >= map->size || index < 0) {
    	return NULL;
  	}
  	return map->data[index];
}

void ac_map_insert_at(ac_map *map, int index, char* value){
	if(map == NULL){
		map = ac_map_init(map);
	}
	// zero fill the vector up to the desired index
  	while (index >= map->size) {
    	ac_map_append(map, 0);
  	}
	// set the value at the desired index
  	map->data[index] = value;
}

void ac_map_double_capacity_if_full(ac_map *map){
	if (map->size >= map->capacity) {
    	// double vector->capacity and resize the allocated memory accordingly
    	map->capacity *= 2;
    	map->data = realloc(map->data, sizeof(char *) * map->capacity);
  	}
}

/*
int add_to_map(const char *relname){
	int stepsize = sizeof(char *);
	all_relations = (char **) realloc(all_relations, ++num_relations*sizeof(char *));
	includes = (char **) realloc(includes, ++num_relations*sizeof(char *));
	included_in = (char **) realloc(included_in, ++num_relations*sizeof(char *));
	*(all_relations+(num_relations-1)*stepsize) = relname;
	return num_relations-1;
}

int find_in_map(const char *relation_name){
	int index = 0;
	int stepsize = sizeof(char *);
	while(strcmp(relation_name, *(all_relations+index*stepsize))) index++;
	return index;
}
*/

/*

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
*/