#include "access_control/access_control.h"
#include "access_control/context.h"

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
    if(a == INSERT || a == DELETE){
        if(s.admin)
    }
}

ac_return_data authorized(ac_decision_data *decision_data){

}
