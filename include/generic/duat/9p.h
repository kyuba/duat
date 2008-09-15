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
#include <curie/tree.h>

enum duat_9p_connection_version {
    duat_9p_uninitialised        = 0,
    duat_9p_version_9p2000       = 1,
    duat_9p_version_9p2000_dot_u = 2
};

struct duat_9p_qid {
    int_8  type;
    int_32 version;
    int_64 path;
};

struct duat_9p_tag_metadata {
    void *arbitrary;
};

struct duat_9p_fid_metadata {
    void    *arbitrary;
    int_16   path_count;
    int_16   path_block_size;
    char   **path;

    char     open;
    int_8    mode;

    int_32   index;
};

#define NO_TAG_9P ((int_16)~0)
#define NO_FID_9P ((int_32)~0)

#define QTDIR       ((int_8) 0x80)
#define QTAPPEND    ((int_8) 0x40)
#define QTEXCL      ((int_8) 0x20)
#define QTMOUNT     ((int_8) 0x10)
#define QTAUTH      ((int_8) 0x08)
#define QTTMP       ((int_8) 0x04)
#define QTLINK      ((int_8) 0x02)
#define QTFILE      ((int_8) 0x00)

#define DMDIR       ((int_32)0x80000000)
#define DMAPPEND    ((int_32)0x40000000)
#define DMEXCL      ((int_32)0x20000000)
#define DMMOUNT     ((int_32)0x10000000)
#define DMAUTH      ((int_32)0x08000000)
#define DMTMP       ((int_32)0x04000000)
#define DMSYMLINK   ((int_32)0x02000000)
#define DMDEVICE    ((int_32)0x00800000)
#define DMNAMEDPIPE ((int_32)0x00200000)
#define DMSOCKET    ((int_32)0x00100000)
#define DMSETUID    ((int_32)0x00080000)
#define DMSETGID    ((int_32)0x00040000)

#define DMUREAD     ((int_32)0x00000100)
#define DMUWRITE    ((int_32)0x00000080)
#define DMUEXEC     ((int_32)0x00000040)

#define DMGREAD     ((int_32)0x00000020)
#define DMGWRITE    ((int_32)0x00000010)
#define DMGEXEC     ((int_32)0x00000008)

#define DMOREAD     ((int_32)0x00000004)
#define DMOWRITE    ((int_32)0x00000002)
#define DMOEXEC     ((int_32)0x00000001)

#define OREAD       ((int_8) 0x00)
#define OWRITE      ((int_8) 0x01)
#define OREADWRITE  ((int_8) 0x02)
#define OEXEC       ((int_8) 0x03)
#define OTRUNC      ((int_8) 0x10)
#define ORCLOSE     ((int_8) 0x40)

#define EDONTCARE   0

struct duat_9p_io {
    struct io *in, *out;

    enum duat_9p_connection_version version;
    int_16 max_message_size;

    struct tree *fids;
    struct tree *tags;

    void (*Tauth)   (struct duat_9p_io *, int_16, int_32, char *, char *);
    void (*Tattach) (struct duat_9p_io *, int_16, int_32, int_32, char *,
                     char *);
    void (*Tflush)  (struct duat_9p_io *, int_16, int_16);
    void (*Twalk)   (struct duat_9p_io *, int_16, int_32, int_32, int_16, char **);
    void (*Topen)   (struct duat_9p_io *, int_16, int_32, int_8);
    void (*Tcreate) (struct duat_9p_io *, int_16, int_32, char *, int_32, int_8);
    void (*Tread)   (struct duat_9p_io *, int_16, int_32, int_64, int_32);
    void (*Twrite)  (struct duat_9p_io *, int_16, int_32, int_64, int_32, int_8 *);
    void (*Tclunk)  (struct duat_9p_io *, int_16, int_32);
    void (*Tremove) (struct duat_9p_io *, int_16, int_32);
    void (*Tstat)   (struct duat_9p_io *, int_16, int_32);
    void (*Twstat)  (struct duat_9p_io *, int_16, int_32, int_16, int_32,
                     struct duat_9p_qid, int_32, int_32, int_32, int_64, char *,
                     char *, char *, char *, char *);

    void (*Rauth)   (struct duat_9p_io *, int_16, struct duat_9p_qid);
    void (*Rattach) (struct duat_9p_io *, int_16, struct duat_9p_qid);
    void (*Rerror)  (struct duat_9p_io *, int_16, char *, int_16);
    void (*Rflush)  (struct duat_9p_io *, int_16);
    void (*Rwalk)   (struct duat_9p_io *, int_16, int_16, struct duat_9p_qid *);
    void (*Ropen)   (struct duat_9p_io *, int_16, struct duat_9p_qid, int_32);
    void (*Rcreate) (struct duat_9p_io *, int_16, struct duat_9p_qid, int_32);
    void (*Rread)   (struct duat_9p_io *, int_16, int_32, int_8 *);
    void (*Rwrite)  (struct duat_9p_io *, int_16, int_32);
    void (*Rclunk)  (struct duat_9p_io *, int_16);
    void (*Rremove) (struct duat_9p_io *, int_16);
    void (*Rstat)   (struct duat_9p_io *, int_16, int_16, int_32,
                     struct duat_9p_qid, int_32, int_32, int_32, int_64, char *,
                     char *, char *, char *, char *);
    void (*Rwstat)  (struct duat_9p_io *, int_16);

    void *arbitrary;
};

struct duat_9p_io *duat_open_io (struct io *, struct io *);

#define duat_open_io_fd(in,out) duat_open_io (io_open ((in)), io_open((out)))
#define duat_open_stdio() duat_open_io_fd(0, 1)

void duat_close_io (struct duat_9p_io *);

void multiplex_duat_9p ();
void multiplex_add_duat_9p (struct duat_9p_io *, void *);

int_16 duat_9p_version (struct duat_9p_io *, int_32, char *);
int_16 duat_9p_auth    (struct duat_9p_io *, int_32, char *, char *);
int_16 duat_9p_attach  (struct duat_9p_io *, int_32, int_32, char *, char *);
int_16 duat_9p_flush   (struct duat_9p_io *, int_16);
int_16 duat_9p_walk    (struct duat_9p_io *, int_32, int_32, int_16, char **);
int_16 duat_9p_open    (struct duat_9p_io *, int_32, int_8);
int_16 duat_9p_create  (struct duat_9p_io *, int_32, char *, int_32, int_8);
int_16 duat_9p_read    (struct duat_9p_io *, int_32, int_64, int_32);
int_16 duat_9p_write   (struct duat_9p_io *, int_32, int_64, int_32, int_8 *);
int_16 duat_9p_clunk   (struct duat_9p_io *, int_32);
int_16 duat_9p_remove  (struct duat_9p_io *, int_32);
int_16 duat_9p_stat    (struct duat_9p_io *, int_32);
int_16 duat_9p_wstat   (struct duat_9p_io *, int_32, int_16, int_32,
                        struct duat_9p_qid, int_32, int_32, int_32, int_64,
                        char *, char *, char *, char *, char *);

void duat_9p_reply_version (struct duat_9p_io *, int_16, int_32, char *);
void duat_9p_reply_auth    (struct duat_9p_io *, int_16, struct duat_9p_qid);
void duat_9p_reply_attach  (struct duat_9p_io *, int_16, struct duat_9p_qid);
void duat_9p_reply_flush   (struct duat_9p_io *, int_16);
void duat_9p_reply_walk    (struct duat_9p_io *, int_16, int_16,
                            struct duat_9p_qid *);
void duat_9p_reply_open    (struct duat_9p_io *, int_16, struct duat_9p_qid, int_32);
void duat_9p_reply_create  (struct duat_9p_io *, int_16, struct duat_9p_qid, int_32);
void duat_9p_reply_read    (struct duat_9p_io *, int_16, int_32, int_8 *);
void duat_9p_reply_write   (struct duat_9p_io *, int_16, int_32);
void duat_9p_reply_clunk   (struct duat_9p_io *, int_16);
void duat_9p_reply_remove  (struct duat_9p_io *, int_16);
void duat_9p_reply_stat    (struct duat_9p_io *, int_16, int_16, int_32,
                            struct duat_9p_qid, int_32, int_32, int_32, int_64,
                            char *, char *, char *, char *, char *);
void duat_9p_reply_wstat   (struct duat_9p_io *, int_16);

void duat_9p_reply_error   (struct duat_9p_io *, int_16, char *, int_16);

struct duat_9p_tag_metadata *duat_9p_tag_metadata (struct duat_9p_io *, int_16);
struct duat_9p_fid_metadata *duat_9p_fid_metadata (struct duat_9p_io *, int_32);
int_16 duat_9p_prepare_stat_buffer
        (struct duat_9p_io *, int_8 **, int_16, int_32, struct duat_9p_qid *,
         int_32, int_32, int_32, int_64, char *, char *, char *, char *,
         char *);

void duat_9p_parse_stat_buffer
        (struct duat_9p_io *, int_32, int_8 *, int_16 *, int_32 *,
         struct duat_9p_qid *, int_32 *, int_32 *, int_32 *, int_64 *, char **,
         char **, char **, char **, char **);

void   duat_9p_update_user  (char *, int_32);
void   duat_9p_update_group (char *, int_32);

int_32 duat_9p_get_user     (char *);
int_32 duat_9p_get_group    (char *);

#endif
