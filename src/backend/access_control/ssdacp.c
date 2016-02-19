#include "access_control/access_control.h"
#include "access_control/context.h"
#include "commands/trigger.h"

/* Declaration of authorized interface
 * Here we need to check what kind of command we are dealing with and pass the args
 * to the respective function.
 * We also need to set the return value union and return that further up.
 */
ac_return_data authorized(ac_decision_data *decision_data);

bool integrity_auth(state s, action a);

/* Pseudocode implementation of the integrity_auth function
 * In the paper this is the auth function.
 * Argument types and must still be declared
 *
 */

bool integrity_auth(state s, action a){
    bool result = false;

    return result;
}

ac_return_data authorized(ac_decision_data *decision_data){

	bool integrity = false;
	bool confidentiality = false;

	// We start by finding out if the query violates integrity
	// The first thing we need to find out is if we are executing a trigger
	// For that there is a function in trigger.c Datum pg_trigger_depth(PG_FUNCTION_ARGS)
	// I don't get why we need the argument there, I will try to pass NULL.
	// Datum is defined by: typedef uintptr_t Datum 
	// uintptr_t is an unsigned int that is capable of holding a pointer
	if(pg_trigger_depth(NULL) > 0){
		// Here we should be in a trigger
		// Next we need to find out if we are executing a triggers condition 
		// I.e. if we are in a trigger and executing a SELECT command
		if(true){
			integrity = true;
		} else {
			// Next we need to check if we are executing the triggers action (should always be true?)
			if(true){
				integrity = integrity_auth(decision_data);
			}
		}

	} else {
		// Here we should NOT be in a trigger
		// Therefore we should be able to just return auth
		integrity = integrity_auth(decision_data);
	}

	// Next we take care about confidentiality
	

}
