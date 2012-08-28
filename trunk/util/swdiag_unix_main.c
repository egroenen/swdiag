/* 
 * swdiag_unix_main.c
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
 * Multithreaded Cisco SW Diagnostics server
 *
 * Requires two threads, one for servicing RPC's and one for SW Diags
 * itself.
 *
 * NOTE that this is all going to change.
 *
 * We need to dameonize properly and safely, and we need an alternative
 * reliable, secure communications mechanism that we can hook into the
 * swdiag CLI API.
 *
 * Ideally we want something that listens on a port, so that we can
 * talk with other swdiag instances on remote machines.
 *
 * I'm thinking that JSON is the best one to use at the moment, SOAP
 * is too bloated. It will also enable easy integration with webapps.
 * The authentication can be handled by HTTP on the outside.
 *
 * For the JSON parsing we can use jansson, and on the client side
 * libcurl for the communications. On the server side we will
 * use mongoose as the SSL web server through which we will pump JSON
 * from jansson.
 */
#include "swdiag_client.h"
#include "swdiag_sched.h"
#include <pthread.h>
#include "swdiag_unix_clients.h"
//#include "swdiag_unix_rpc.h"
#include "swdiag_cli.h"
#include "swdiag_cli_local.h"
#include "swdiag_api.h"

#include "mongoose/mongoose.h"


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
    //CLIENT *clnt;
    //swdiag_register_t register_info;
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
    /*swdiag_info->clnt = clnt_create(swdiag_info->host,
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
    }*/

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

/*
 * Simple hello world callback. In reality we need to have an authorise callback
 * that will authorise the connection and setup a session. That session is then
 * returned in a cookie. That cookie is used for subsequent requests.
 *
 * We can use the CLI handle as the session, two birds with one stone.
 */
static void *callback(enum mg_event event,
                      struct mg_connection *conn) {
  const struct mg_request_info *request_info = mg_get_request_info(conn);

  if (event == MG_NEW_REQUEST) {
    char content[1024];
    int content_length = snprintf(content, sizeof(content),
                                  "Hello from mongoose! Remote port: %d",
                                  request_info->remote_port);
    mg_printf(conn,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: %d\r\n"        // Always set Content-Length
              "\r\n"
              "%s",
              content_length, content);
    // Mark as processed
    return "";
  } else {
    return NULL;
  }
}


int main (int argc, char *argv[])
{
    pthread_t rpc_thread_id;
    int rc;
    static swdiag_unix_info_t swdiag_info;

    swdiag_debug_enable();

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
    swdiag_debug_enable();
    if (!swdiag_create_instance(&swdiag_info)) {
        fprintf(stderr, "Could not register this SW Diags instance " 
                "with the master\n");
        exit(1);
    }

    /*
     * Start the RPC thread, then go into our main SW Diags loop.
     */
    /*rc = pthread_create(&rpc_thread_id, NULL, swdiag_rpc_thread,
                        &swdiag_info);

    if (rc) {
        fprintf(stderr, "Could not create RPC thread, rc = %d", rc);
        exit(1);
    }
*/

    /*
     * Start the embedded monsoon web server
     */
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", "8080", NULL};

    ctx = mg_start(&callback, NULL, options);

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

