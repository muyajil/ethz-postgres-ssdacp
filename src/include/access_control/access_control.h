#ifndef SSDACP_H
#define SSDACP_H

//Enum for command that was issued
typedef enum {INSERT, DELETE, SELECT, GRANT, REVOKE, CREATE_TRIGGER, CREATE_RELATION} command_type;


#define SSDACP_ACTIVATE 1

typedef struct ac_decision_data {
	bool trigger; //Trigger Flag, TRUE if inside trigger


} ac_decision_data;

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
