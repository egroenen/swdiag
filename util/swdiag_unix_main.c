/* 
 *  swdiag_unix_main.c
 *
 *     ``The contents of this file are subject to the Mozilla Public License
 *   Version 1.1 (the "License"); you may not use this file except in
 *   compliance with the License. You may obtain a copy of the License at
 *   http://www.mozilla.org/MPL/
 *
 *   Software distributed under the License is distributed on an "AS IS"
 *   basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 *   License for the specific language governing rights and limitations
 *   under the License.
 *
 *   The Original Code is libswdiag.
 *
 *   The Initial Developer of the Original Code is Cisco Systems Inc.
 *   Portions created by Cisco Systems Inc are 
 *   Copyright (C) Cisco Systems Inc. 2007-2009. All Rights Reserved.
 *
 *   Contributor(s): 
 *        Edward Groenendaal     eddyg@myreflection.org
 *        Brett Wollard          brettw@cisco.com
 *        Chandra Wagh           cwagh@cisco.com
 */

/*
 * Multithreaded Cisco SW Diagnostics server
 *
 * Requires two threads, one for servicing RPC's and one for SW Diags
 * itself.
 */
#include "swdiag_client.h"
#include "swdiag_sched.h"
#include <pthread.h>
#include "swdiag_unix_clients.h"
#include "swdiag_unix_rpc.h"
#include "swdiag_cli.h"
#include "swdiag_cli_local.h"

/*
 * swdiag_xos_register_with_master()
 *
 * Set up a TCP connection with the master and then register via RPC
 * with the master so it knows to contact us. Leave the TCP connection
 * open so that we can quickly notify the master should we encounter any
 * problems.
 */
void swdiag_xos_register_with_master (const char *component)
{
    CLIENT *clnt;
    swdiag_register_t register_info;
    char my_hostname[MAXHOSTNAMELEN];
    int retval;
    swdiag_unix_info_t *swdiag_info;

    /*
     * Our system component holds all the info about how to contact the
     * master, so get that first.
     */
    swdiag_info = (swdiag_unix_info_t *)swdiag_api_comp_get_context(SWDIAG_SYSTEM_COMP);

    if (!swdiag_info) {
        /*
         * error
         */
        fprintf(stderr, "Could not get the component context\n");
        return;
    }
    
    /*
     * And register with the master. This needs to be moved into 
     * the main code, with hooks out to here for the actual work.
     */
    swdiag_info->clnt = clnt_create(swdiag_info->host, 
                                   SWDIAG_REMOTE_MASTER_PROTOCOL,
                                   SWDIAG_MASTER_PROTOCOL_V1,
                                   "tcp");
    
    if (swdiag_info->clnt == NULL) {
        fprintf(stderr, "Could not establish contact with master\n");
        return;
    }

    if (gethostname(my_hostname, MAXHOSTNAMELEN) == -1) {
        clnt_destroy(swdiag_info->clnt);
        swdiag_info->clnt = NULL;
        return;
    }

    register_info.entity = (char *)swdiag_api_make_name(my_hostname, swdiag_info->my_name);
    register_info.hostname = strdup(my_hostname); // we need to free this
    register_info.instance = swdiag_info->rpc_instance;

    if (!register_info.hostname) {
        clnt_destroy(swdiag_info->clnt);
        swdiag_info->clnt = NULL;
        return;
    }

    if (swdiag_register_1(register_info, &retval, 
                          swdiag_info->clnt) != RPC_SUCCESS) {
        clnt_destroy(swdiag_info->clnt);
        swdiag_info->clnt = NULL;
        return;
    }

    printf("Send register message to Master, retval=%d\n", retval);
}
 
void swdiag_xos_register_as_master (void)
{

}

void swdiag_xos_slave_to_master (void)
{

}


/* 
 * swdiag_get_info_handle()
 * Description:
 *     This is the wrapper to fetch a handle based on cli type and filter
 * Input:
 *     type refers to type of object as Test/Rule/Action
 *     filter refers to type of object failure or current failure.
 * Return:
 *     handle.
 */

unsigned int swdiag_cli_get_info_handle (const char *name,
                                         cli_type_t type,
                                         cli_type_filter_t filter,
                                         const char *instance_name) 
{
    return (swdiag_cli_local_get_info_handle(name, type, filter, 
                                             instance_name));
}    


/* 
 * swdiag_cli_get_info()
 * Description:
 *     This is the wrapper to fetch information based on handle.  
 * Input:
 *     int:  handle
 * Return:
 *     Pointer to cli_info_t
 */
cli_info_t *swdiag_cli_get_info (unsigned int handle)
{
    return (swdiag_cli_local_get_info(handle, MAX_LOCAL));
}    


int main (int argc, char *argv[])
{
    pthread_t rpc_thread_id;
    int rc;
    static swdiag_unix_info_t swdiag_info;

    if (argc == 2) {
        /*
         * Should be the master, verify.
         */
        if (strcmp(argv[1], "-m") != 0) {
            fprintf(stderr, "Usage: %s [-m | -s <host>]\n", argv[0]);
            exit(1);
        }
        swdiag_info.host = NULL;
        swdiag_info.master = TRUE;
    } else if (argc == 3) {
        if (strcmp(argv[1], "-s") != 0) {
            fprintf(stderr, "Usage: %s [-m | -s <host>]\n", argv[0]);
            exit(1);
        }
        swdiag_info.host = strdup(argv[2]);
        swdiag_info.master = FALSE;
    } else {
        fprintf(stderr, "Usage: %s [-m | -s <host>]\n", argv[0]);
        exit(1);
    }

    swdiag_sched_initialize();

    if (!swdiag_create_instance(&swdiag_info)) {
        fprintf(stderr, "Could not register this SW Diags instance " 
                "with the master\n");
        exit(1);
    }

    /*
     * Start the RPC thread, then go into our main SW Diags loop.
     */
    rc = pthread_create(&rpc_thread_id, NULL, swdiag_rpc_thread, 
                        &swdiag_info);

    if (rc) {
        fprintf(stderr, "Could not create RPC thread, rc = %d", rc);
        exit(1);
    }

    /*
     * Tell Ha Diags whether we are a master or slave
     */
    if (swdiag_info.master) {
        swdiag_set_master();
    } else {
        /*
         * Before informing SW Diags that we are a slave, store some context
         * in the system component so that we know how to contact the master.
         */
        swdiag_api_comp_set_context(SWDIAG_SYSTEM_COMP, &swdiag_info);
        swdiag_set_slave(swdiag_info.my_name);
    }
    pthread_exit(NULL);
    return(0);
}

