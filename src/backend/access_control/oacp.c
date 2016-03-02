#include "access_control/access_control.h"

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
	uint64		inval_count;
	Oid			relid;
	Oid			oldrelid = InvalidOid;
	Oid			nspid;
	Oid			oldnspid = InvalidOid;
	bool		retry = false;

	/*
	 * We check the catalog name and then ignore it.
	 */
	if (relation->catalogname)
	{
		if (strcmp(relation->catalogname, get_database_name(MyDatabaseId)) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cross-database references are not implemented: \"%s.%s.%s\"",
							relation->catalogname, relation->schemaname,
							relation->relname)));
	}

	/*
	 * As in RangeVarGetRelidExtended(), we guard against concurrent DDL
	 * operations by tracking whether any invalidation messages are processed
	 * while we're doing the name lookups and acquiring locks.  See comments
	 * in that function for a more detailed explanation of this logic.
	 */
	for (;;)
	{
		AclResult	aclresult;

		inval_count = SharedInvalidMessageCounter;

		/* Look up creation namespace and check for existing relation. */
		nspid = RangeVarGetCreationNamespace(relation);
		Assert(OidIsValid(nspid));
		if (existing_relation_id != NULL)
			relid = get_relname_relid(relation->relname, nspid);
		else
			relid = InvalidOid;

		/*
		 * In bootstrap processing mode, we don't bother with permissions or
		 * locking.  Permissions might not be working yet, and locking is
		 * unnecessary.
		 */
		if (IsBootstrapProcessingMode())
			break;

		/* Check namespace permissions. */
		aclresult = pg_namespace_aclcheck(nspid, GetUserId(), ACL_CREATE);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
						   get_namespace_name(nspid));

		if (retry)
		{
			/* If nothing changed, we're done. */
			if (relid == oldrelid && nspid == oldnspid)
				break;
			/* If creation namespace has changed, give up old lock. */
			if (nspid != oldnspid)
				UnlockDatabaseObject(NamespaceRelationId, oldnspid, 0,
									 AccessShareLock);
			/* If name points to something different, give up old lock. */
			if (relid != oldrelid && OidIsValid(oldrelid) && lockmode != NoLock)
				UnlockRelationOid(oldrelid, lockmode);
		}

		/* Lock namespace. */
		if (nspid != oldnspid)
			LockDatabaseObject(NamespaceRelationId, nspid, 0, AccessShareLock);

		/* Lock relation, if required if and we have permission. */
		if (lockmode != NoLock && OidIsValid(relid))
		{
			if (!pg_class_ownercheck(relid, GetUserId()))
				aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_CLASS,
							   relation->relname);
			if (relid != oldrelid)
				LockRelationOid(relid, lockmode);
		}

		/* If no invalidation message were processed, we're done! */
		if (inval_count == SharedInvalidMessageCounter)
			break;

		/* Something may have changed, so recheck our work. */
		retry = true;
		oldrelid = relid;
		oldnspid = nspid;
	}

	RangeVarAdjustRelationPersistence(relation, nspid);
	if (existing_relation_id != NULL)
		*existing_relation_id = relid;
	return nspid;
}

void ssdacp_CreateTrigger(bool isInternal, Relation rel, Oid constrrelid, AclResult *aclresult)
{
	if (!isInternal)
	{
		aclresult = pg_class_aclcheck(RelationGetRelid(rel), GetUserId(),
									  ACL_TRIGGER);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, ACL_KIND_CLASS,
						   RelationGetRelationName(rel));

		if (OidIsValid(constrrelid))
		{
			aclresult = pg_class_aclcheck(constrrelid, GetUserId(),
										  ACL_TRIGGER);
			if (aclresult != ACLCHECK_OK)
				aclcheck_error(aclresult, ACL_KIND_CLASS,
							   get_rel_name(constrrelid));
		}
	}
}

bool ssdacp_ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation)
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

/*
 * ExecCheckRTEPermsModified
 *		Check INSERT or UPDATE access permissions for a single RTE (these
 *		are processed uniformly).
 */
static bool
ExecCheckRTEPermsModified(Oid relOid, Oid userid, Bitmapset *modifiedCols,
						  AclMode requiredPerms)
{
	int			col = -1;

	/*
	 * When the query doesn't explicitly update any columns, allow the query
	 * if we have permission on any column of the rel.  This is to handle
	 * SELECT FOR UPDATE as well as possible corner cases in UPDATE.
	 */
	if (bms_is_empty(modifiedCols))
	{
		if (pg_attribute_aclcheck_all(relOid, userid, requiredPerms,
									  ACLMASK_ANY) != ACLCHECK_OK)
			return false;
	}

	while ((col = bms_next_member(modifiedCols, col)) >= 0)
	{
		/* bit #s are offset by FirstLowInvalidHeapAttributeNumber */
		AttrNumber	attno = col + FirstLowInvalidHeapAttributeNumber;

		if (attno == InvalidAttrNumber)
		{
			/* whole-row reference can't happen here */
			elog(ERROR, "whole-row update is not implemented");
		}
		else
		{
			if (pg_attribute_aclcheck(relOid, attno, userid,
									  requiredPerms) != ACLCHECK_OK)
				return false;
		}
	}
	return true;
}

ac_return_data authorized(ac_decision_data *decision_data){

	/* Declare return data */
	ac_return_data return_data;

	/* Based on the command type call the correct decision functions */
	if(decision_data->create_relation_data != NULL){
		return_data.target_namespace = ssdacp_RangeVarGetAndCheckCreationNamespace(
			decision_data->create_relation_data->relation,
			decision_data->create_relation_data->lockmode,
			decision_data->create_relation_data->existing_relation_id);
		return return_data;
	} else if(decision_data->grant_data != NULL){
		return_data.current_privileges = ssdacp_restrict_and_check_grant(
			decision_data->grant_data->is_grant,
			decision_data->grant_data->avail_goptions,
			decision_data->grant_data->all_privs,
			decision_data->grant_data->privileges,
			decision_data->grant_data->objectId,
			decision_data->grant_data->grantorId,
			decision_data->grant_data->objkind,
			decision_data->grant_data->objname,
			decision_data->grant_data->att_number,
			decision_data->grant_data->colname);
		return return_data;
	} else if(decision_data->create_trigger_data != NULL){
		ssdacp_CreateTrigger(
			decision_data->create_trigger_data->isInternal,
			decision_data->create_trigger_data->rel,
			decision_data->create_trigger_data->constrrelid,
			decision_data->create_trigger_data->aclresult);
	} else if(decision_data->nutility_data != NULL){
		return_data.execute = ssdacp_ExecCheckRTPerms(
			decision_data->nutility_data->rangeTable,
			decision_data->nutility_data->ereport_on_violation);
		return return_data;
	} 
	return_data.nothing = 0;
	return return_data;
}