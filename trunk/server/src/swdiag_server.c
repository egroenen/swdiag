/*
 * swdiag_server.c - SW Diagnostics Server
 *
 * Copyright (c) 2012 Edward Groenendaal
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
#include <pthread.h>
#include <getopt.h>

#include "swdiag_client.h"
#include "swdiag_sched.h"
#include "swdiag_cli.h"
#include "swdiag_cli_local.h"
#include "swdiag_api.h"

#include "swdiag_json_parser.h"
#include "mongoose/mongoose.h"

int debug_flag = 0;

/*
 * Using the swdiag library requires that some functions be implemented, we will
 * leave these as stubs for now.
 */
void swdiag_xos_register_with_master (const char *component)
{

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
static void *https_request_callback(enum mg_event event,
                                    struct mg_connection *conn) {

  const struct mg_request_info *request_info = mg_get_request_info(conn);

  if (event == MG_NEW_REQUEST) {
    char content[1024];
    int content_length = 0;
    cli_info_element_t *element;
    cli_type_t type;

    if (strcmp(request_info->uri, "/component") == 0) {
        type = CLI_COMPONENT;
    } else if (strcmp(request_info->uri, "/test") == 0) {
        type = CLI_TEST;
    } else if (strcmp(request_info->uri, "/rule") == 0) {
        type = CLI_RULE;
    } else if (strcmp(request_info->uri, "/action") == 0) {
        type = CLI_ACTION;
    } else {
        type = CLI_UNKNOWN;
    }

    if (type != CLI_UNKNOWN) {
        unsigned int handle = swdiag_cli_local_get_info_handle(NULL, type,
                                                               CLI_FILTER_NONE, NULL);
        // Now use the handle to get the actual test information.

        if (handle != 0) {
            cli_info_t *info = swdiag_cli_local_get_info(handle, MAX_LOCAL);

            if (info != NULL) {
                element = info->elements;
                while(element != NULL) {
                    content_length += snprintf(content + content_length, sizeof(content), "Test %s %d %d %d\n", element->name, element->stats.runs, element->stats.passes, element->stats.failures);
                    element = element->next;
                }
                mg_printf(conn,
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: %d\r\n"        // Always set Content-Length
                          "\r\n"
                          "%s",
                          content_length, content);
            }
        }
    }
    // Mark as processed
    return "";
  } else {
    return NULL;
  }
}

/**
 * The server
 */
int main (int argc, char **argv)
{
    pthread_t rpc_thread_id;
    int rc;
    char *modules_path = "/etc/swdiag/modules";
    int c;

    static struct option long_options[] = {
            {"debug", no_argument, &debug_flag, 1},
            {"modules", required_argument, 0, 'm'},
            {0,0,0,0}
    };

    while (1) {
        int option_index = 0;

        c = getopt_long(argc, argv, "m:", long_options, &option_index);

        if (c == -1)
            break;

        switch(c) {
        case 0:
            if (long_options[option_index].flag != 0)
                break;
            break;
        case 'm':
            modules_path = strdup(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-m <module path>] [--debug]\n", argv[0]);
            exit(1);
        }
    }

    if (debug_flag) {
        swdiag_debug_enable();
    }

    swdiag_sched_initialize();
    modules_init(modules_path);
    if (!modules_process_config()) {
        // Failed to read the configuration.
        fprintf(stderr, "ERROR: Failed to read the configuration, exiting.\n");
        exit(2);
    }

    /*
     * Start the embedded monsoon web server running non-SSL (for now) on 7654
     */
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", "7654", NULL};

    ctx = mg_start(&https_request_callback, NULL, options);

    //processJsonRequest("module", strdup("\"test\":{\"name\":\"parser_test\",\"polled\":true,\"interval\":3000}"), NULL);


    swdiag_set_master();


    //swdiag_api_comp_set_context(SWDIAG_SYSTEM_COMP, NULL);
    //swdiag_set_slave("slave");

    pthread_exit(NULL);
    return(0);
}




