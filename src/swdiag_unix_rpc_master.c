/* 
 * swdiag_unix_rpc_master.c
 *
 * Copyright (c) 2007-2009 Cisco Systems Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Unix Sun RPC Server functions for the HA Diags Master
 */
#include "swdiag_unix_rpc.h"
#include "swdiag_client.h"
#include "swdiag_unix_clients.h"
#include "swdiag_cli.h"
#include "swdiag_cli_local.h"
#include "swdiag_api.h"
#include "swdiag_obj.h"

static swdiag_result_t poll_slave (const char *instance,
                                   void *context, long *retval)
{
    swdiag_unix_info_t *client_info = (swdiag_unix_info_t *)context;
    int health = 0;

    if (!client_info) {
        printf("%s: Aborting test, no client info\n", __FUNCTION__);
        return(SWDIAG_RESULT_ABORT);
    }

    if (!client_info->clnt) {
        /*
         * Setup communications with the remote slave
         */
        client_info->clnt = clnt_create(client_info->host, 
                                        client_info->rpc_instance,
                                        SWDIAG_SLAVE_PROTOCOL_V1,
                                        "tcp");

        if (!client_info->clnt) {
            fprintf(stderr, "Could not contact slave at %s:0x%x\n",
                    client_info->host,
                    client_info->rpc_instance);
            return(SWDIAG_RESULT_ABORT);
        }
    }

    if (swdiag_get_health_1(&health, client_info->clnt) != RPC_SUCCESS) {
        fprintf(stderr, "Could not get the health\n");
        return(SWDIAG_RESULT_FAIL);
    }

    swdiag_health_set(client_info->my_name, health);

    return(SWDIAG_RESULT_PASS);
}

/*
 * remove_slave()
 *
 * Slave HA Diags has gone down, remove the component.
 */
static swdiag_result_t remove_slave (const char *instance, void *context)
{
    const char *test_name;
    const char *action_name;
    const char *rule_name;
    swdiag_unix_info_t *client_info = context;
    char *component;

    if (!client_info) {
        return(SWDIAG_RESULT_FAIL);
    }
    
    component = client_info->my_name;
    test_name = swdiag_api_make_name("Poll", component);
    action_name = swdiag_api_make_name("Remove Slave", component);
    rule_name = swdiag_api_make_name("Health Check", component);
    
    swdiag_api_comp_set_context(component,
                                NULL);

    swdiag_test_delete(test_name);
    swdiag_action_delete(action_name);
    swdiag_rule_delete(rule_name);
    swdiag_comp_delete(component);

    free(client_info->host);
    free(client_info->my_name);
    if (client_info->clnt) {
        clnt_destroy(client_info->clnt);
        client_info->clnt = NULL;
    }

    free(client_info);

    return(SWDIAG_RESULT_PASS);
}

/*
 * swdiag_register_1_svc()
 *
 * Received an incoming request to register a ha diags client with this
 * ha diags master.
 */
bool_t swdiag_register_1_svc (swdiag_register_t register_msg, 
                             int *retval, 
                             struct svc_req *req)
{
    char *component = register_msg.entity;
    const char *test_name;
    const char *action_name;
    swdiag_unix_info_t *client_info;
    char *component_name_copy;
    obj_t *comp_obj;

    /*
     * Create the component for the remote entity
     */
    swdiag_comp_create(component);

    client_info = malloc(sizeof(swdiag_unix_info_t));

    if (!client_info) {
        *retval = FALSE;
        return(TRUE);
    }

    client_info->host = strdup(register_msg.hostname);
    client_info->my_name = strdup(component);
    client_info->master = FALSE;
    client_info->rpc_instance = register_msg.instance;
    client_info->clnt = NULL;

    /*
     * And now we need to associate the client id with that component
     * so that we can communicate with it. We also need to register the
     * fact that it is a remote entity so we know to use a remote access
     * to contact it.
     */
    swdiag_comp_set_context(component,
                            client_info);

    test_name = swdiag_api_make_name("Poll", component);

    swdiag_test_create_polled(test_name,
                              poll_slave,
                              client_info,
                              1000);
    
    action_name = swdiag_api_make_name("Remove Slave", component);
    
    swdiag_action_create(action_name,
                         remove_slave,
                         client_info);
    
    swdiag_rule_create(swdiag_api_make_name("Health Check", component),
                       test_name,
                       action_name);
    
    swdiag_comp_contains_many(component,
                              test_name,
                              action_name,
                              swdiag_api_make_name("Health Check", component),
                              NULL);

    swdiag_comp_enable(component);
    
    *retval = TRUE;

    return(TRUE);
}

/*
 * swdiag_get_info_1_svc()
 *
 * Got a CLI request for some info, pass it on to the HA Diags CLI
 * handler to get a handle for further requests.
 */
bool_t swdiag_get_info_handle_1_svc (int type, 
                                     unsigned int *handle, 
                                     struct svc_req *req)
{

    *handle = swdiag_cli_get_info_handle(NULL, (cli_type_t) type,
                                         CLI_FILTER_NONE, NULL);
    return(TRUE);
}

/*
 * swdiag_get_info_1_svc()
 *
 * Get the next batch of info associated with this handle and package it
 * in a response message.
 */
bool_t swdiag_get_info_1_svc (unsigned int handle, 
                              swdiag_cli_info_rsp_t *rsp, 
                              struct svc_req *req)
{
    cli_info_t *cli_data = NULL;
    cli_info_element_t *cli_element, *old_element;
    swdiag_cli_info_element_rpc_t *rpc_element;
    int i;
    int num_elements;
    int count = 0;

    cli_data = swdiag_cli_get_info(handle);

    if (cli_data) {
        /*
         * Repackage the CLI data in our RPC response.
         */
        num_elements = cli_data->num_elements;
        
        rsp->swdiag_cli_info_rsp_t_val = NULL;

        if (num_elements) {
            rsp->swdiag_cli_info_rsp_t_val =
                malloc(sizeof(swdiag_cli_info_element_rpc_t) * num_elements);
            
            if (rsp->swdiag_cli_info_rsp_t_val) {

                cli_element = cli_data->elements;

                rpc_element = rsp->swdiag_cli_info_rsp_t_val;

                for(i=0; i < num_elements; i++) {
                    rpc_element[i].type = cli_element->type;
                    rpc_element[i].name = strdup(cli_element->name);
                    rpc_element[i].last_result = cli_element->last_result;
                    rpc_element[i].last_result_count = 
                        cli_element->last_result_count;
                    rpc_element[i].failures = cli_element->stats.failures;
                    rpc_element[i].aborts = cli_element->stats.aborts;
                    rpc_element[i].passes = cli_element->stats.passes;
                    rpc_element[i].runs = cli_element->stats.runs;
                    rpc_element[i].state = cli_element->state;
                    rpc_element[i].default_state = cli_element->default_state;
                    rpc_element[i].health = cli_element->health;
                    rpc_element[i].confidence = cli_element->confidence;
                    rpc_element[i].severity = cli_element->severity;

                    old_element = cli_element;
                    cli_element = cli_element->next;
                    free(old_element);
                    count++;
                    if (!cli_element) {
                        break;
                    }
                    //rpc_element += sizeof(swdiag_cli_info_element_rpc_t);
                }
            }
        } 
        free(cli_data);
    }

    rsp->swdiag_cli_info_rsp_t_len = count;

    return(TRUE);
}

/*
 * swdiag_remote_master_protocol_1_freeresult()
 *
 * Called on the RPC Server for all of packets that we replied with
 */
int swdiag_remote_master_protocol_1_freeresult (SVCXPRT *transp, 
                                               xdrproc_t xdr_result, 
                                               caddr_t result)
{
    (void) xdr_free(xdr_result, result);

    return (TRUE);
}

bool_t swdiag_get_health_1_svc (int *retval, struct svc_req *req)
{
    /*
     * Get the system health
     */
    *retval = swdiag_health_get(SWDIAG_SYSTEM_COMP);
    
    return (TRUE);
}

int swdiag_remote_slave_protocol_1_freeresult (SVCXPRT *transp, 
                                              xdrproc_t xdr_result, 
                                              caddr_t result)
{
    (void) xdr_free(xdr_result, result);

    /*
     * Insert additional freeing code here, if needed
     */

    return (TRUE);
}
