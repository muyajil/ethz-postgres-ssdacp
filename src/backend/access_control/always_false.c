#include "access_control.h"
#include "postgres_ext.h"
#include "nodes/parsenodes.h"
#include "utils/acl.h"
#include "access/attnum.h"
#include "storage/lockdefs.h"
#include "primnodes.h"
#include "nodes/pg_list.h"
#include "c.h"

/* Declaration of function that checks GRANT
 * If the grant is allowed this function returns the current privileges
 * If the grant is not allowed this function throws an error and cancels the operation
 */
static AclMode ssdacp_restrict_and_check_grant(bool is_grant, AclMode avail_goptions,
						 bool all_privs, AclMode privileges,
						 Oid objectId, Oid grantorId,
						 AclObjectKind objkind, const char *objname,
						 AttrNumber att_number, const char *colname);

/* Declaration of function that checks CREATE for relations
 * If the create is allowed this function returns the OID of the target namespace
 * If the create is not allowed this function signals an error
 */
Oid ssdacp_RangeVarGetAndCheckCreationNamespace(RangeVar *relation,
									     LOCKMODE lockmode,
									     Oid *existing_relation_id);

/* Declaration of function that checks CREATE for triggers
 * If the create trigger is allowed this function does nothing
 * If the create trigger is not allowed this function signals an error
 */
void ssdacp_CreateTrigger(bool isInternal, Relation rel, Oid constrrelid, AclResult *aclresult);


/* Declaration of function that checks non_utility commands
 * If the command is allowed this function returns true
 * If the command is not allowed this function returns false and signals an error
 */

bool ssdacp_ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation);

/* Declaration of authorized interface
 * Here we need to check what kind of command we are dealing with and pass the args
 * to the respective function.
 * We also need to set the return value union and return that further up.
 */
ac_return_data authorized(ac_decision_data *decision_data);

static AclMode
ssdacp_restrict_and_check_grant(bool is_grant, AclMode avail_goptions, bool all_privs,
						 AclMode privileges, Oid objectId, Oid grantorId,
						 AclObjectKind objkind, const char *objname,
						 AttrNumber att_number, const char *colname)
{
	/* Signal an errro in any case */
	aclcheck_error(ACLCHECK_NO_PRIV, objkind, objname);
	return NULL;
}

Oid ssdacp_RangeVarGetAndCheckCreationNamespace(RangeVar *relation,
									     LOCKMODE lockmode,
									     Oid *existing_relation_id)
{
	/* Need to set aclresult to pass to aclcheck_error */
	aclresult = pg_namespace_aclcheck(nspid, GetUserId(), ACL_CREATE);
	/* Signal error in any case */
	aclcheck_error(aclresult, ACL_KIND_NAMESPACE, get_namespace_name(nspid));
	return NULL;
}

ObjectAddress ssdacp_CreateTrigger(CreateTrigStmt *stmt, const char *queryString,
			  Oid relOid, Oid refRelOid, Oid constraintOid, Oid indexOid,
			  bool isInternal)
{
	/* Signal an error in any case */
	aclcheck_error(aclresult, ACL_KIND_CLASS, RelationGetRelationName(rel));
	return NULL;
}

bool ssdacp_ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation)
{
	/* Always return false, and signal error */
	aclcheck_error(ACLCHECK_NO_PRIV, ACL_KIND_CLASS, get_rel_name(rte->relid))
	return false;
}

ac_return_data authorized(ac_decision_data *decision_data)
{
	/* Declare return data */
	ac_return_data return_data;

	/* Based on the command type call the correct decision functions */
	if(decision_data->command == CREATE_RELATION){
		return_data.target_namespace = ssdacp_RangeVarGetAndCheckCreationNamespace(
			decision_data->create_relation_data->relation,
			decision_data->create_relation_data->lockmode,
			decision_data->create_relation_data->existing_relation_id);
		return return_data;
	} else if(decision_data->command == GRANT || decision_data->command == REVOKE){
		return_data.current_privileges = ssdacp_restrict_and_check_grant(
			decision_data->grant_data->is_grant,
			decision_data->grant_data->avail_goptions,
			decision_data->grant_data->all_privs,
			decision_data->grant_data->privileges,
			decision_data->grant_data->objectId,
			decision_data->grant_data->grantorId,
			decision_data->grant_data->objkind,
			decision_data->grant_data->objname
			decision_data->grant_data->att_number,
			decision_data->grant_data->colname);
		return return_data;
	} else if(decision_data->command == CREATE_TRIGGER){
		ssdacp_CreateTrigger(
			decision_data->create_trigger_data->isInternal,
			decision_data->create_trigger_data->rel,
			decision_data->create_trigger_data->constrrelid,
			decision_data->create_trigger_data->aclresult);
		return NULL;
	} else if(decision_data->command == INSERT || decision_data->command == SELECT || decision_data->command == DELETE){
		return_data.execute = ssdacp_ExecCheckRTPerms(
			decision_data->nutility_data->rangeTable,
			decision_data->nutility_data->ereport_on_violation);
		return return_data;
	} else {
		/* We get here if decision_data->command == default
		 * This means that one of our functions got called by a function that does
		 * something else than we actually support.
		 * Here we need to define what happens!
		 */
	}
}