#ifndef SSDACP_H
#define SSDACP_H

/* Enum for command that was issued */
typedef enum {
	INSERT, 
	DELETE, 
	SELECT, 
	GRANT, 
	REVOKE, 
	CREATE_TRIGGER, 
	CREATE_RELATION
} command_type;

/* Union for the return value of authorized 
 * Union is NULL if CREATE TRIGGER is allowed
 */
typedef union ac_return_data {
	Oid target_namespace; /* Returned if CREATE is authorized */
	AclMode current_privileges; /* Returned if GRANT is authorized */
	bool execute; /* Returned if Non-utility command is authorized */
} ac_return_data;


#define SSDACP_ACTIVATE 1

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
	ac_create_relation_data create_relation_data; /* If (command == CREATE_RELATION) this cannot be NULL */
	ac_grant_data grant_data; /* If (command == GRANT || command == REVOKE) this cannot be NULL */
	ac_nutility_data nutility_data; /* If (command == INSERT || command == DELETE || command == SELECT) this cannot be NULL */
	ac_create_trigger_data create_trigger_data; /* If (command == CREATE_TRIGGER) this cannot be NULL */
} ac_decision_data;

const struct AC_DECISION_DATA_DEFAULT {FALSE, NULL, NULL, NULL, NULL, NULL};

/*
 * If allowed return oid of target namespace
 * If not allowed aclcheck_error
 */
extern Oid ssdacp_RangeVarGetAndCheckCreationNamespace(RangeVar *relation,
									     LOCKMODE lockmode,
									     Oid *existing_relation_id);

/*
 * If allowed return current privileges
 * If not allowed aclcheck_error
 */
extern AclMode ssdacp_restrict_and_check_grant(bool is_grant, AclMode avail_goptions,
						 bool all_privs, AclMode privileges,
						 Oid objectId, Oid grantorId,
						 AclObjectKind objkind, const char *objname,
						 AttrNumber att_number, const char *colname);

/*
 * If allowed do nothing
 * If not allowed aclcheck_error
 */
extern void ssdacp_CreateTrigger(bool isInternal, Relation rel, Oid constrrelid, AclResult *aclresult);

/*
 * If allowed return true
 * If not allowed return false
 */
extern bool ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation);

/*
 * Declaration of the interface authorized
 * Input arguments is all data needed to make the decision
 * The return type depends on the type of command issued -> see typedef ac_return_data
 *
 */
 extern ac_return_data authorized(ac_decision_data decision_data);

#endif /* SSDACP_H */