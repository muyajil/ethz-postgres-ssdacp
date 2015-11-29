#ifndef SSDACP_H
#define SSDACP_H

/* Enum for command that was issued */
typedef enum {INSERT, DELETE, SELECT, GRANT, REVOKE, CREATE_TRIGGER, CREATE_RELATION} command_type;


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
	AttrNumber att_number;
	const char *colname;
} ac_grant_data;

typedef struct ac_decision_data {
	bool trigger; /* Trigger Flag, TRUE if inside trigger */
	command_type command; /* Flag that indicates the command that was used */
	ac_create_relation_data create_data; /* If (command == CREATE_RELATION) this cannot be NULL */
	ac_grant_data grant_data; /* If (command == GRANT || command == REVOKE) this cannot be NULL */
} ac_decision_data;

const struct AC_DECISION_DATA_DEFAULT {FALSE, NULL, NULL, NULL};

extern Oid ssdacp_RangeVarGetAndCheckCreationNamespace(RangeVar *relation,
									     LOCKMODE lockmode,
									     Oid *existing_relation_id);

extern AclMode ssdacp_restrict_and_check_grant(bool is_grant, AclMode avail_goptions,
						 bool all_privs, AclMode privileges,
						 Oid objectId, Oid grantorId,
						 AclObjectKind objkind, const char *objname,
						 AttrNumber att_number, const char *colname);

extern ObjectAddress ssdacp_CreateTrigger(CreateTrigStmt *stmt, const char *queryString,
			  Oid relOid, Oid refRelOid, Oid constraintOid, Oid indexOid,
			  bool isInternal);

extern bool ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation);

#endif /* SSDACP_H */
