/*
 * swdiag_server_exec.c - SW Diagnostics Server Exec functions
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

/**
 * Functions that swdiag execs that in turn call the swdiag server
 * modules to exec a test or action.
 */

#include "swdiag_xos.h"
#include "swdiag_client.h"
#include "swdiag_trace.h"
#include "swdiag_server_module.h"
#include "swdiag_server_config.h"
#include "smtpfuncs.h"
#include <dirent.h>
#include <errno.h>

int moduleCount = 0;
static char **modules_;
static char *modules_path_;

// Maximum size for a modules configuration
#define MAXBUFLEN (1024 * 10)

int EndsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void modules_init(char *modules_path) {
    DIR *d;
    struct dirent *dir;
    modules_path_ = modules_path;

    d = opendir(modules_path_);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char *filename = dir->d_name;
            if (*filename != '.' && !EndsWith(filename, "~") && !EndsWith(filename, ".conf") && !EndsWith(filename, "_conf.py")) {
                moduleCount++;
            }
        }
        closedir(d);
    }

    if (moduleCount > 0) {
        modules_ = (char**) calloc(moduleCount, sizeof(void*));
        moduleCount = 0;
        d = opendir(modules_path_);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                char *filename = dir->d_name;
                if (*filename != '.' && !EndsWith(filename, "~") && !EndsWith(filename, ".conf") && !EndsWith(filename, "_conf.py")) {
                    modules_[moduleCount++] = strdup(filename);
                    swdiag_debug(NULL, "Added MODULE '%s'", modules_[moduleCount-1]);
                }
            }
            closedir(d);
        }
    }
}

boolean modules_process_config() {
    boolean ret = FALSE;
    int i;
    if (modules_ != NULL)  {
        for (i = 0; i < moduleCount; i++) {
            swdiag_debug(NULL, "Processing configuration for MODULE '%s'", modules_[i]);
            char configuration[MAXBUFLEN + 1];
            char *filename = malloc(strlen(modules_path_) + strlen(modules_[i]) + 7);
            strcpy(filename, modules_path_);
            strcat(filename, "/");
            strcat(filename, modules_[i]);
            strcat(filename, " --conf");
            swdiag_debug(NULL, "MODULE path: %s", filename);
            FILE *fp = popen(filename, "r");
            if (fp != NULL) {
                size_t newLen = fread(configuration, sizeof(char), MAXBUFLEN, fp);
                if (newLen == 0) {
                    fprintf(stderr, "Error: empty configuration for module file '%s'\n", filename);
                    break;
                } else {
                    configuration[++newLen] = '\0'; /* Just to be safe. */
                }
                if (pclose(fp) == 0) {
                    // Parse and process the configuration.
                    char request[MAXBUFLEN + 1];
                    snprintf(request, MAXBUFLEN, "\"configuration\":{%s}", configuration);
                    ret = process_json_request(modules_[i], request, NULL);
                }
            }
        }
    }
    return ret;
}
/**
 * The context contains a string with the name of the module in it, the instance may or may not
 * contain the instance name, and the value is there in case any value that can be converted into
 * a number is returned from the test.
 */
swdiag_result_t swdiag_server_exec_test(const char *instance, void *context, long *value) {
    swdiag_result_t result = SWDIAG_RESULT_ABORT;
    test_context *testcontext = (test_context*)context;
    int rc;;

    swdiag_debug(NULL, "Module %s: Test %s instance %s is being run", testcontext->module_name, testcontext->test_name, instance);
    char *test_results = calloc(MAXBUFLEN, sizeof(char));
    char *filename = malloc(strlen(modules_path_) + MAXBUFLEN);
    strcpy(filename, modules_path_);
    strcat(filename, "/");
    strcat(filename, testcontext->module_name);
    strcat(filename, " --test ");
    strcat(filename, testcontext->test_name);
    if (instance != NULL) {
        strcat(filename, " --instance ");
        strcat(filename, instance);
    }
    swdiag_debug(NULL, "MODULE path: %s", filename);
    FILE *fp = popen(filename, "r");
    if (fp != NULL) {
        size_t newLen = fread(test_results, sizeof(char), MAXBUFLEN, fp);
        if (newLen != 0) {
            test_results[++newLen] = '\0'; /* Just to be safe. */
        }
        pclose(fp);
        if (newLen != 0) {
            char *request = calloc(MAXBUFLEN, sizeof(char));
            if (request) {
                int len = snprintf(request, MAXBUFLEN, "\"results\":{%s}", test_results);
                if (process_json_request(testcontext->module_name, request, NULL)) {
                    result = SWDIAG_RESULT_IN_PROGRESS;
                } else {
                    result = SWDIAG_RESULT_ABORT;
                }
                free(request);
            }
        } else {
            result = SWDIAG_RESULT_ABORT;
        }
    }

    return result;
}

/**
 * The context contains a string with the name of the module in it, the instance may or may not
 * contain the instance name, and the value is there in case any value that can be converted into
 * a number is returned from the test.
 */
swdiag_result_t swdiag_server_exec_action(const char *instance, void *context) {
    swdiag_result_t result = SWDIAG_RESULT_ABORT;
    test_context *testcontext = (test_context*)context;
    int rc;;

    swdiag_debug(NULL, "Module %s: Test %s instance %s is being run", testcontext->module_name, testcontext->test_name, instance);
    char *test_results = calloc(MAXBUFLEN, sizeof(char));
    char *filename = malloc(strlen(modules_path_) + MAXBUFLEN);
    strcpy(filename, modules_path_);
    strcat(filename, "/");
    strcat(filename, testcontext->module_name);
    strcat(filename, " action ");
    strcat(filename, testcontext->test_name);
    if (instance != NULL) {
        strcat(filename, " ");
        strcat(filename, instance);
    }
    swdiag_debug(NULL, "MODULE path: %s", filename);
    FILE *fp = popen(filename, "r");
    if (fp != NULL) {
        size_t newLen = fread(test_results, sizeof(char), MAXBUFLEN, fp);
        if (newLen != 0) {
            test_results[++newLen] = '\0'; /* Just to be safe. */
        }
        pclose(fp);
        if (newLen != 0) {
            char *request = calloc(MAXBUFLEN, sizeof(char));
            if (request) {
                int len = snprintf(request, MAXBUFLEN, "\"results\":{%s}", test_results);
                process_json_request(testcontext->module_name, request, NULL);
                result = SWDIAG_RESULT_IN_PROGRESS;
                free(request);
            }
        } else {
            result = SWDIAG_RESULT_ABORT;
        }
    }

    return result;
}

/**
 * Alert the user using the swdiag server preferred mechanism with the message in context. We
 * are using this in preference to the swdiag_action_create_user_alert() for now since it allows
 * better control.
 */
swdiag_result_t swdiag_server_email(const char *instance, void *context) {
    swdiag_result_t result = SWDIAG_RESULT_PASS;
    email_context *email = (email_context*)context;

    if (!instance) {
        instance = "";
    }

    if (email) {
        char *to = email->to;
        if (*to == '\0') {
            to = server_config.alert_email_to;
        }

        /* I'm hardcoding the hostname to swdiag-server - it's not AFAIK important anyway. */
        send_mail(server_config.smtp_hostname, "swdiag-server", server_config.alert_email_from, to, email->subject, NULL, email->subject);
    }

    return result;
}
