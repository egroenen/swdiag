/* 
 * swdiag_unix_rpc_slave.c
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
 * Unix Sun RPC Server functions for the HA Diags Slave
 */
#include "swdiag_unix_rpc.h"
#include "swdiag_client.h"
#include "swdiag_unix_clients.h"
#include "swdiag_cli.h"
#include "swdiag_trace.h"

/*
 * swdiag_get_info_1_svc()
 *
 * Got a CLI request for some info, pass it on to the HA Diags CLI
 * handler to get a handle for further requests.
 */
bool_t swdiag_get_slave_info_handle_1_svc (int type, 
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
bool_t swdiag_get_slave_info_1_svc (unsigned int handle, 
                                    swdiag_cli_info_rsp_t *rsp, 
                                    struct svc_req *req)
{
    cli_info_t *cli_data;
    cli_info_element_t *cli_element, *old_element;
    swdiag_cli_info_element_rpc_t *rpc_element;
    int i;
    int num_elements;
    int count = 0;

    cli_data = swdiag_cli_get_info(handle);

    rsp->swdiag_cli_info_rsp_t_val = NULL;

    if (cli_data) {
        /*
         * Repackage the CLI data in our RPC response.
         */
        num_elements = cli_data->num_elements;

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
                }
            }
        } 
        free(cli_data);
    }

    rsp->swdiag_cli_info_rsp_t_len = count;

    return(TRUE);
}

#if 0
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
#endif 

/*
 * swdiag_cli_get_info_handle_remote()
 * 
 * API called on the master to obtain the CLi info using the slave protocol
 * from a slave HA Diags instance.
 */
unsigned int swdiag_cli_get_info_handle_remote (const char *comp_name, 
                                                cli_type_t type,
                                                cli_type_filter_t filter)
{
    const swdiag_unix_info_t *client_info;
    unsigned int handle = 0;

    client_info = swdiag_api_comp_get_context(comp_name);

    if (client_info && client_info->clnt) {
        if (swdiag_get_slave_info_handle_1(type, &handle, 
                                           client_info->clnt) != RPC_SUCCESS) {
            swdiag_error("Could not obtain CLI handle from slave '%s'", 
                         comp_name);
        }
    }

    return(handle);
}

/*
 * swdiag_cli_get_info_remote()
 *
 * Use Sun RPC over the slave protocol to get the CLI info from the
 * remote HA Diags instance, reformat that data into a form that 
 * HA Diags knows.
 */
cli_info_t *swdiag_cli_remote_get_info (const char *comp_name, 
                                        unsigned int handle) 
{
    cli_info_t *cli_info = NULL;
    static swdiag_cli_info_rsp_t rsp;
    swdiag_cli_info_element_rpc_t *rpc_element;
    const swdiag_unix_info_t *client_info;
    cli_info_element_t *element, *last_element;
    int count = 0;

    client_info = swdiag_api_comp_get_context(comp_name);

    cli_info = malloc(sizeof(cli_info_t));

    if (cli_info == NULL) {
        return(NULL);
    }

    if (handle == 0) {
        cli_info->num_elements = 0;
        swdiag_error("%s: Invalid handle", __FUNCTION__);
        return(cli_info);
    }

    if (!client_info || !client_info->clnt) {
        cli_info->num_elements = 0;
        swdiag_error("%s: Invalid component context for component '%s'", 
                     __FUNCTION__, comp_name);
        return(cli_info);
    }

    rsp.swdiag_cli_info_rsp_t_len = -1;
    rsp.swdiag_cli_info_rsp_t_val = NULL;
    while(rsp.swdiag_cli_info_rsp_t_len != 0) {
        if (swdiag_get_slave_info_1(handle, &rsp, client_info->clnt) == RPC_SUCCESS) {
            int i;  
            count = 0;
            cli_info->elements = NULL;
            last_element = NULL;
            rpc_element = rsp.swdiag_cli_info_rsp_t_val;
            for (i=0; i < rsp.swdiag_cli_info_rsp_t_len; i++) {
                /*
                 * Repackage 
                 */
                element = malloc(sizeof(cli_info_element_t));
                element->type = rpc_element[i].type;
                element->name = strdup(swdiag_api_make_name(
                                           comp_name,
                                           rpc_element[i].name));
                element->last_result_count = rpc_element[i].last_result_count;
                element->stats.failures = rpc_element[i].failures;
                element->stats.aborts = rpc_element[i].aborts;
                element->stats.passes = rpc_element[i].passes;
                element->stats.runs = rpc_element[i].runs;
                element->health = rpc_element[i].health;
                element->confidence = rpc_element[i].confidence;

                if (!cli_info->elements) {
                    cli_info->elements = element;
                }
                if (last_element) {
                    last_element->next = element;
                }
                last_element = element;

                count++;
            }
            cli_info->num_elements = count;
            break;
        }
    }
    return(cli_info);
}
