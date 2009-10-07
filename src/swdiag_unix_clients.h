/* 
 * swdiag_unix_clients.h
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
 * Header file defining the Unix info on whether we are a client and
 * the different client ids that we may be using.
 *
 * TODO: Rewrite with new networking API, this file is RPC specific.
 */

#ifndef __SWDIAG_UNIX_H__
#define __SWDIAG_UNIX_H__

#include <rpc/rpc.h>
#include "../src/swdiag_xos.h"

/*
 * Keep track of whether this instance is the master instance, or whether
 * it is a slave instance which must connect with a master.
 */
typedef struct swdiag_unix_info_ {
    char *host;
    char *my_name;
    boolean master;
    int rpc_instance;
    CLIENT *clnt;
} swdiag_unix_info_t;

enum swdiag_clients {
    SWDIAG_MASTER_ID = 0x3000F002,
    SWDIAG_DAEMON_ID = 0x3000F003,
    SWDIAG_TEST_CLIENT_ID = 0x3000F004,
};

boolean swdiag_create_instance(swdiag_unix_info_t *info);
void *swdiag_rpc_thread (void *thread_context);

#endif
