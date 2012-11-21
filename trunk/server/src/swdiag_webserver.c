/*
 * swdiag_json_parser.c - SW Diagnostics JSON web server
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

#include "mongoose/mongoose.h"
#include "swdiag_cli.h"
#include "swdiag_cli_local.h"

#include "swdiag_server_config.h"

/*
 * Simple hello world callback. In reality we need to have an authorise callback
 * that will authorise the connection and setup a session. That session is then
 * returned in a cookie. That cookie is used for subsequent requests.
 *
 * We can use the CLI handle as the session, two birds with one stone.
 *
 * This is returning text for now, but as soon as the PrimUI is available will
 * return JSON so that the web interface can display the results in datatables
 * etc.
 */
static void *https_request_callback(enum mg_event event,
        struct mg_connection *conn) {

    void *processed = "yes";
    const struct mg_request_info *request_info = mg_get_request_info(conn);

    if (event == MG_NEW_REQUEST) {
        char content[1024];
        int content_length = 0;
        cli_info_element_t *element;
        cli_type_t type;

        if (strcmp(request_info->uri, "/tabcontent/1") == 0) {
            type = CLI_COMPONENT;
        } else if (strcmp(request_info->uri, "/tabcontent/2") == 0) {
            type = CLI_TEST;
        } else if (strcmp(request_info->uri, "/tabcontent/3") == 0) {
            type = CLI_RULE;
        } else if (strcmp(request_info->uri, "/tabcontent/4") == 0) {
            type = CLI_ACTION;
        } else {
            type = CLI_UNKNOWN;
        }

        if (type != CLI_UNKNOWN) {
            unsigned int handle = swdiag_cli_local_get_info_handle(NULL, type,
                    CLI_FILTER_NONE, NULL);
            // Now use the handle to get the actual information.

            if (handle != 0) {
                cli_info_t *info = swdiag_cli_local_get_info(handle, MAX_LOCAL);

                if (info != NULL) {
                    element = info->elements;
                    switch(element->type) {
                    case CLI_COMPONENT:
                        content_length += snprintf(content + content_length, sizeof(content),
                                    "                         Health \n"
                                    "                Name   Now/Conf    Runs Passes  Fails\n");
                        while(element != NULL) {
                            content_length += snprintf(content + content_length, sizeof(content),
                                    "%20s %s%5.1f/%-5.1f%s %6d %6d %6d\n", element->name, (element->health/10 < 100) ? "<span style=\"color:red;\">" : "", element->health/10.0, element->confidence/10.0, element->health/10 < 100 ? "</span>" : "", element->stats.runs, element->stats.passes, element->stats.failures);
                            element = element->next;
                        }
                        break;
                    case CLI_TEST:
                        while(element != NULL) {
                            content_length += snprintf(content + content_length, sizeof(content), "Test %s %s %d %d %d\n", element->name, swdiag_cli_state_to_str(element->state), element->stats.runs, element->stats.passes, element->stats.failures);
                            element = element->next;
                        }
                        break;
                    case CLI_RULE:
                        while(element != NULL) {
                            content_length += snprintf(content + content_length, sizeof(content), "Rule %s %d %d %d\n", element->name, element->stats.runs, element->stats.passes, element->stats.failures);
                            element = element->next;
                        }
                        break;
                    case CLI_ACTION:
                        while(element != NULL) {
                            content_length += snprintf(content + content_length, sizeof(content), "Action %s %d %d %d\n", element->name, element->stats.runs, element->stats.passes, element->stats.failures);
                            element = element->next;
                        }
                        break;
                    }
                    content_length += 12; // For pre's below
                    mg_printf(conn,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: %d\r\n"        // Always set Content-Length
                            "\r\n"
                            "<pre>%s</pre>",
                            content_length, content);
                    free(info);
                }
            }
        } else {
            processed = NULL;
        }
    } else {
        // Not processed, let mongoose handle it (e.g. a static page).
        processed = NULL;;
    }

    return processed;
}

boolean swdiag_webserver_start() {
    /*
     * Start the embedded monsoon web server running non-SSL (for now) on 7654
     */
    struct mg_context *ctx;
    static const char *options[] = {"listening_ports", server_config.http_port,
                                    "document_root", server_config.http_root,
                                    "num_threads", "5",
                                    NULL};

    ctx = mg_start(&https_request_callback, NULL, options);

    if (!ctx) {
        return FALSE;
    }
    return TRUE;
}
