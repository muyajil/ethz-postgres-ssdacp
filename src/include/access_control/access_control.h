#ifndef SSDACP_H
#define SSDACP_H

#include <stdlib.h>
#include "c.h"
#include "postgres_ext.h"
#include "nodes/parsenodes.h"
#include "utils/acl.h"
#include "access/attnum.h"
#include "storage/lockdefs.h"
#include "nodes/primnodes.h"
#include "nodes/pg_list.h"


#define SSDACP_ACTIVATE 1

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
union ac_return_data_struct {
	Oid target_namespace; /* Returned if CREATE is authorized */
	AclMode current_privileges; /* Returned if GRANT is authorized */
	bool execute; /* Returned if Non-utility command is authorized */
};

typedef union ac_return_data_struct ac_return_data;

/* Data needed for deciding on CREATE queries */
struct ac_create_relation_data_struct {
	RangeVar *relation;
	LOCKMODE lockmode;
	Oid *existing_relation_id;
};

typedef struct ac_create_relation_data_struct ac_create_relation_data;

/* Data needed for deciding on GRANT queries */
struct ac_grant_data_struct {
	bool is_grant;
	AclMode avail_goptions;
	bool all_privs;
	AclMode privileges;
	Oid objectId;
	Oid grantorId;
	AclObjectKind objkind;
	const char *objname;
	AttrNumber att_number;
	const char *colname;
};

typedef struct ac_grant_data_struct ac_grant_data;

/* Data needed for deciding on Non utility queries */
struct ac_nutility_data_struct {
	List *rangeTable;
	bool ereport_on_violation;
};

typedef struct ac_nutility_data_struct ac_nutility_data;

/* Data needed for deciding on CREATE TRIGGER queries */
struct ac_create_trigger_data_struct {
	bool isInternal;
	Relation rel;
	Oid constrrelid;
	AclResult *aclresult;
};

typedef struct ac_create_trigger_data_struct ac_create_trigger_data;

struct ac_decision_data_struct {
	bool trigger; /* Trigger Flag, TRUE if inside trigger */
	command_type command; /* Flag that indicates the command that was used */
	ac_create_relation_data *create_relation_data; /* If (command == CREATE_RELATION) this cannot be NULL */
	ac_grant_data *grant_data; /* If (command == GRANT || command == REVOKE) this cannot be NULL */
	ac_nutility_data *nutility_data; /* If (command == INSERT || command == DELETE || command == SELECT) this cannot be NULL */
	ac_create_trigger_data *create_trigger_data; /* If (command == CREATE_TRIGGER) this cannot be NULL */
};

typedef struct ac_decision_data_struct ac_decision_data;

const ac_decision_data AC_DECISION_DATA_DEFAULT = {FALSE, DEFAULT, NULL, NULL, NULL, NULL};

/*
 * Declaration of the interface authorized
 * Input arguments is all data needed to make the decision
 * The return type depends on the type of command issued -> see typedef ac_return_data
 *
 */
extern ac_return_data authorized(ac_decision_data *decision_data);

#endif /* SSDACP_H */
