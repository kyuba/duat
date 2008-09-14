/*
 *  9p.c
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

#include <duat/9p.h>
#include <curie/memory.h>
#include <curie/multiplex.h>

static unsigned int pop_message (unsigned char *, int_32, struct duat_9p_io *,
                                 void *);

struct io_element {
    struct duat_9p_io *io;
    void *data;
};

/* read these codes in the plan9 sourcecode... guess there's not much of an
   alternative to look these codes up. */
enum request_code {
    Tversion = 100,
    Rversion = 101,
    Tauth    = 102,
    Rauth    = 103,
    Tattach  = 104,
    Rattach  = 105,
/*    Terror   = 106, */
    Rerror   = 107,
    Tflush   = 108,
    Rflush   = 109,
    Twalk    = 110,
    Rwalk    = 111,
    Topen    = 112,
    Ropen    = 113,
    Tcreate  = 114,
    Rcreate  = 115,
    Tread    = 116,
    Rread    = 117,
    Twrite   = 118,
    Rwrite   = 119,
    Tclunk   = 120,
    Rclunk   = 121,
    Tremove  = 122,
    Rremove  = 123,
    Tstat    = 124,
    Rstat    = 125,
    Twstat   = 126,
    Rwstat   = 127,
};

static struct memory_pool list_pool = MEMORY_POOL_INITIALISER(sizeof (struct io_element));
static struct memory_pool duat_io_pool = MEMORY_POOL_INITIALISER(sizeof (struct duat_9p_io));

static struct memory_pool duat_tag_pool = MEMORY_POOL_INITIALISER(sizeof (struct duat_9p_tag_metadata));
static struct memory_pool duat_fid_pool = MEMORY_POOL_INITIALISER(sizeof (struct duat_9p_fid_metadata));

struct duat_9p_io *duat_open_io (struct io *in, struct io *out) {
    struct duat_9p_io *rv;

    rv = get_pool_mem (&duat_io_pool);

    rv->in = in;
    rv->out = out;

    rv->tags = tree_create();
    rv->fids = tree_create();

    rv->Tauth   = (void *)0;
    rv->Tattach = (void *)0;
    rv->Tflush  = (void *)0;
    rv->Twalk   = (void *)0;
    rv->Topen   = (void *)0;
    rv->Tcreate = (void *)0;
    rv->Tread   = (void *)0;
    rv->Twrite  = (void *)0;
    rv->Tclunk  = (void *)0;
    rv->Tremove = (void *)0;
    rv->Tstat   = (void *)0;
    rv->Twstat  = (void *)0;

    rv->Rauth   = (void *)0;
    rv->Rattach = (void *)0;
    rv->Rerror  = (void *)0;
    rv->Rflush  = (void *)0;
    rv->Rwalk   = (void *)0;
    rv->Ropen   = (void *)0;
    rv->Rcreate = (void *)0;
    rv->Rread   = (void *)0;
    rv->Rwrite  = (void *)0;
    rv->Rclunk  = (void *)0;
    rv->Rremove = (void *)0;
    rv->Rstat   = (void *)0;
    rv->Rwstat  = (void *)0;

    rv->arbitrary = (void *)0;

    rv->version = duat_9p_uninitialised;

    in->type = iot_read;
    out->type = iot_write;

    return rv;
}

void duat_close_io (struct duat_9p_io *io) {
    io_close (io->in);
    io_close (io->out);

    free_pool_mem (io);
}

void multiplex_duat_9p () {
    static char installed = (char)0;

    if (installed == (char)0) {
        multiplex_io();
        installed = (char)1;
    }
}

static int_64 popq (unsigned char *p) {
    return (((int_64)(p[7])) << 56) +
           (((int_64)(p[6])) << 48) +
           (((int_64)(p[5])) << 40) +
           (((int_64)(p[4])) << 32) +
           (((int_64)(p[3])) << 24) +
           (((int_64)(p[2])) << 16) +
           (((int_64)(p[1])) << 8)  +
           (((int_64)(p[0])));
}

static int_32 popl (unsigned char *p) {
    return (((int_32)(p[3])) << 24) +
           (((int_32)(p[2])) << 16) +
           (((int_32)(p[1])) << 8)  +
           (((int_32)(p[0])));
}

static int_16 popw (unsigned char *p) {
    return (((int_16)(p[1])) << 8) +
           (((int_16)(p[0])));
}

static int_64 toleq (int_64 n) {
    union {
        unsigned char c[8];
        int_64 i;
    } res = { .c = { (unsigned char)(n         & 0xff),
                     (unsigned char)((n >> 8)  & 0xff),
                     (unsigned char)((n >> 16) & 0xff),
                     (unsigned char)((n >> 24) & 0xff),
                     (unsigned char)((n >> 32) & 0xff),
                     (unsigned char)((n >> 40) & 0xff),
                     (unsigned char)((n >> 48) & 0xff),
                     (unsigned char)((n >> 56) & 0xff) } };

    return res.i;
}

static int_32 tolel (int_32 n) {
    union {
      unsigned char c[4];
      int_32 i;
    } res = { .c = { (unsigned char)(n         & 0xff),
                     (unsigned char)((n >> 8)  & 0xff),
                     (unsigned char)((n >> 16) & 0xff),
                     (unsigned char)((n >> 24) & 0xff) } };

    return res.i;
}

static int_16 tolew (int_16 n) {
    union {
        unsigned char c[2];
        int_16 i;
    } res = { .c = { (unsigned char)(n         & 0xff),
                     (unsigned char)((n >> 8)  & 0xff) } };

    return res.i;
}

static char *pop_string (unsigned char *b, int_32 *ip, int_32 length) {
    int_16 slen;
    int_32 i = (*ip);

    if ((i + 2) > length) return (char *)0;
    slen  = popw (b + i);

    if ((slen + i + 2) > length) return (char *)0;

    unsigned char *sp = (b + i), *bs = sp;
    i += 2;

    for (; slen > 0; i++, slen--, sp++)
        *sp = b[i];
    *sp = (unsigned char)0;

    (*ip) = i + slen;

    return (void *)bs;
}

int_16 duat_9p_prepare_stat_buffer
        (struct duat_9p_io *io, int_8 **buffer, int_16 type, int_32 dev,
         struct duat_9p_qid *qid, int_32 mode, int_32 atime, int_32 mtime,
         int_64 length, char *name, char *uid, char *gid, char *muid)
{
    int_16 nlen = 0;
    if (name != (char *)0) while (name[nlen]) nlen++;
    int_16 ulen = 0;
    if (uid != (char *)0)  while (uid[ulen])  ulen++;
    int_16 glen = 0;
    if (gid != (char *)0)  while (gid[glen])  glen++;
    int_16 mlen = 0;
    if (muid != (char *)0) while (muid[mlen]) mlen++;

    int_16 slen =   2
                  + 4
                  + 13
                  + 4
                  + 4
                  + 4
                  + 8
                  + 2 + nlen
                  + 2 + ulen
                  + 2 + glen
                  + 2 + mlen;
    int_16 sslen = slen + 2;

    int_8 *b = aalloc (sslen);

    *((int_16 *)(b))      = tolew (slen);
    *((int_16 *)(b + 2))  = tolew (type);
    *((int_32 *)(b + 4))  = tolel (dev);
    *((int_8  *)(b + 8))  = qid->type;
    *((int_32 *)(b + 9))  = tolel (qid->version);
    *((int_64 *)(b + 13)) = toleq (qid->path);
    *((int_32 *)(b + 21)) = tolel (mode);
    *((int_32 *)(b + 25)) = tolel (atime);
    *((int_32 *)(b + 29)) = tolel (mtime);
    *((int_64 *)(b + 33)) = toleq (length);

    *((int_16 *)(b + 41)) = tolew (nlen);
    int_16 i = 43;
    if (name != (char *)0) {
        int_16 j = 0;
        while (j < nlen) {
            b[i] = name[j];
            j++, i++;
        }
    };

    *((int_16 *)(b + i))  = tolew (ulen);
    i += 2;
    if (uid != (char *)0) {
        int_16 j = 0;
        while (j < ulen) {
            b[i] = uid[j];
            j++, i++;
        }
    };

    *((int_16 *)(b + i))  = tolew (glen);
    i += 2;
    if (gid != (char *)0) {
        int_16 j = 0;
        while (j < glen) {
            b[i] = gid[j];
            j++, i++;
        }
    };

    *((int_16 *)(b + i))  = tolew (mlen);
    i += 2;
    if (muid != (char *)0) {
        int_16 j = 0;
        while (j < mlen) {
            b[i] = muid[j];
            j++, i++;
        }
    };

    *buffer = b;
    return sslen;
}

void duat_9p_parse_stat_buffer
        (struct duat_9p_io *io, int_32 slen, int_8 *b, int_16 *type,
         int_32 *dev, struct duat_9p_qid *qid, int_32 *mode, int_32 *atime,
         int_32 *mtime, int_64 *length, char **name, char **uid, char **gid,
         char **muid)
{
    if (slen < 43) return;
    int_16 sl = popw (b);

    if (slen < sl) return;

    *type        = popw (b + 2);
    *dev         = popl (b + 4);
    qid->type    = b[8];
    qid->version = popl (b + 9);
    qid->path    = popq (b + 13);
    *mode        = popl (b + 21);
    *atime       = popl (b + 25);
    *mtime       = popl (b + 29);
    *length      = popq (b + 33);

    int_32 i = 41;
    *name        = pop_string(b, &i, (int_32)sl);
    *uid         = pop_string(b, &i, (int_32)sl);
    *gid         = pop_string(b, &i, (int_32)sl);
    *muid        = pop_string(b, &i, (int_32)sl);
}


static void register_tag (struct duat_9p_io *io, int_16 tag) {
    struct duat_9p_tag_metadata *md = get_pool_mem (&duat_tag_pool);
    md->arbitrary = (void *)0;

    tree_add_node_value (io->tags, (int_pointer)tag, (void *)md);
}

static void kill_tag (struct duat_9p_io *io, int_16 tag) {
    struct tree_node *n =
            tree_get_node(io->tags, (int_pointer)tag);

    if (n != (struct tree_node *)0) {
        free_pool_mem ((struct duat_9p_tag_metadata *)node_get_value(n));
        tree_remove_node (io->tags, tag);
    }
}

static int_16 find_free_tag (struct duat_9p_io *io) {
    int_16 tag = 0;

    while (tree_get_node(io->tags, (int_pointer)tag) != (struct tree_node *)0) tag++;

    register_tag(io, tag);

    return tag;
}

struct duat_9p_tag_metadata *
        duat_9p_tag_metadata (struct duat_9p_io *io, int_16 tag)
{
    struct tree_node *n =
            tree_get_node(io->tags, (int_pointer)tag);

    if (n != (struct tree_node *)0) {
        return (struct duat_9p_tag_metadata *)node_get_value(n);
    }

    return (struct duat_9p_tag_metadata *)0;
}

static void register_fid (struct duat_9p_io *io, int_32 fid, int_16 pathc,
                          char **path) {
    struct duat_9p_fid_metadata *md = get_pool_mem (&duat_fid_pool);
    md->arbitrary       = (void *)0;
    md->path_count      = pathc;

    md->open            = 0;
    md->mode            = 0;
    md->index           = 0;

    int_16 i = 0, size = 0;

    while (i < pathc) {
        int_16 j = 0;

        size += sizeof(char *) + 1;

        while (path[i][j]) j++;

        size += j;
        i++;
    }

    md->path_block_size = size;

    if (size > 0) {
        char *pathb     = aalloc (size);
        char **pathbb   = (char **)pathb;

        int_16 b        = pathc * sizeof (char *);

        for (i = 0; i < pathc; i++) {
            int_16 j    = 0;

            pathbb[i]   = pathb + b;

            while (path[i][j]) {
                pathb[b]= path[i][j];

                b++;
                j++;
            }

            pathb[b]    = (char)0;
            b++;
        }

        md->path        = (char **)pathb;
    } else {
        md->path        = (char **)0;
    }

    tree_add_node_value (io->fids, (int_pointer)fid, (void *)md);
}

static void kill_fid (struct duat_9p_io *io, int_32 fid) {
    struct tree_node *n =
            tree_get_node(io->fids, (int_pointer)fid);

    if (n != (struct tree_node *)0) {
        struct duat_9p_fid_metadata *md =
                (struct duat_9p_fid_metadata *)node_get_value(n);

        if ((md->path_block_size > 0) &&
            (md->path != (char **)0))
        {
            afree (md->path_block_size, md->path);
        }

        free_pool_mem (md);
        tree_remove_node (io->fids, (int_pointer)fid);
    }
}

struct duat_9p_fid_metadata *
        duat_9p_fid_metadata (struct duat_9p_io *io, int_32 fid)
{
    struct tree_node *n =
            tree_get_node(io->fids, (int_pointer)fid);

    if (n != (struct tree_node *)0) {
        return (struct duat_9p_fid_metadata *)node_get_value(n);
    }

    return (struct duat_9p_fid_metadata *)0;
}

#define VERSION_STRING_9P2000 "9P2000"
#define VERSION_STRING_LENGTH 6
#define MINMSGSIZE            0x2000
#define MAXMSGSIZE            0x2000

static void mx_on_read_9p (struct io *in, void *d) {
    struct io_element *element = (struct io_element *)d;
    unsigned int p = in->position;
    int_32 cl = (in->length - p);

    if (cl > 6) { /* enough data to parse a message... */
        int_32 length = popl ((unsigned char *)(in->buffer + p));

        if (cl < length) return;

        in->position += pop_message ((unsigned char *)(in->buffer + p), length,
                                     element->io, element->data);
    }
}

static void mx_on_close_9p (struct io *in, void *d) {
    struct io_element *element = (struct io_element *)d;

    multiplex_del_io (element->io->out);
    free_pool_mem (element->io);
    free_pool_mem (d);
}

void multiplex_add_duat_9p (struct duat_9p_io *io, void *data) {
    struct io_element *element = get_pool_mem (&list_pool);

    element->io = io;
    element->data = data;

    multiplex_add_io (io->in, mx_on_read_9p, mx_on_close_9p, (void *)element);
    multiplex_add_io_no_callback(io->out);
}

/* message parser */

static unsigned int pop_message (unsigned char *b, int_32 length,
                                 struct duat_9p_io *io, void *d) {
    enum request_code code = (enum request_code)(b[4]);
    int_16 tag = popw (b + 5);
    int_32 i = 7;

    register_tag(io, tag);

    switch (code) {
        case Tversion:
            if (length > 13)
            {
                int_32 msize = popl (b + 7);

                if (msize < MINMSGSIZE) msize = MINMSGSIZE;
                if (msize > MAXMSGSIZE) msize = MAXMSGSIZE;
                io->max_message_size = msize;

                i += 4;
                char *versionstring = pop_string(b, &i, length);

                if (versionstring == (char *)0) {
                    duat_9p_reply_error (io, tag, "Malformed message.");
                    return 0;
                } else {
                    int_32 p;
                    for (p = 0;
                         (p < 6) &&
                         (versionstring[p] == VERSION_STRING_9P2000[p]);
                         p++);

                    if (p == 6) {
                        io->version =
                          ((versionstring[6] == '.') &&
                           (versionstring[7] == 'u') &&
                           (versionstring[8] == (char)0)) ?
                                duat_9p_version_9p2000_dot_u :
                                duat_9p_version_9p2000;

                        duat_9p_reply_version(io, tag, msize, ((io->version == duat_9p_version_9p2000_dot_u) ? "9P2000.u" : "9P2000"));

                        return length;
                    }

                    duat_9p_reply_version(io, tag, msize, "unknown");
                    return length;
                }
            }
            break;

        case Rversion:
            if (length > 13)
            {
                int_32 msize = popl (b + 7);

                if (msize > MAXMSGSIZE) msize = MAXMSGSIZE;

                io->max_message_size = msize;

                i += 4;
                char *versionstring = pop_string(b, &i, length);

                if (versionstring != (char *)0) {
                    int_32 p;
                    for (p = 0;
                         (p < 6) &&
                         (versionstring[p] == VERSION_STRING_9P2000[p]);
                         p++);

                    if (p == 6) {
                        io->version =
                          ((versionstring[6] == '.') &&
                           (versionstring[7] == 'u') &&
                           (versionstring[8] == (char)0)) ?
                                duat_9p_version_9p2000_dot_u :
                                duat_9p_version_9p2000;
                    } else {
                        io->version = duat_9p_uninitialised;
                    }
                }
            }
            return length;

        case Tauth:
            if (io->Tauth == (void *)0) break;

            if (length >= 15) {
                int_32 afid = popl (b + 7);
                i = 11;

                char *uname = pop_string(b, &i, length);
                if (uname == (char *)0) break;

                char *aname = pop_string(b, &i, length);
                if (aname == (char *)0) break;

                io->Tauth(io, tag, afid, uname, aname);

                return length;
            }
            break;

        case Rauth:
            if (io->Rauth == (void *)0) return length;

            if (length >= 20) {
                struct duat_9p_qid qid = {
                    .type = b[7],
                    .version = popl (b + 8),
                    .path = popq (b + 12)
                };

                io->Rauth(io, tag, qid);
            }
            return length;

        case Tattach:
            if (io->Tattach == (void *)0) break;

            if (length >= 19) {
                int_32 tfid = popl (b + 7);
                int_32 afid = popl (b + 11);
                i = 15;

                char *uname = pop_string(b, &i, length);
                if (uname == (char *)0) break;

                char *aname = pop_string(b, &i, length);
                if (aname == (char *)0) break;

                register_fid (io, tfid, 0, (char **)0);

                io->Tattach(io, tag, tfid, afid, uname, aname);

                return length;
            }
            break;

        case Rattach:
            if (io->Rattach == (void *)0) return length;

            if (length >= 20) {
                struct duat_9p_qid qid = {
                    .type = b[7],
                    .version = popl (b + 8),
                    .path = popq (b + 12)
                };

                io->Rattach(io, tag, qid);
            }
            return length;

        case Rerror:
            if (io->Rerror == (void *)0) return length;

            if (length > 9) {
                char *message = pop_string(b, &i, length);
                if (message != (char *)0) return length;

                io->Rerror(io, tag, message);
            }
            return length;

        case Tflush:
            if (io->Tflush == (void *)0) break;

            if (length >= 9) {
                int_16 otag = popw (b + 7);

                io->Tflush(io, tag, otag);

                return length;
            }
            break;

        case Rflush:
            if (io->Rflush != (void *)0)
            {
                io->Rflush(io, tag);
            }
            return length;

        case Twalk:
            if (io->Twalk == (void *)0) break;

            if (length >= 17) {
                int_32 tfid = popl (b + 7);
                int_32 nfid = popl (b + 11);
                int_16 namec = popw (b + 15);
                int_16 namei = 0;
                char *names[namec];

                i = 17;

                for (; namei < namec; namei++) {
                    names[namei] = pop_string(b, &i, length);

                    if (names[namei] == (char *)0) {
                        duat_9p_reply_error (io, tag, "Malformed message.");
                        return length;
                    }
                }

                register_fid (io, nfid, namec, names);

                io->Twalk(io, tag, tfid, nfid, namec, names);

                return length;
            }
            break;

        case Rwalk:
            if (io->Rwalk == (void *)0) return length;

            if (length >= 9) {
                int_16 qidc = popw (b + 7);
                int_16 r = 0;

                struct duat_9p_qid qid[qidc];

                i = 9;

                while (r < qidc) {
                    if ((i + 13) < length) return length;

                    qid[r].type    = b[i];
                    qid[r].version = popl (b + i + 1);
                    qid[r].path    = popq (b + i + 5);

                    i += 13;

                    r++;
                }

                io->Rwalk(io, tag, qidc, qid);
            }
            return length;

        case Topen:
            if (io->Topen == (void *)0) break;

            if (length >= 10) {
                int_32 fid  = popl (b + 7);
                int_8  mode = b[11];

                io->Topen(io, tag, fid, mode);
                return length;
            }
            break;

        case Ropen:
            if (io->Ropen == (void *)0) return length;

            if (length >= 24) {
                struct duat_9p_qid qid = {
                    .type    = b[7],
                    .version = popl (b + 8),
                    .path    = popq (b + 12)
                };
                int_32 iounit= popl (b + 20);

                io->Ropen(io, tag, qid, iounit);
            }
            return length;

        case Tcreate:
            if (io->Tcreate == (void *)0) break;

            if (length >= 18) {
                int_32 fid  = popl (b + 7);
                i = 11;

                char *name = pop_string(b, &i, length);
                if (name == (char *)0) break;

                if ((i + 5) < length) break;
                int_32 perm = popl (b + i);
                int_8  mode = b[i + 4];

                io->Tcreate(io, tag, fid, name, perm, mode);
                return length;
            }
            break;

        case Rcreate:
            if (io->Rcreate == (void *)0) return length;

            if (length >= 24) {
                struct duat_9p_qid qid = {
                    .type    = b[7],
                    .version = popl (b + 8),
                    .path    = popq (b + 12)
                };
                int_32 iounit= popl (b + 20);

                io->Rcreate(io, tag, qid, iounit);
            }
            return length;

        case Tread:
            if (io->Tread == (void *)0) break;

            if (length >= 23) {
                int_32 fid    = popl (b + 7);
                int_64 offset = popq (b + 11);
                int_32 count  = popl (b + 19);

                io->Tread(io, tag, fid, offset, count);
                return length;
            }
            break;

        case Rread:
            if (io->Rread == (void *)0) return length;

            if (length >= 11) {
                int_32 count = popl (b + 7);
                int_8 *data  = b + 11;

                if ((11 + count) <= length) {
                    io->Rread(io, tag, count, data);
                }
            }
            return length;

        case Twrite:
            if (io->Twrite == (void *)0) break;

            if (length >= 23) {
                int_32 fid   = popl (b + 7);
                int_64 offset= popl (b + 11);
                int_32 count = popl (b + 19);
                int_8 *data  = b + 23;

                if ((23 + count) <= length) {
                    io->Twrite(io, tag, fid, offset, count, data);
                }
                return length;
            }
            break;

        case Rwrite:
            if (io->Rwrite == (void *)0) return length;

            if (length >= 11) {
                int_32 count = popl (b + 7);

                io->Rwrite(io, tag, count);
            }
            return length;

        case Tclunk:
            if (length >= 11) {
                int_32 fid = popl (b + 7);

                if (io->Tclunk == (void *)0)
                {
                    duat_9p_reply_clunk (io, tag);
                }
                else
                {
                    io->Tclunk(io, tag, fid);
                }

                kill_fid (io, fid);

                return length;
            }
            break;
        case Rclunk:
            if (io->Rclunk != (void *)0)
            {
                io->Rclunk(io, tag);
            }
            return length;

        case Tremove:
        case Rremove:
            break;

        case Tstat:
            if (io->Tstat == (void *)0) break;

            if (length >= 11) {
                int_32 fid = popl (b + 7);

                io->Tstat(io, tag, fid);

                return length;
            }
            break;

        case Rstat:
            if (io->Rstat == (void *)0) return length;

            if (length >= 45) {
                int_16 slen = popw (b + 7), type;
                struct duat_9p_qid qid;
                int_32 dev, mode, atime, mtime;
                int_64 length;
                char *name, *uid, *gid, *muid;

                duat_9p_parse_stat_buffer
                        (io, (int_32)slen, b + 9, &type, &dev, &qid, &mode,
                         &atime, &mtime, &length, &name, &uid, &gid, &muid);

                io->Rstat(io, tag, type, dev, qid, mode, atime, mtime, length,
                          name, uid, gid, muid);
            }
            return length;

        case Twstat:
            if (io->Twstat == (void *)0) break;

            if (length >= 49) {
                int_16 slen = popw (b + 11), type;
                struct duat_9p_qid qid;
                int_32 fid = popl (b + 7), dev, mode, atime, mtime;
                int_64 length;
                char *name, *uid, *gid, *muid;

                duat_9p_parse_stat_buffer
                        (io, (int_32)slen, b + 13, &type, &dev, &qid, &mode,
                         &atime, &mtime, &length, &name, &uid, &gid, &muid);

                io->Twstat(io, tag, type, fid, dev, qid, mode, atime, mtime,
                           length, name, uid, gid, muid);
            }
            break;

        case Rwstat:
            if (io->Rwstat != (void *)0)
            {
                io->Rwstat(io, tag);
            }
            return length;

        default:
            /* bad/unrecognised message */
            break;
    }

    duat_9p_reply_error (io, tag,
                         "Function not implemented or malformed message.");

    return length;
}

/* utility functions */

static void collect_qid (struct io *out, struct duat_9p_qid *qid) {
    int_32 ol = tolel (qid->version);
    int_64 t  = toleq (qid->path);

    io_collect (out, (void *)&(qid->type), 1);
    io_collect (out, (void *)&ol,          4);
    io_collect (out, (void *)&t,           8);
}

static void collect_header (struct io *out, int_32 ol, int_8 c, int_16 tag) {
    ol    = tolel (4 + 1 + 2 + ol);
    tag   = tolew (tag);

    io_collect (out, (void *)&ol,        4);
    io_collect (out, (void *)&c,         1);
    io_collect (out, (void *)&tag,       2);
}

static void collect_header_reply
        (struct duat_9p_io *io, int_32 ol, int_8 c, int_16 tag)
{
    kill_tag(io, tag);

    collect_header (io->out, ol, c, tag);
}

/* request messages */

int_16 duat_9p_version (struct duat_9p_io *io, int_32 msize, char *version) {
    struct io *out = io->out;
    int_16 len = 0;
    while (version[len]) len++;

    msize = tolel (msize);

    collect_header (out, 4 + 2 + len, Tversion, NO_TAG_9P);

    io_collect (out, (void *)&msize,     4);

    int_16 slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, version,            len);

    return NO_TAG_9P;
}

int_16 duat_9p_auth    (struct duat_9p_io *io, int_32 afid, char *uname,
                        char *aname)
{
    register_fid (io, afid, 0, (char **)0);
    int_16 otag = find_free_tag (io);

    struct io *out = io->out;
    int_16 uname_len = 0;
    int_16 aname_len = 0;
    while (uname[uname_len]) uname_len++;
    while (aname[aname_len]) aname_len++;

    afid      = tolel (afid);

    collect_header (out, 4 + 2 + 2 + uname_len + aname_len, Tattach, otag);

    io_collect (out, (void *)&afid,      4);

    int_16 slen = tolew (uname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, uname,              uname_len);

    slen        = tolew (aname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, aname,              aname_len);

    return otag;
}

int_16 duat_9p_attach  (struct duat_9p_io *io, int_32 fid, int_32 afid,
                        char *uname, char *aname)
{
    register_fid (io, fid, 0, (char **)0);
    int_16 otag = find_free_tag (io);

    struct io *out = io->out;
    int_16 uname_len = 0;
    int_16 aname_len = 0;
    while (uname[uname_len]) uname_len++;
    while (aname[aname_len]) aname_len++;

    fid       = tolel (fid);
    afid      = tolel (afid);

    collect_header (out, 4 + 4 + 2 + 2 + uname_len + aname_len,
                    Tattach, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&afid,      4);

    int_16 slen = tolew (uname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, uname,              uname_len);

    slen        = tolew (aname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, aname,              aname_len);

    return otag;
}

int_16 duat_9p_walk    (struct duat_9p_io *io, int_32 fid, int_32 newfid,
                        int_16 pathcount, char **path)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    int_32 ol  = 4 + 4 + 2 + (pathcount * 2);
    int_16 i   = 0, le[pathcount];

    while (i < pathcount)
    {
        int len = 0;

        while (path[i][len]) len++;
        le[i]  = len;
        ol    += 1 + len;

        i++;
    }

    fid        = tolel (fid);
    newfid     = tolel (newfid);

    collect_header (out, ol, Twalk, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&newfid,    4);

    int_16 p   = tolew (pathcount);
    io_collect (out, (void *)&p,         2);

    for (i = 0; i < pathcount; i++) {
        p      = tolew (le[i]);
        io_collect (out, (void *)&p,     2);
        io_collect (out, (void *)path[i],le[i]);
    }

    return otag;
}


int_16 duat_9p_stat    (struct duat_9p_io *io, int_32 fid)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid        = tolel (fid);

    collect_header (out, 4, Tstat, otag);

    io_collect (out, (void *)&fid,       4);

    return otag;
}

int_16 duat_9p_clunk   (struct duat_9p_io *io, int_32 fid) {
    kill_fid(io, fid);

    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid         = tolel (fid);

    collect_header (out, 4, Tclunk, otag);

    io_collect (out, (void *)&fid,       4);

    return otag;
}

int_16 duat_9p_open    (struct duat_9p_io *io, int_32 fid, int_8 mode) {
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid         = tolel (fid);

    collect_header (out, 4 + 1, Topen, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&mode,      1);

    return otag;
}

int_16 duat_9p_create  (struct duat_9p_io *io, int_32 fid, char *name,
                        int_32 perm, int_8 mode)
{
    int_16 len = 0;
    int_16 otag = find_free_tag (io);
    while (name[len]) name++;

    struct io *out = io->out;

    fid         = tolel (fid);
    perm        = tolel (fid);

    collect_header (out, 4 + 2 + len + 4 + 1, Tcreate, otag);

    io_collect (out, (void *)&fid,       4);

    int_16 slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, (void *)&name,      len);

    io_collect (out, (void *)&perm,      4);
    io_collect (out, (void *)&mode,      1);

    return otag;
}

int_16 duat_9p_read    (struct duat_9p_io *io, int_32 fid, int_64 offset,
                        int_32 count)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid         = tolel (fid);
    offset      = toleq (offset);
    count       = tolel (count);

    collect_header (out, 4 + 8 + 4, Tread, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&offset,    8);
    io_collect (out, (void *)&count,     4);

    return otag;
}

int_16 duat_9p_write   (struct duat_9p_io *io, int_32 fid, int_64 offset,
                        int_32 count, int_8 *data)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid       = tolel (fid);
    offset    = toleq (offset);

    collect_header (out, 4 + count, Rwrite, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&offset,    8);

    int_32 c  = tolel (count);
    io_collect (out, (void *)&c,         4);
    io_collect (out, (void *)data,       count);

    return otag;
}

int_16 duat_9p_wstat   (struct duat_9p_io *io, int_32 fid,
                        int_16 type, int_32 dev, struct duat_9p_qid qid,
                        int_32 mode, int_32 atime, int_32 mtime,
                        int_64 length, char *name, char *uid, char *gid,
                        char *muid)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    int_8 *bb;
    int_16 slen = duat_9p_prepare_stat_buffer
            (io, &bb, type, dev, &qid, mode, atime, mtime, length, name, uid,
             gid, muid);

    fid         = tolel (fid);

    collect_header (out, 4 + 2 + slen, Twstat, otag);

    io_collect (out, (void *)&fid,       4);
    int_16 s    = tolew (slen);
    io_collect (out, (void *)&s,         2);
    io_collect (out, (void *)bb,         slen);

    afree (slen, bb);

    return otag;
}

int_16 duat_9p_flush   (struct duat_9p_io *io, int_16 oxtag) {
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    collect_header (out, 2, Tflush, otag);

    oxtag       = tolel (oxtag);
    io_collect (out, (void *)&oxtag,     2);

    return otag;
}


/* reply messages */

void duat_9p_reply_version (struct duat_9p_io *io, int_16 tag, int_32 msize, char *version) {
    int_16 len = 0;
    while (version[len]) len++;

    collect_header_reply (io, 4 + 2 + len, Rversion, tag);

    struct io *out = io->out;

    msize = tolel (msize);
    io_collect (out, (void *)&msize,     4);

    int_16 slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, version,            len);
}

void duat_9p_reply_error  (struct duat_9p_io *io, int_16 tag, char *string) {
    int_16 len = 0;
    while (string[len]) len++;

    collect_header_reply (io, 2 + len, Rversion, tag);

    struct io *out = io->out;

    int_16 slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, string,             len);
}

void duat_9p_reply_attach (struct duat_9p_io *io, int_16 tag,
                           struct duat_9p_qid qid)
{
    collect_header_reply (io, 13, Rattach, tag);
    collect_qid (io->out, &qid);
}

void duat_9p_reply_walk   (struct duat_9p_io *io, int_16 tag, int_16 qidc,
                           struct duat_9p_qid *qid)
{
    collect_header_reply (io, 2 + qidc * 13, Rwalk, tag);

    struct io *out = io->out;

    tag         = tolew (qidc);
    io_collect (out, (void *)&tag,       2);

    for (int_16 i = 0; i < qidc; i++) {
        collect_qid (out, &(qid[i]));
    }
}

void duat_9p_reply_stat   (struct duat_9p_io *io, int_16 tag, int_16 type,
                           int_32 dev, struct duat_9p_qid qid, int_32 mode,
                           int_32 atime, int_32 mtime, int_64 length,
                           char *name, char *uid, char *gid, char *muid)
{
    int_8 *bb;
    int_16 slen = duat_9p_prepare_stat_buffer
            (io, &bb, type, dev, &qid, mode, atime, mtime, length, name, uid,
             gid, muid);

    collect_header_reply (io, 2 + slen, Rstat, tag);

    struct io *out = io->out;

    tag         = tolew (slen);
    io_collect (out, (void *)&tag,       2);
    io_collect (out, (void *)bb,         slen);

    afree (slen, bb);
}

void duat_9p_reply_clunk   (struct duat_9p_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rclunk, tag);
}

void duat_9p_reply_open   (struct duat_9p_io *io, int_16 tag,
                           struct duat_9p_qid qid, int_32 iounit)
{
    collect_header_reply (io, 13 + 4, Ropen, tag);

    struct io *out = io->out;

    collect_qid (out, &qid);
    iounit      = tolel (iounit);
    io_collect (out, (void *)&iounit,    4);
}

void duat_9p_reply_create (struct duat_9p_io *io, int_16 tag,
                           struct duat_9p_qid qid, int_32 iounit)
{
    struct io *out = io->out;

    collect_header_reply (io, 13 + 4, Rcreate, tag);

    collect_qid (out, &qid);
    iounit      = tolel (iounit);
    io_collect (out, (void *)&iounit,    4);
}

void duat_9p_reply_read   (struct duat_9p_io *io, int_16 tag, int_32 count,
                           int_8 *data)
{
    struct io *out = io->out;

    collect_header_reply (io, 4 + count, Rread, tag);

    int_32 c    = tolel (count);
    io_collect (out, (void *)&c,         4);
    io_collect (out, (void *)data,       count);
}

void duat_9p_reply_write   (struct duat_9p_io *io, int_16 tag, int_32 count) {
    collect_header_reply (io, 4, Rwrite, tag);

    count       = tolel (count);
    io_collect (io->out, (void *)&count,     4);
}

void duat_9p_reply_wstat   (struct duat_9p_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rwstat, tag);
}

void duat_9p_reply_flush   (struct duat_9p_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rflush, tag);
}
