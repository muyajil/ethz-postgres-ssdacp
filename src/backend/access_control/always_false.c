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
 * If the command is not allowed this function returns false
 */

bool ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation);

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
}

Oid ssdacp_RangeVarGetAndCheckCreationNamespace(RangeVar *relation,
									     LOCKMODE lockmode,
									     Oid *existing_relation_id)
{

}

ObjectAddress ssdacp_CreateTrigger(CreateTrigStmt *stmt, const char *queryString,
			  Oid relOid, Oid refRelOid, Oid constraintOid, Oid indexOid,
			  bool isInternal)
{

}

bool ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation)
{

}

ac_return_data authorized(ac_decision_data *decision_data)
{

}