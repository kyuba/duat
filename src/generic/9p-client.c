/*
 *  9p-client.c
 *  libduat
 *
 *  Created by Magnus Deininger on 31/01/2009.
 *  Copyright 2009 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2009, Magnus Deininger All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. *
 * Neither the name of the project nor the names of its contributors may
 * be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <curie/memory.h>
#include <curie/multiplex.h>
#include <curie/network.h>
#include <duat/9p-client.h>

#define ROOT_FID 1

enum d9c_status_code
{
    d9c_attaching,
    d9c_walking,
    d9c_opening,
    d9c_writing,
    d9c_reading,
    d9c_closing,
    d9c_ready,
    d9c_error
};

struct d9c_status
{
    enum d9c_status_code code;
    int_32 root_fid;
};

static void Rattach (struct d9r_io *io, int_16 tag, struct d9r_qid qid)
{
    struct d9c_status *status = (struct d9c_status *)(io->aux);

    status->code = d9c_ready;
}

/*

static void Rerror  (struct d9r_io *, int_16, char *, int_16);
static void Rflush  (struct d9r_io *, int_16);
static void Rwalk   (struct d9r_io *, int_16, int_16, struct d9r_qid *);
static void Ropen   (struct d9r_io *, int_16, struct d9r_qid, int_32);
static void Rcreate (struct d9r_io *, int_16, struct d9r_qid, int_32);
static void Rread   (struct d9r_io *, int_16, int_32, int_8 *);
static void Rwrite  (struct d9r_io *, int_16, int_32);
static void Rclunk  (struct d9r_io *, int_16);
static void Rremove (struct d9r_io *, int_16);
static void Rstat   (struct d9r_io *, int_16, int_16, int_32,
           struct d9r_qid, int_32, int_32, int_32, int_64, char *,
           char *, char *, char *, char *);
static void Rwstat  (struct d9r_io *, int_16);

*/

void multiplex_d9c ()
{
    static char initialised = 0;

    if (initialised == (char)0)
    {
        multiplex_io();
        multiplex_network();
        multiplex_d9r();

        initialised = (char)1;
    }
}

static struct d9r_io *initialise_io (struct d9r_io *io)
{
    struct memory_pool pool
            = MEMORY_POOL_INITIALISER (sizeof (struct d9c_status));

    struct d9c_status *status = get_pool_mem (&pool);

    if (status == (struct d9c_status *)0)
    {
        d9r_close_io (io);
        return (struct d9r_io *)0;
    }

    status->code = d9c_attaching;

    io->aux = (void *)status;

    io->Rattach = Rattach;
/*    io->Rwalk   = Rwalk;
    io->Rstat   = Rstat;
    io->Ropen   = Ropen;
    io->Rcreate = Rcreate;
    io->Rread   = Rread;
    io->Rwrite  = Rwrite;
    io->Rstat   = Rstat;
    io->Rwstat  = Rwstat;*/

    multiplex_add_d9r (io, (void *)0);

    d9r_version (io, 0x2000, "9P2000");
    d9r_attach  (io, ROOT_FID, NO_FID_9P, "none", "none");

    return io;
}

struct d9r_io *multiplex_add_d9c_socket (const char *socket)
{
    struct io *in;
    struct io *out;

    net_open_socket (socket, &in, &out);

    if ((in == (struct io *)0) || (out == (struct io *)0))
    {
        if (in != (struct io *)0)
        {
            io_close (in);
        }

        if (out != (struct io *)0)
        {
            io_close (in);
        }

        return (struct d9r_io *)0;
    }

    return multiplex_add_d9c_io  (in, out);
}

struct d9r_io *multiplex_add_d9c_io (struct io *in, struct io *out)
{
    struct d9r_io *io = d9r_open_io(in, out);

    if (io == (struct d9r_io *)0) return (struct d9r_io *)0;

    return initialise_io (io);
}

struct d9r_io *multiplex_add_d9c_stdio ()
{
    struct d9r_io *io = d9r_open_stdio();

    if (io == (struct d9r_io *)0) return (struct d9r_io *)0;

    return initialise_io (io);
}
