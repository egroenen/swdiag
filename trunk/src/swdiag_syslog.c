/* 
 * swdiag_syslog.c - SW Diagnostics Unix syslog interface
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
 * Interface to the Unix syslog facility for traceability logging
 * from SW Diagnostics.
 */
#include "swdiag_xos.h"
#include "swdiag_trace.h"
#include <syslog.h>

void swdiag_xos_trace (trace_event_t *event)
{
    static boolean initialised = FALSE;

    if (!initialised) {
        openlog("SWDiags", (LOG_ODELAY | LOG_PID), LOG_LOCAL5);
        initialised = TRUE;
    }

    switch(event->type) {
    case TRACE_STRING:
        //syslog(LOG_INFO, event->string);
        //printf("Info: %s\n", event->string);
        break;
    case TRACE_ERROR:
        syslog(LOG_ERR, event->string);
        printf("Error: %s\n", event->string);
        break;
    default:
    }
}
