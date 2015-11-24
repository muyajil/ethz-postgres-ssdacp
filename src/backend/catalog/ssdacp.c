#include "catalog/ssdacp.h"
#include "postgres_ext.h"
#include "nodes/parsenodes.h"
#include "utils/acl.h"
#include "access/attnum.h"
#include "storage/lockdefs.h"
#include "primnodes.h"

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
 * 
 */
Oid ssdacp_RangeVarGetAndCheckCreationNamespace(RangeVar *relation,
									     LOCKMODE lockmode,
									     Oid *existing_relation_id);

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
