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
#include <curie/exec.h>
#include <curie/main.h>

#include <duat/9p.h>

#include <curie/sexpr.h>

static void sx_stdio_write (struct sexpr *sx) {
    static struct sexpr_io *stdio = (struct sexpr_io *)0;

    if (stdio == (struct sexpr_io *)0) stdio = sx_open_stdio();

    sx_write (stdio, sx);
    sx_destroy (sx);
}

static void debug (char *t) {
    sx_stdio_write (cons(make_symbol("debug"), make_string(t)));
}

static void debug_num (int i) {
    sx_stdio_write (cons(make_symbol("debug"), make_integer(i)));
}



void Tattach (struct duat_9p_io *io, int_16 tag, int_32 fid, int_32 afid,
              char *uname, char *aname)
{
    struct duat_9p_qid qid = { 0, 0, 0 };

    duat_9p_reply_attach (io, tag, qid);
}

void Twalk (struct duat_9p_io *io, int_16 tag, int_32 fid, int_32 afid,
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

void on_connect(struct io *in, struct io *out, void *p) {
    struct duat_9p_io *io = duat_open_io (in, out);

    io->Tattach = Tattach;
    io->Twalk   = Twalk;

    multiplex_add_duat_9p (io, (void *)0);
}

int a_main(void) {
    multiplex_network();
    multiplex_duat_9p();
    multiplex_add_socket ("./test-socket-9p", on_connect, (void *)0);

    while (multiplex() != mx_nothing_to_do);

    return 0;
}
