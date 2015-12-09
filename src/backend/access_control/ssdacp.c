#include "access_control.h"
#include "postgres_ext.h"
#include "nodes/parsenodes.h"
#include "utils/acl.h"
#include "access/attnum.h"
#include "storage/lockdefs.h"
#include "primnodes.h"
#include "nodes/pg_list.h"
#include "c.h"

/* Declaration of authorized interface
 * Here we need to check what kind of command we are dealing with and pass the args
 * to the respective function.
 * We also need to set the return value union and return that further up.
 */
ac_return_data authorized(ac_decision_data *decision_data);

ac_return_data authorized(ac_decision_data *decision_data)
{

}