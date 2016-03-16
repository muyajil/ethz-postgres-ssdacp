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
	Query *query;
	const char *query_string;
	bool authorized;
	bool rewritten;
	bool authorizes_next;
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

/* Maps Data structure */
struct ac_map_struct{
	int size;
	int capacity;
	char **data;
};

typedef struct ac_map_struct ac_map;

#define INIT_MAP_SIZE 100

/* Functions relevant to maps */

extern void ac_map_init(ac_map **map);

extern int ac_map_append(ac_map **map, char* value);

extern int ac_map_get_index(ac_map **map, char* value);

extern char* ac_map_get_value(ac_map **map, int index);

extern void ac_map_insert_at(ac_map **map, int index, char* value);

extern void ac_map_double_capacity_if_full(ac_map **map);

/* global 2d - map: view -> included in 
 * Here we take the intersections for the rewriting
 */
extern ac_map *included_in;

/* global 2d - map: view -> includes 
 * Here we take the unions for the rewriting
 */
extern ac_map *includes;

/* global 1d - map: all created tables and views (even helper views) 
 * just an array of char pointers, each one 8 bytes
 * this gives the index into the 2d maps above
 */
extern ac_map *all_relations;

extern bool psql_connection;

/* number of char pointers in the array above */
//extern int num_relations;

/* add to maps function, returns the index of the table after adding */
//extern int add_to_map(const char *relname);

//extern int find_in_map(const char *relname);

/* perform mapping function */
extern bool perform_mapping();

/* Stack push method */
//extern void ac_context_push(ac_context *context, ac_context_stack *context_stack);
extern void ac_context_push(ac_context *context);

/* global context stack */
extern ac_context_stack context_stack;

/* Stack pop method */
extern ac_context *ac_context_pop();

/* Method for getting options, defined in guc.c */
extern char* GetConfigOptionByName(const char *name, const char **varname, bool missing_ok);

/* Method for analyzing and parsing query, defined in postgres.c */
Query * parse_analyze(Node *parseTree, const char *sourceText, Oid *paramTypes, int numParams);

#endif /* CONTEXT_H */