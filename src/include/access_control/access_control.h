#ifndef SSDACP_H
#define SSDACP_H

#define SSDACP_ACTIVATE 1
#define INIT_STACK_SIZE 10

/* Data structure representing context */
typedef struct ac_context{
	Oid user;
	Oid invoker;
	String command;
	Oid trigger;
} ac_context;

/* Context Stack Data Structure */
typedef struct ac_context_stack{
	ac_context **array;
	ac_context *top;
	int size;
	int free_slots;
} ac_context_stack;

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

/* Enum for command that was issued */
typedef enum {
	INSERT, 
	DELETE, 
	SELECT, 
	GRANT, 
	REVOKE, 
	CREATE_TRIGGER, 
	CREATE_RELATION,
	DEFAULT
} command_type;

/* Union for the return value of authorized 
 * Union is NULL if CREATE TRIGGER is allowed
 */
typedef union ac_return_data {
	Oid target_namespace; /* Returned if CREATE is authorized */
	AclMode current_privileges; /* Returned if GRANT is authorized */
	bool execute; /* Returned if Non-utility command is authorized */
} ac_return_data;

/* Data needed for deciding on CREATE queries */
typedef struct ac_create_relation_data {
	RangeVar *relation;
	LOCKMODE lockmode;
	Oid *existing_relation_id;
} ac_create_relation_data;

/* Data needed for deciding on GRANT queries */
typedef struct ac_grant_data {
	bool is_grant;
	AclMode avail_goptions;
	bool all_privs;
	AclMode privileges;
	Oid objectId;
	Oid grantorId;
	AclObjectKind objkind;
	const char *objname;
	AttrNumber att_number;constrrelid
	const char *colname;
} ac_grant_data;

/* Data needed for deciding on Non utility queries */
typedef struct ac_nutility_data {
	List *rangeTable;
	bool ereport_on_violation;
} ac_nutility_data;

/* Data needed for deciding on CREATE TRIGGER queries */
typedef struct ac_create_trigger_data
{
	bool isInternal;
	Relation rel;
	Oid constrrelid;
	AclResult *aclresult;
} ac_create_trigger_data;

typedef struct ac_decision_data {
	bool trigger; /* Trigger Flag, TRUE if inside trigger */
	command_type command; /* Flag that indicates the command that was used */
	ac_create_relation_data *create_relation_data; /* If (command == CREATE_RELATION) this cannot be NULL */
	ac_grant_data *grant_data; /* If (command == GRANT || command == REVOKE) this cannot be NULL */
	ac_nutility_data *nutility_data; /* If (command == INSERT || command == DELETE || command == SELECT) this cannot be NULL */
	ac_create_trigger_data *create_trigger_data; /* If (command == CREATE_TRIGGER) this cannot be NULL */
} ac_decision_data;

const ac_decision_data AC_DECISION_DATA_DEFAULT = {FALSE, DEFAULT, NULL, NULL, NULL, NULL};

/*
 * Declaration of the interface authorized
 * Input arguments is all data needed to make the decision
 * The return type depends on the type of command issued -> see typedef ac_return_data
 *
 */
extern ac_return_data authorized(ac_decision_data *decision_data);

#endif /* SSDACP_H */
