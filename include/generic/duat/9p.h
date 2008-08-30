/*
 *  9p.h
 *  libduat
 *
 *  Created by Magnus Deininger on 21/08/2008.
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

#if !defined(DUAT_9P_H)
#define DUAT_9P_H

#include <curie/int.h>
#include <curie/io.h>

enum duat_9p_connection_version {
    duat_9p_version_9p2000       = 0,
    duat_9p_version_9p2000_dot_u = 1
};

#define NO_TAG_9P ((int_16)~0)

struct duat_9p_io {
    struct io *in, *out;

    enum duat_9p_connection_version version;
    int_16 max_message_size;

    void (*Tauth)   (int_16);
    void (*Tattach) (int_16);
    void (*Tflush)  (int_16);
    void (*Twalk)   (int_16);
    void (*Topen)   (int_16);
    void (*Tcreate) (int_16);
    void (*Tread)   (int_16);
    void (*Twrite)  (int_16);
    void (*Tclunk)  (int_16);
    void (*Tremove) (int_16);
    void (*Tstat)   (int_16);
    void (*Twstat)  (int_16);

    void (*Rauth)   ();
    void (*Rattach) ();
    void (*Rerror)  ();
    void (*Rflush)  ();
    void (*Rwalk)   ();
    void (*Ropen)   ();
    void (*Rcreate) ();
    void (*Rread)   ();
    void (*Rwrite)  ();
    void (*Rclunk)  ();
    void (*Rremove) ();
    void (*Rstat)   ();
    void (*Rwstat)  ();

    void *arbitrary;
};

struct duat_9p_qid {
    int_8  type;
    int_32 version;
    int_64 path;
};

struct duat_9p_io *duat_open_io (struct io *, struct io *);

#define duat_open_io_fd(in,out) duat_open_io (io_open ((in)), io_open((out)))
#define duat_open_stdio() duat_open_io_fd(0, 1)

void duat_close_io (struct duat_9p_io *);

void multiplex_duat_9p ();
void multiplex_add_duat_9p (struct duat_9p_io *, void *);

void duat_9p_auth   (struct duat_9p_io *, int_32, char *, char *);
void duat_9p_attach (struct duat_9p_io *, int_32, int_32, char *, char *);
void duat_9p_flush  (struct duat_9p_io *, int_16);
void duat_9p_walk   (struct duat_9p_io *, int_32, int_32, char **);
void duat_9p_open   (struct duat_9p_io *, int_32, int_8);
void duat_9p_create (struct duat_9p_io *, int_32, char *, int_32, int_8);
void duat_9p_read   (struct duat_9p_io *, int_32, int_64, int_32);
void duat_9p_write  (struct duat_9p_io *, int_32, int_64, int_32, char *);
void duat_9p_clunk  (struct duat_9p_io *, int_32);
void duat_9p_remove (struct duat_9p_io *, int_32);
void duat_9p_stat   (struct duat_9p_io *, int_32);
void duat_9p_wstat  (struct duat_9p_io *, int_32); /* missing some parameters */

void duat_9p_reply_auth   (struct duat_9p_io *, int_16, struct duat_9p_qid);
void duat_9p_reply_attach (struct duat_9p_io *, int_16, struct duat_9p_qid);
void duat_9p_reply_flush  (struct duat_9p_io *, int_16);
void duat_9p_reply_walk   (struct duat_9p_io *, int_16, struct duat_9p_qid *);
void duat_9p_reply_open   (struct duat_9p_io *, int_16, struct duat_9p_qid, int_32);
void duat_9p_reply_create (struct duat_9p_io *, int_16, struct duat_9p_qid, int_32);
void duat_9p_reply_read   (struct duat_9p_io *, int_16, int_32, char *);
void duat_9p_reply_write  (struct duat_9p_io *, int_16, int_32);
void duat_9p_reply_clunk  (struct duat_9p_io *, int_16);
void duat_9p_reply_remove (struct duat_9p_io *, int_16);
void duat_9p_reply_stat   (struct duat_9p_io *, int_16); /* still some stuff missing */
void duat_9p_reply_wstat  (struct duat_9p_io *, int_16);

void duat_9p_reply_error  (struct duat_9p_io *, int_16, char *);

#endif
