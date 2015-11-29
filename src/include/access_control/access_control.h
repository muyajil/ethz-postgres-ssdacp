#ifndef SSDACP_H
#define SSDACP_H

/* Enum for command that was issued */
typedef enum {INSERT, DELETE, SELECT, GRANT, REVOKE, CREATE_TRIGGER, CREATE_RELATION} command_type;


#define SSDACP_ACTIVATE 1

typedef struct ac_decision_data {
	bool trigger; /* Trigger Flag, TRUE if inside trigger */
	command_type command; /* Flag that indicates the command that was used */
	RangeVar *relation; /* If command == CREATE_RELATION we need to set this */
} ac_decision_data;

const struct AC_DECISION_DATA_DEFAULT {FALSE, NULL, NULL};

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
