/*
 *  server.c
 *  libcurie
 *
 *  Created by Magnus Deininger on 24/08/2008.
 *  Copyright 2008 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, Magnus Deininger All rights reserved.
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

#include <curie/multiplex.h>
#include <curie/network.h>

#include <duat/9p.h>

#include <curie/main.h>
#include <curie/memory.h>

static void Tattach (struct duat_9p_io *io, int_16 tag, int_32 fid, int_32 afid,
                     char *uname, char *aname)
{
    struct duat_9p_qid qid = { 0, 0, 0 };

    duat_9p_reply_attach (io, tag, qid);
}

static void Twalk (struct duat_9p_io *io, int_16 tag, int_32 fid, int_32 afid,
                   int_16 c, char **names)
{
    struct duat_9p_qid qid[c];
    int_16 i = 0;

    while (i < c) {
        qid[i].type    = 0;
        qid[i].version = 0;
        qid[i].path    = 0;
        i++;
    }

    duat_9p_reply_walk (io, tag, c, qid);
}

static void Tstat (struct duat_9p_io *io, int_16 tag, int_32 fid)
{
    struct duat_9p_fid_metadata *md = duat_9p_fid_metadata (io, fid);

    if (md->path_count == 0) {
        struct duat_9p_qid qid = { QTDIR, 1, 2 };

        duat_9p_reply_stat (io, tag, 1, 1, qid,
                            DMDIR | DMUREAD | DMOREAD | DMGREAD,
                            1, 1, 2,
                            "/", "nyu", "kittens", "nyu", (char *)0);
    } else {
        struct duat_9p_qid qid = { 0, 1, 2 };

        duat_9p_reply_stat (io, tag, 1, 1, qid,
                            DMUREAD | DMOREAD | DMGREAD,
                            1, 1, 800,
                            "nyoron", "nyu", "kittens", "nyu", (char *)0);
    }
}

static void Topen (struct duat_9p_io *io, int_16 tag, int_32 fid, int_8 mode)
{
    struct duat_9p_qid qid = { 0, 1, 2 };

    duat_9p_reply_open (io, tag, qid, 0x1000);
}

static void Tcreate (struct duat_9p_io *io, int_16 tag, int_32 fid, char *name, int_32 perm, int_8 mode)
{
    struct duat_9p_qid qid = { 0, 1, 2 };

    duat_9p_reply_create (io, tag, qid, 0x1000);
}

static void Tread (struct duat_9p_io *io, int_16 tag, int_32 fid, int_64 offset, int_32 length)
{
    struct duat_9p_fid_metadata *md = duat_9p_fid_metadata (io, fid);

    if (md->path_count == 0) {
        struct duat_9p_qid qid = { 0, 1, 2 };
        int_8 *bb;
        int_16 slen = 0;

        if (md->index == 0) {
            slen = duat_9p_prepare_stat_buffer
                    (io, &bb, 1, 1, &qid, DMUREAD | DMOREAD | DMGREAD,
                     1, 1, 6, "nyoron", "nyu", "kittens", "nyu", (char *)0);
            duat_9p_reply_read (io, tag, slen, bb);
        } else if (md->index == 1) {
            slen = duat_9p_prepare_stat_buffer
                    (io, &bb, 1, 1, &qid, DMUREAD | DMOREAD | DMGREAD,
                     1, 1, 6, "nyoronZ", "nyu", "kittens", "nyu", (char *)0);
            duat_9p_reply_read (io, tag, slen, bb);
        } else {
            duat_9p_reply_read (io, tag, 0, (int_8 *)0);
        }
        (md->index)++;
    } else {
        if (offset == 0) {
            duat_9p_reply_read (io, tag, 6, (int_8 *)"meow!\n");
        } else {
            duat_9p_reply_read (io, tag, 0, (int_8 *)0);
        }
    }
}

static void Twrite (struct duat_9p_io *io, int_16 tag, int_32 fid, int_64 offset, int_32 count, int_8 *data)
{
    char dd[count + 1];
    int i;

    for (i = 0; i < count; i++) {
        dd[i] = (char)data[i];
    }
    dd[i] = 0;

    duat_9p_reply_write (io, tag, count);
}

void on_connect(struct io *in, struct io *out, void *p) {
    struct duat_9p_io *io = duat_open_io (in, out);

    io->Tattach = Tattach;
    io->Twalk   = Twalk;
    io->Tstat   = Tstat;
    io->Topen   = Topen;
    io->Tcreate = Tcreate;
    io->Tread   = Tread;
    io->Twrite  = Twrite;

    multiplex_add_duat_9p (io, (void *)0);
}

static void *rm_recover(unsigned long int s, void *c, unsigned long int l)
{
    a_exit(22);
    return (void *)0;
}

static void *gm_recover(unsigned long int s)
{
    a_exit(23);
    return (void *)0;
}

int a_main(void) {
    set_resize_mem_recovery_function(rm_recover);
    set_get_mem_recovery_function(gm_recover);

    duat_9p_update_user  ("nyu",     1000);
    duat_9p_update_group ("kittens", 100);

    multiplex_network();
    multiplex_duat_9p();
    multiplex_add_socket ("./test-socket-9p", on_connect, (void *)0);

    while (multiplex() != mx_nothing_to_do);

    return 0;
}
