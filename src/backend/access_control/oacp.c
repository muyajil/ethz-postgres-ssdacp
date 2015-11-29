#include "catalog/ssdacp.h"
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

static AclMode
ssdacp_restrict_and_check_grant(bool is_grant, AclMode avail_goptions, bool all_privs,
						 AclMode privileges, Oid objectId, Oid grantorId,
						 AclObjectKind objkind, const char *objname,
						 AttrNumber att_number, const char *colname)
{
	AclMode		this_privileges;
	AclMode		whole_mask;

	switch (objkind)
	{
		case ACL_KIND_COLUMN:
			whole_mask = ACL_ALL_RIGHTS_COLUMN;
			break;
		case ACL_KIND_CLASS:
			whole_mask = ACL_ALL_RIGHTS_RELATION;
			break;
		case ACL_KIND_SEQUENCE:
			whole_mask = ACL_ALL_RIGHTS_SEQUENCE;
			break;
		case ACL_KIND_DATABASE:
			whole_mask = ACL_ALL_RIGHTS_DATABASE;
			break;
		case ACL_KIND_PROC:
			whole_mask = ACL_ALL_RIGHTS_FUNCTION;
			break;
		case ACL_KIND_LANGUAGE:
			whole_mask = ACL_ALL_RIGHTS_LANGUAGE;
			break;
		case ACL_KIND_LARGEOBJECT:
			whole_mask = ACL_ALL_RIGHTS_LARGEOBJECT;
			break;
		case ACL_KIND_NAMESPACE:
			whole_mask = ACL_ALL_RIGHTS_NAMESPACE;
			break;
		case ACL_KIND_TABLESPACE:
			whole_mask = ACL_ALL_RIGHTS_TABLESPACE;
			break;
		case ACL_KIND_FDW:
			whole_mask = ACL_ALL_RIGHTS_FDW;
			break;
		case ACL_KIND_FOREIGN_SERVER:
			whole_mask = ACL_ALL_RIGHTS_FOREIGN_SERVER;
			break;
		case ACL_KIND_EVENT_TRIGGER:
			elog(ERROR, "grantable rights not supported for event triggers");
			/* not reached, but keep compiler quiet */
			return ACL_NO_RIGHTS;
		case ACL_KIND_TYPE:
			whole_mask = ACL_ALL_RIGHTS_TYPE;
			break;
		default:
			elog(ERROR, "unrecognized object kind: %d", objkind);
			/* not reached, but keep compiler quiet */
			return ACL_NO_RIGHTS;
	}

	/*
	 * If we found no grant options, consider whether to issue a hard error.
	 * Per spec, having any privilege at all on the object will get you by
	 * here.
	 */
	if (avail_goptions == ACL_NO_RIGHTS)
	{
		if (pg_aclmask(objkind, objectId, att_number, grantorId,
					   whole_mask | ACL_GRANT_OPTION_FOR(whole_mask),
					   ACLMASK_ANY) == ACL_NO_RIGHTS)
		{
			if (objkind == ACL_KIND_COLUMN && colname)
				aclcheck_error_col(ACLCHECK_NO_PRIV, objkind, objname, colname);
			else
				aclcheck_error(ACLCHECK_NO_PRIV, objkind, objname);
		}
	}

	/*
	 * Restrict the operation to what we can actually grant or revoke, and
	 * issue a warning if appropriate.  (For REVOKE this isn't quite what the
	 * spec says to do: the spec seems to want a warning only if no privilege
	 * bits actually change in the ACL. In practice that behavior seems much
	 * too noisy, as well as inconsistent with the GRANT case.)
	 */
	this_privileges = privileges & ACL_OPTION_TO_PRIVS(avail_goptions);
	if (is_grant)
	{
		if (this_privileges == 0)
		{
			if (objkind == ACL_KIND_COLUMN && colname)
				ereport(WARNING,
errcode(ERRCODE_WARNING_PRIVILEGE_NOT_GRANTED),
						 errmsg("no privileges were granted for column \"%s\" of relation \"%s\"",
								colname, objname)));
			else
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_GRANTED),
						 errmsg("no privileges were granted for \"%s\"",
								objname)));
		}
		else if (!all_privs && this_privileges != privileges)
		{
			if (objkind == ACL_KIND_COLUMN && colname)
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_GRANTED),
						 errmsg("not all privileges were granted for column \"%s\" of relation \"%s\"",
								colname, objname)));
			else
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_GRANTED),
						 errmsg("not all privileges were granted for \"%s\"",
								objname)));
		}
	}
	else
	{
		if (this_privileges == 0)
		{
			if (objkind == ACL_KIND_COLUMN && colname)
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_REVOKED),
						 errmsg("no privileges could be revoked for column \"%s\" of relation \"%s\"",
								colname, objname)));
			else
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_REVOKED),
						 errmsg("no privileges could be revoked for \"%s\"",
								objname)));
		}
		else if (!all_privs && this_privileges != privileges)
		{
			if (objkind == ACL_KIND_COLUMN && colname)
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_REVOKED),
						 errmsg("not all privileges could be revoked for column \"%s\" of relation \"%s\"",
								colname, objname)));
			else
				ereport(WARNING,
						(errcode(ERRCODE_WARNING_PRIVILEGE_NOT_REVOKED),
					 errmsg("not all privileges could be revoked for \"%s\"",
							objname)));
		}
	}

	return this_privileges;
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
	ListCell   *l;
	bool		result = true;

	foreach(l, rangeTable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(l);

		result = ExecCheckRTEPerms(rte);
		if (!result)
		{
			Assert(rte->rtekind == RTE_RELATION);
			if (ereport_on_violation)
				aclcheck_error(ACLCHECK_NO_PRIV, ACL_KIND_CLASS,
							   get_rel_name(rte->relid));
			return false;
		}
	}

	if (ExecutorCheckPerms_hook)
		result = (*ExecutorCheckPerms_hook) (rangeTable,
											 ereport_on_violation);
	return result;
}

/*
 * ExecCheckRTEPerms
 *		Check access permissions for a single RTE.
 */
static bool
ExecCheckRTEPerms(RangeTblEntry *rte)
{
	AclMode		requiredPerms;
	AclMode		relPerms;
	AclMode		remainingPerms;
	Oid			relOid;
	Oid			userid;

	/*
	 * Only plain-relation RTEs need to be checked here.  Function RTEs are
	 * checked by init_fcache when the function is prepared for execution.
	 * Join, subquery, and special RTEs need no checks.
	 */
	if (rte->rtekind != RTE_RELATION)
		return true;

	/*
	 * No work if requiredPerms is empty.
	 */
	requiredPerms = rte->requiredPerms;
	if (requiredPerms == 0)
		return true;

	relOid = rte->relid;

	/*
	 * userid to check as: current user unless we have a setuid indication.
	 *
	 * Note: GetUserId() is presently fast enough that there's no harm in
	 * calling it separately for each RTE.  If that stops being true, we could
	 * call it once in ExecCheckRTPerms and pass the userid down from there.
	 * But for now, no need for the extra clutter.
	 */
	userid = rte->checkAsUser ? rte->checkAsUser : GetUserId();

	/*
	 * We must have *all* the requiredPerms bits, but some of the bits can be
	 * satisfied from column-level rather than relation-level permissions.
	 * First, remove any bits that are satisfied by relation permissions.
	 */
	relPerms = pg_class_aclmask(relOid, userid, requiredPerms, ACLMASK_ALL);
	remainingPerms = requiredPerms & ~relPerms;
	if (remainingPerms != 0)
	{
		int			col = -1;

		/*
		 * If we lack any permissions that exist only as relation permissions,
		 * we can fail straight away.
		 */
		if (remainingPerms & ~(ACL_SELECT | ACL_INSERT | ACL_UPDATE))
			return false;

		/*
		 * Check to see if we have the needed privileges at column level.
		 *
		 * Note: failures just report a table-level error; it would be nicer
		 * to report a column-level error if we have some but not all of the
		 * column privileges.
		 */
		if (remainingPerms & ACL_SELECT)
		{
			/*
			 * When the query doesn't explicitly reference any columns (for
			 * example, SELECT COUNT(*) FROM table), allow the query if we
			 * have SELECT on any column of the rel, as per SQL spec.
			 */
			if (bms_is_empty(rte->selectedCols))
			{
				if (pg_attribute_aclcheck_all(relOid, userid, ACL_SELECT,
											  ACLMASK_ANY) != ACLCHECK_OK)
					return false;
			}

			while ((col = bms_next_member(rte->selectedCols, col)) >= 0)
			{
				/* bit #s are offset by FirstLowInvalidHeapAttributeNumber */
				AttrNumber	attno = col + FirstLowInvalidHeapAttributeNumber;

				if (attno == InvalidAttrNumber)
				{
					/* Whole-row reference, must have priv on all cols */
					if (pg_attribute_aclcheck_all(relOid, userid, ACL_SELECT,
												  ACLMASK_ALL) != ACLCHECK_OK)
						return false;
				}
				else
				{
					if (pg_attribute_aclcheck(relOid, attno, userid,
											  ACL_SELECT) != ACLCHECK_OK)
						return false;
				}
			}
		}

		/*
		 * Basically the same for the mod columns, for both INSERT and UPDATE
		 * privilege as specified by remainingPerms.
		 */
		if (remainingPerms & ACL_INSERT && !ExecCheckRTEPermsModified(relOid,
																	  userid,
														   rte->insertedCols,
																 ACL_INSERT))
			return false;

		if (remainingPerms & ACL_UPDATE && !ExecCheckRTEPermsModified(relOid,
																	  userid,
															rte->updatedCols,
																 ACL_UPDATE))
			return false;
	}
	return true;
}