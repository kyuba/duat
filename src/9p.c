/**\file
 * \brief Duat 9P implementation
 *
 * \copyright
 * Copyright (c) 2008-2014, Kyuba Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/duat
 * \see Project Source Code: http://git.becquerel.org/kyuba/duat.git
 */

#include <duat/9p.h>
#include <duat/filesystem.h>
#include <curie/memory.h>
#include <curie/multiplex.h>

static unsigned int pop_message (unsigned char *, int_32, struct d9r_io *,
                                 void *);

struct io_element {
    struct d9r_io *io;
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

struct d9r_io *d9r_open_io (struct io *in, struct io *out) {
    static struct memory_pool d9r_io_pool = MEMORY_POOL_INITIALISER(sizeof (struct d9r_io));

    struct d9r_io *rv = get_pool_mem (&d9r_io_pool);

    if (rv == (struct d9r_io *)0) return (struct d9r_io *)0;

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

    rv->close   = (void *)0;

    rv->aux     = (void *)0;

    rv->version = d9r_uninitialised;

    in->type = iot_read;
    out->type = iot_write;

    return rv;
}

struct d9r_io *d9r_open_stdio() {
    struct io *in, *out;

    if ((in = io_open (0)) == (struct io *)0)
    {
        return (struct d9r_io *)0;
    }

    if ((out = io_open (1)) == (struct io *)0)
    {
        io_close (in);
        return (struct d9r_io *)0;
    }

    return d9r_open_io (in, out);
}

static void tree_free_pool_mem_tag (struct tree_node *n, void *aux)
{
    free_pool_mem (node_get_value (n));
}

static void tree_free_pool_mem_fid (struct tree_node *n, void *aux)
{
    struct d9r_fid_metadata *md =
            (struct d9r_fid_metadata *)node_get_value(n);

    if ((md->path_block_size > 0) &&
         (md->path != (char **)0))
    {
        afree (md->path_block_size, md->path);
    }

    free_pool_mem (md);
}

static void d9r_free_resources (struct d9r_io *io)
{
    tree_map (io->tags, tree_free_pool_mem_tag, (void *)0);
    tree_map (io->fids, tree_free_pool_mem_fid, (void *)0);

    tree_destroy (io->tags);
    tree_destroy (io->fids);

    free_pool_mem (io);
}

void d9r_close_io (struct d9r_io *io) {
    io_close (io->in);
    io_close (io->out);

    d9r_free_resources (io);
}

void multiplex_del_d9r (struct d9r_io *io)
{
    multiplex_del_io (io->in);
    multiplex_del_io (io->out);

    d9r_free_resources (io);
}

void multiplex_d9r () {
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
    unsigned char *sp, *bs;

    if ((i + 2) > length) return (char *)0;
    slen  = popw (b + i);

    if ((slen + i + 2) > length) return (char *)0;

    sp = (b + i);
    bs = sp;
    i += 2;

    for (; slen > 0; i++, slen--, sp++)
        *sp = b[i];
    *sp = (unsigned char)0;

    (*ip) = i + slen;

    return (void *)bs;
}

int_16 d9r_prepare_stat_buffer
        (struct d9r_io *io, int_8 **buffer, int_16 type, int_32 dev,
         struct d9r_qid *qid, int_32 mode, int_32 atime, int_32 mtime,
         int_64 length, char *name, char *uid, char *gid, char *muid, char *ext)
{
    int_16 nlen = 0;
    int_16 ulen = 0;
    int_16 glen = 0;
    int_16 mlen = 0;
    int_16 xlen = 0;

    int_16 slen =   2
                  + 4
                  + 13
                  + 4
                  + 4
                  + 4
                  + 8
                  + 2
                  + 2
                  + 2
                  + 2;
    int_16 sslen;
    int_8 *b;
    int_16 i;

    if (name != (char *)0) while (name[nlen]) nlen++;
    if (uid != (char *)0)  while (uid[ulen])  ulen++;
    if (gid != (char *)0)  while (gid[glen])  glen++;
    if (muid != (char *)0) while (muid[mlen]) mlen++;

    slen += nlen + ulen + glen + mlen;

    if (io->version == d9r_version_9p2000_dot_u) {
        if (ext != (char *)0) while (ext[xlen]) xlen++;
        slen += 2 + xlen + 4 + 4 + 4;
    }

    sslen = slen + 2;

    if ((b = aalloc (sslen)) == (int_8 *)0)
    {
        *buffer = (int_8 *)0;
        return 0;
    }

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
    i = 43;
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

    if (io->version == d9r_version_9p2000_dot_u) {
        int_32 du;

        *((int_16 *)(b + i))  = tolew (xlen);
        i += 2;
        if (ext != (char *)0) {
            int_16 j = 0;
            while (j < xlen) {
                b[i] = ext[j];
                j++, i++;
            }
        };

        du = 0;

        if (uid != (char *)0) {
            *((int_32 *)(b + i))     = tolel (dfs_get_user(uid));
        } else {
            *((int_32 *)(b + i))     = tolel (du);
        }
        if (gid != (char *)0) {
            *((int_32 *)(b + i + 4)) = tolel (dfs_get_group(gid));
        } else {
            *((int_32 *)(b + i + 4)) = tolel (du);
        }
        if (muid != (char *)0) {
            *((int_32 *)(b + i + 8)) = tolel (dfs_get_user(muid));
        } else {
            *((int_32 *)(b + i + 8)) = tolel (du);
        }
    }

    *buffer = b;
    return sslen;
}

int_32 d9r_parse_stat_buffer
        (struct d9r_io *io, int_32 slen, int_8 *b, int_16 *type,
         int_32 *dev, struct d9r_qid *qid, int_32 *mode, int_32 *atime,
         int_32 *mtime, int_64 *length, char **name, char **uid, char **gid,
         char **muid, char **ext)
{
    int_16 sl;
    int_32 i;

    if (slen < 43) return (int_32)0;
    sl = popw (b) + 2;

    if (slen < sl) return (int_32)0;

    *type        = popw (b + 2);
    *dev         = popl (b + 4);
    qid->type    = b[8];
    qid->version = popl (b + 9);
    qid->path    = popq (b + 13);
    *mode        = popl (b + 21);
    *atime       = popl (b + 25);
    *mtime       = popl (b + 29);
    *length      = popq (b + 33);

    i = 41;
    *name        = pop_string(b, &i, (int_32)sl);
    *uid         = pop_string(b, &i, (int_32)sl);
    *gid         = pop_string(b, &i, (int_32)sl);
    *muid        = pop_string(b, &i, (int_32)sl);

    if (io->version == d9r_version_9p2000_dot_u) {
        if (slen < (i + 2)) return (int_32)0;
        *ext = pop_string(b, &i, (int_32)sl);

        if (slen > (i + 12)) {
            int_32 nuid  = popl (b + i);
            int_32 ngid  = popl (b + i + 4);
            int_32 nmuid = popl (b + i + 8);

            if (*uid != (char *)0) {
                dfs_update_user (*uid, nuid);
            }
            if (*gid != (char *)0) {
                dfs_update_group (*gid, ngid);
            }
            if (*muid != (char *)0) {
                dfs_update_user (*muid, nmuid);
            }
        }
    }

    return (int_32)sl;
}

static void register_tag (struct d9r_io *io, int_16 tag) {
    static struct memory_pool d9r_tag_pool = MEMORY_POOL_INITIALISER(sizeof (struct d9r_tag_metadata));

    struct d9r_tag_metadata *md = get_pool_mem (&d9r_tag_pool);

    if (md == (struct d9r_tag_metadata *)0) return;

    md->aux = (void *)0;

    tree_add_node_value (io->tags, (int_pointer)tag, (void *)md);
}

static void kill_tag (struct d9r_io *io, int_16 tag) {
    struct tree_node *n =
            tree_get_node(io->tags, (int_pointer)tag);

    if (n != (struct tree_node *)0) {
        free_pool_mem ((struct d9r_tag_metadata *)node_get_value(n));
        tree_remove_node (io->tags, tag);
    }
}

static int_16 find_free_tag (struct d9r_io *io) {
    int_16 tag = 0;

    while (tree_get_node(io->tags, (int_pointer)tag) != (struct tree_node *)0) tag++;

    register_tag(io, tag);

    return tag;
}

int_32 find_free_fid (struct d9r_io *io) {
    int_32 fid = 2;

    while (tree_get_node(io->tags, (int_pointer)fid) != (struct tree_node *)0) fid++;

    return fid;
}

struct d9r_tag_metadata *
        d9r_tag_metadata (struct d9r_io *io, int_16 tag)
{
    struct tree_node *n =
            tree_get_node(io->tags, (int_pointer)tag);

    if (n != (struct tree_node *)0) {
        return (struct d9r_tag_metadata *)node_get_value(n);
    }

    return (struct d9r_tag_metadata *)0;
}

void register_fid (struct d9r_io *io, int_32 fid, int_16 pathc, char **path)
{
    static struct memory_pool d9r_fid_pool = MEMORY_POOL_INITIALISER(sizeof (struct d9r_fid_metadata));

    int_16 i = 0, size = 0;

    struct d9r_fid_metadata *md = get_pool_mem (&d9r_fid_pool);

    if (md == (struct d9r_fid_metadata *)0) return;

    md->aux             = (void *)0;
    md->path_count      = pathc;

    md->open            = 0;
    md->mode            = 0;
    md->index           = 0;

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

        if (pathb == (char *)0) {
            free_pool_mem ((void *)md);
            return;
        }

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

void kill_fid (struct d9r_io *io, int_32 fid)
{
    struct tree_node *n =
            tree_get_node(io->fids, (int_pointer)fid);

    if (n != (struct tree_node *)0) {
        struct d9r_fid_metadata *md =
                (struct d9r_fid_metadata *)node_get_value(n);

        if ((md->path_block_size > 0) &&
            (md->path != (char **)0))
        {
            afree (md->path_block_size, md->path);
        }

        free_pool_mem (md);
        tree_remove_node (io->fids, (int_pointer)fid);
    }
}

struct d9r_fid_metadata *
        d9r_fid_metadata (struct d9r_io *io, int_32 fid)
{
    struct tree_node *n =
            tree_get_node(io->fids, (int_pointer)fid);

    if (n != (struct tree_node *)0) {
        return (struct d9r_fid_metadata *)node_get_value(n);
    }

    return (struct d9r_fid_metadata *)0;
}

#define VERSION_STRING_9P2000 "9P2000"
#define VERSION_STRING_LENGTH 6
#define MINMSGSIZE            0x2000
#define MAXMSGSIZE            0x2000

static void mx_on_read_9p (struct io *in, void *d) {
    struct io_element *element = (struct io_element *)d;
    int_32 cl = (in->length - in->position);

    while (cl > 6) { /* enough data to parse a message... */
        int_32 length = popl ((unsigned char *)(in->buffer + in->position));

        if (cl < length) return;

        in->position += pop_message
                ((unsigned char *)(in->buffer + in->position), length,
                 element->io, element->data);

        cl = (in->length - in->position);
    }
}

static void mx_on_close_9p (struct io *in, void *d) {
    struct io_element *element = (struct io_element *)d;
    struct d9r_io *io = element->io;

    if (io->close != (void *)0)
    {
        io->close (io);
    }

    if (io->in != io->out)
    {
        multiplex_del_io (io->out);
    }
    d9r_free_resources (io);
    free_pool_mem (d);
}

void multiplex_add_d9r (struct d9r_io *io, void *data) {
    static struct memory_pool list_pool = MEMORY_POOL_INITIALISER(sizeof (struct io_element));

    struct io_element *element = get_pool_mem (&list_pool);

    if (element == (struct io_element *)0) return;

    element->io = io;
    element->data = data;

    multiplex_add_io (io->in, mx_on_read_9p, mx_on_close_9p, (void *)element);
    if (io->in != io->out)
    {
        multiplex_add_io_no_callback(io->out);
    }
}

/* message parser */

static unsigned int pop_message (unsigned char *b, int_32 length,
                                 struct d9r_io *io, void *d) {
    enum request_code code = (enum request_code)(b[4]);
    int_16 tag = popw (b + 5);
    int_32 i = 7;

    if ((code % 2) == 1) /* Rxxx message */
    {
        if ((tag != NO_TAG_9P) &&
            d9r_tag_metadata (io, tag) == (struct d9r_tag_metadata *)0)
        {
            /* reply message with unknown tag, gotta be ignored. maybe killing
               the connection might be an even better idea, but let's keep it
               like this for the time being.
               unfortunately, Rerror is reserved for client errors, so we can't
               even tell the server about this issue. */
            return length;
        }
    }

    switch (code) {
        case Tversion:
            register_tag(io, tag);
            if (length > 13)
            {
                char *versionstring;
                int_32 p;
                int_32 msize = popl (b + 7);

                if (msize < MINMSGSIZE) msize = MINMSGSIZE;
                if (msize > MAXMSGSIZE) msize = MAXMSGSIZE;
                io->max_message_size = msize;

                i += 4;
                versionstring = pop_string(b, &i, length);
                if (versionstring == (char *)0) break;

                for (p = 0;
                     (p < 6) &&
                     (versionstring[p] == VERSION_STRING_9P2000[p]);
                     p++);

                if (p == 6) {
                    io->version =
                      ((versionstring[6] == '.') &&
                       (versionstring[7] == 'u') &&
                       (versionstring[8] == (char)0)) ?
                            d9r_version_9p2000_dot_u :
                            d9r_version_9p2000;

                    d9r_reply_version(io, tag, msize, ((io->version == d9r_version_9p2000_dot_u) ? "9P2000.u" : "9P2000"));

                    return length;
                }

                d9r_reply_version(io, tag, msize, "unknown");
                return length;
            }
            break;

        case Rversion:
            if (length > 13)
            {
                int_32 msize = popl (b + 7);
                char *versionstring;

                if (msize > MAXMSGSIZE) msize = MAXMSGSIZE;

                io->max_message_size = msize;

                i += 4;
                versionstring = pop_string(b, &i, length);

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
                                d9r_version_9p2000_dot_u :
                                d9r_version_9p2000;
                    } else {
                        io->version = d9r_uninitialised;
                    }
                }
            }

            kill_tag (io, tag);
            return length;

        case Tauth:
            register_tag(io, tag);
            if (io->Tauth == (void *)0) break;

            if (length >= 15) {
                char *uname, *aname;

                int_32 afid = popl (b + 7);
                i = 11;

                uname = pop_string(b, &i, length);
                if (uname == (char *)0) break;

                aname = pop_string(b, &i, length);
                if (aname == (char *)0) break;

                io->Tauth(io, tag, afid, uname, aname);

                return length;
            }
            break;

        case Rauth:
            if (io->Rauth == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 20) {
                struct d9r_qid qid = {
                    .type = b[7],
                    .version = popl (b + 8),
                    .path = popq (b + 12)
                };

                io->Rauth(io, tag, qid);
            }

            kill_tag (io, tag);
            return length;

        case Tattach:
            register_tag(io, tag);
            if (io->Tattach == (void *)0) break;

            if (length >= 19) {
                char *uname, *aname;

                int_32 tfid = popl (b + 7);
                int_32 afid = popl (b + 11);
                i = 15;

                uname = pop_string(b, &i, length);
                if (uname == (char *)0) break;

                aname = pop_string(b, &i, length);
                if (aname == (char *)0) break;

                register_fid (io, tfid, 0, (char **)0);

                io->Tattach(io, tag, tfid, afid, uname, aname);

                return length;
            }
            break;

        case Rattach:
            if (io->Rattach == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 20) {
                struct d9r_qid qid = {
                    .type = b[7],
                    .version = popl (b + 8),
                    .path = popq (b + 12)
                };

                io->Rattach(io, tag, qid);
            }

            kill_tag (io, tag);
            return length;

        case Rerror:
            if (io->Rerror == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length > 9) {
                char *message = pop_string(b, &i, length);
                int_16 errno = P9_EDONTCARE;

                if (message == (char *)0)
                {
                    kill_tag (io, tag);
                    return length;
                }

                if ((io->version == d9r_version_9p2000_dot_u) &&
                    (length >= (i + 2)))
                {
                    errno = popw (b + i);
                }

                io->Rerror(io, tag, message, errno);
            }

            kill_tag (io, tag);
            return length;

        case Tflush:
            register_tag(io, tag);
            if (length >= 9) {
                int_16 otag = popw (b + 7);

                if (io->Tflush != (void *)0) {
                    io->Tflush(io, tag, otag);
                }

                kill_tag (io, otag);

                return length;
            }
            break;

        case Rflush:
            if (io->Rflush != (void *)0)
            {
                io->Rflush(io, tag);
            }

            kill_tag (io, tag);
            return length;

        case Twalk:
            register_tag(io, tag);
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
                        d9r_reply_error (io, tag, "Malformed message.",
                                             P9_EDONTCARE);
                        return length;
                    }
                }

                register_fid (io, nfid, namec, names);

                io->Twalk(io, tag, tfid, nfid, namec, names);

                return length;
            }
            break;

        case Rwalk:
            if (io->Rwalk == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 9) {
                int_16 qidc = popw (b + 7);
                int_16 r = 0;

                struct d9r_qid qid[qidc];

                i = 9;

                while (r < qidc) {
                    if ((i + 13) > length)
                    {
                        kill_tag (io, tag);
                        return length;
                    }

                    qid[r].type    = b[i];
                    qid[r].version = popl (b + i + 1);
                    qid[r].path    = popq (b + i + 5);

                    i += 13;

                    r++;
                }

                io->Rwalk(io, tag, qidc, qid);
            }

            kill_tag (io, tag);
            return length;

        case Topen:
            register_tag(io, tag);
            if (io->Topen == (void *)0) break;

            if (length >= 10) {
                int_32 fid  = popl (b + 7);
                int_8  mode = b[11];

                io->Topen(io, tag, fid, mode);
                return length;
            }
            break;

        case Ropen:
            if (io->Ropen == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 24) {
                struct d9r_qid qid = {
                    .type    = b[7],
                    .version = popl (b + 8),
                    .path    = popq (b + 12)
                };
                int_32 iounit= popl (b + 20);

                io->Ropen(io, tag, qid, iounit);
            }

            kill_tag (io, tag);
            return length;

        case Tcreate:
            register_tag(io, tag);
            if (io->Tcreate == (void *)0) break;

            if (length >= 18) {
                char *name, *ext = (char *)0;
                int_32 perm;
                int_8  mode;

                int_32 fid  = popl (b + 7);
                i = 11;

                name = pop_string(b, &i, length);
                if (name == (char *)0) break;

                if (length < (i + 5)) break;

                perm = popl (b + i);
                mode = b[i + 4];

                i += 5;

                if (length > (i + 2))
                {
                    ext = pop_string(b, &i, length);
                }

                io->Tcreate(io, tag, fid, name, perm, mode, ext);
                return length;
            }
            break;

        case Rcreate:
            if (io->Rcreate == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 24) {
                struct d9r_qid qid = {
                    .type    = b[7],
                    .version = popl (b + 8),
                    .path    = popq (b + 12)
                };
                int_32 iounit= popl (b + 20);

                io->Rcreate(io, tag, qid, iounit);
            }

            kill_tag (io, tag);
            return length;

        case Tread:
            register_tag(io, tag);
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
            if (io->Rread == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 11) {
                int_32 count = popl (b + 7);
                int_8 *data  = b + 11;

                if ((11 + count) <= length) {
                    io->Rread(io, tag, count, data);
                }
            }

            kill_tag (io, tag);
            return length;

        case Twrite:
            register_tag(io, tag);
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
            if (io->Rwrite == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 11) {
                int_32 count = popl (b + 7);

                io->Rwrite(io, tag, count);
            }

            kill_tag (io, tag);
            return length;

        case Tclunk:
            register_tag(io, tag);
            if (length >= 11) {
                int_32 fid = popl (b + 7);

                if (io->Tclunk == (void *)0)
                {
                    d9r_reply_clunk (io, tag);
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

            kill_tag (io, tag);
            return length;

        case Tremove:
            register_tag(io, tag);
            if (length >= 11) {
                int_32 fid = popl (b + 7);

                if (io->Tremove == (void *)0)
                {
                    d9r_reply_remove (io, tag);
                }
                else
                {
                    io->Tremove(io, tag, fid);
                }

                kill_fid (io, fid);

                return length;
            }
            break;

        case Rremove:
            if (io->Rremove != (void *)0)
            {
                io->Rremove(io, tag);
            }

            kill_tag (io, tag);
            return length;

        case Tstat:
            register_tag(io, tag);
            if (io->Tstat == (void *)0) break;

            if (length >= 11) {
                int_32 fid = popl (b + 7);

                io->Tstat(io, tag, fid);

                return length;
            }
            break;

        case Rstat:
            if (io->Rstat == (void *)0)
            {
                kill_tag (io, tag);
                return length;
            }

            if (length >= 45) {
                int_16 slen = popw (b + 7), type;
                struct d9r_qid qid;
                int_32 dev, mode, atime, mtime;
                int_64 length;
                char *name, *uid, *gid, *muid, *ext;

                d9r_parse_stat_buffer
                        (io, (int_32)slen, b + 9, &type, &dev, &qid, &mode,
                         &atime, &mtime, &length, &name, &uid, &gid, &muid,
                         &ext);

                io->Rstat(io, tag, type, dev, qid, mode, atime, mtime, length,
                          name, uid, gid, muid, ext);
            }

            kill_tag (io, tag);
            return length;

        case Twstat:
            register_tag(io, tag);
            if (io->Twstat == (void *)0) break;

            if (length >= 49) {
                int_16 slen = popw (b + 11), type;
                struct d9r_qid qid;
                int_32 fid = popl (b + 7), dev, mode, atime, mtime;
                int_64 length;
                char *name, *uid, *gid, *muid, *ext;

                d9r_parse_stat_buffer
                        (io, (int_32)slen, b + 13, &type, &dev, &qid, &mode,
                         &atime, &mtime, &length, &name, &uid, &gid, &muid,
                         &ext);

                io->Twstat(io, tag, type, fid, dev, qid, mode, atime, mtime,
                           length, name, uid, gid, muid, ext);
            }
            break;

        case Rwstat:
            if (io->Rwstat != (void *)0)
            {
                io->Rwstat(io, tag);
            }

            kill_tag (io, tag);
            return length;

        default:
            /* bad/unrecognised message */
            break;
    }

    d9r_reply_error (io, tag,
                     "Function not implemented or malformed message.",
                     P9_EDONTCARE);

    return length;
}

/* utility functions */

static void collect_qid (struct io *out, struct d9r_qid *qid) {
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
        (struct d9r_io *io, int_32 ol, int_8 c, int_16 tag)
{
    kill_tag(io, tag);

    collect_header (io->out, ol, c, tag);
}

/* request messages */

int_16 d9r_version (struct d9r_io *io, int_32 msize, char *version) {
    struct io *out = io->out;
    int_16 len = 0, slen;
    while (version[len]) len++;

    msize = tolel (msize);

    collect_header (out, 4 + 2 + len, Tversion, NO_TAG_9P);

    io_collect (out, (void *)&msize,     4);

    slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, version,            len);

    return NO_TAG_9P;
}

int_16 d9r_auth    (struct d9r_io *io, int_32 afid, char *uname,
                        char *aname)
{
    int_16 otag = find_free_tag (io);
    int_16 slen;
    struct io *out = io->out;
    int_16 uname_len = 0;
    int_16 aname_len = 0;

    register_fid (io, afid, 0, (char **)0);

    while (uname[uname_len]) uname_len++;
    while (aname[aname_len]) aname_len++;

    afid      = tolel (afid);

    collect_header (out, 4 + 2 + 2 + uname_len + aname_len, Tauth, otag);

    io_collect (out, (void *)&afid,      4);

    slen        = tolew (uname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, uname,              uname_len);

    slen        = tolew (aname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, aname,              aname_len);

    return otag;
}

int_16 d9r_attach  (struct d9r_io *io, int_32 fid, int_32 afid,
                        char *uname, char *aname)
{
    int_16 otag = find_free_tag (io);
    int_16 slen;
    struct io *out = io->out;
    int_16 uname_len = 0;
    int_16 aname_len = 0;
    register_fid (io, fid, 0, (char **)0);

    while (uname[uname_len]) uname_len++;
    while (aname[aname_len]) aname_len++;

    fid       = tolel (fid);
    afid      = tolel (afid);

    collect_header (out, 4 + 4 + 2 + 2 + uname_len + aname_len,
                    Tattach, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&afid,      4);

    slen        = tolew (uname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, uname,              uname_len);

    slen        = tolew (aname_len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, aname,              aname_len);

    return otag;
}

int_16 d9r_walk    (struct d9r_io *io, int_32 fid, int_32 newfid,
                        int_16 pathcount, char **path)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);
    int_16 p;
    int_32 ol  = 4 + 4 + 2 + (pathcount * 2);
    int_16 i   = 0, le[pathcount];

    register_fid (io, newfid, pathcount, path);

    while (i < pathcount)
    {
        int len = 0;

        while (path[i][len]) len++;
        le[i]  = len;
        ol    += len;

        i++;
    }

    fid        = tolel (fid);
    newfid     = tolel (newfid);

    collect_header (out, ol, Twalk, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&newfid,    4);

    p          = tolew (pathcount);
    io_collect (out, (void *)&p,         2);

    for (i = 0; i < pathcount; i++) {
        p      = tolew (le[i]);
        io_collect (out, (void *)&p,     2);
        io_collect (out, (void *)path[i],le[i]);
    }

    return otag;
}


int_16 d9r_stat    (struct d9r_io *io, int_32 fid)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid        = tolel (fid);

    collect_header (out, 4, Tstat, otag);

    io_collect (out, (void *)&fid,       4);

    return otag;
}

int_16 d9r_clunk   (struct d9r_io *io, int_32 fid) {
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    kill_fid(io, fid);

    fid         = tolel (fid);

    collect_header (out, 4, Tclunk, otag);

    io_collect (out, (void *)&fid,       4);

    return otag;
}

int_16 d9r_remove  (struct d9r_io *io, int_32 fid) {
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    kill_fid(io, fid);

    fid         = tolel (fid);

    collect_header (out, 4, Tremove, otag);

    io_collect (out, (void *)&fid,       4);

    return otag;
}

int_16 d9r_open    (struct d9r_io *io, int_32 fid, int_8 mode) {
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    fid         = tolel (fid);

    collect_header (out, 4 + 1, Topen, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&mode,      1);

    return otag;
}

int_16 d9r_create  (struct d9r_io *io, int_32 fid, const char *name,
                    int_32 perm, int_8 mode, const char *ext)
{
    int_16 len = 0, extlen = 0;
    int_16 otag = find_free_tag (io);
    struct io *out = io->out;
    int_16 slen;

    while (name[len]) len++;
    if ((io->version == d9r_version_9p2000_dot_u) &&
        (ext != (char *)0))
    {
        while (ext[extlen]) extlen++;
    }

    fid         = tolel (fid);
    perm        = tolel (fid);

    collect_header (out, 4 + 2 + len + 4 + 1 + ((io->version == d9r_version_9p2000_dot_u) ? 2 + extlen : 0), Tcreate, otag);

    io_collect (out, (void *)&fid,       4);

    slen        = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, (void *)&name,      len);

    io_collect (out, (void *)&perm,      4);
    io_collect (out, (void *)&mode,      1);

    if (io->version == d9r_version_9p2000_dot_u)
    {
        slen        = tolew (extlen);
        io_collect (out, (void *)&slen,      2);
        io_collect (out, (void *)&ext,       extlen);
    }

    return otag;
}

int_16 d9r_read    (struct d9r_io *io, int_32 fid, int_64 offset,
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

int_16 d9r_write   (struct d9r_io *io, int_32 fid, int_64 offset,
                        int_32 count, int_8 *data)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);
    int_32 c;

    fid       = tolel (fid);
    offset    = toleq (offset);

    collect_header (out, 4 + 8 + 4 + count, Twrite, otag);

    io_collect (out, (void *)&fid,       4);
    io_collect (out, (void *)&offset,    8);

    c         = tolel (count);
    io_collect (out, (void *)&c,         4);
    io_collect (out, (void *)data,       count);

    return otag;
}

int_16 d9r_wstat   (struct d9r_io *io, int_32 fid,
                        int_16 type, int_32 dev, struct d9r_qid qid,
                        int_32 mode, int_32 atime, int_32 mtime,
                        int_64 length, char *name, char *uid, char *gid,
                        char *muid, char *ext)
{
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);
    int_16 s;
    int_8 *bb;
    int_16 slen = d9r_prepare_stat_buffer
            (io, &bb, type, dev, &qid, mode, atime, mtime, length, name, uid,
             gid, muid, ext);

    fid         = tolel (fid);

    collect_header (out, 4 + 2 + slen, Twstat, otag);

    io_collect (out, (void *)&fid,       4);
    s           = tolew (slen);
    io_collect (out, (void *)&s,         2);
    io_collect (out, (void *)bb,         slen);

    afree (slen, bb);

    return otag;
}

int_16 d9r_flush   (struct d9r_io *io, int_16 oxtag) {
    struct io *out = io->out;
    int_16 otag = find_free_tag (io);

    kill_tag (io, oxtag);

    collect_header (out, 2, Tflush, otag);

    oxtag       = tolel (oxtag);
    io_collect (out, (void *)&oxtag,     2);

    return otag;
}

/* reply messages */

void d9r_reply_version (struct d9r_io *io, int_16 tag, int_32 msize, char *version) {
    int_16 len = 0;
    int_16 slen;
    struct io *out = io->out;

    while (version[len]) len++;

    collect_header_reply (io, 4 + 2 + len, Rversion, tag);

    msize = tolel (msize);
    io_collect (out, (void *)&msize,     4);

    slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, version,            len);

    kill_tag (io, tag);
}

void d9r_reply_error  (struct d9r_io *io, int_16 tag, const char *string,
                           int_16 errno)
{
    int_16 len = 0;
    int_16 slen;
    struct io *out = io->out;

    while (string[len]) len++;

    collect_header_reply (io,
                          2 + len +
                          ((io->version == d9r_version_9p2000_dot_u) ? 2 : 0),
                          Rerror, tag);

    slen = tolew (len);
    io_collect (out, (void *)&slen,      2);
    io_collect (out, string,             len);

    if (io->version == d9r_version_9p2000_dot_u) {
        errno = tolew (errno);
        io_collect (out, (void *)&errno, 2);
    }

    kill_tag (io, tag);
}

void d9r_reply_auth   (struct d9r_io *io, int_16 tag,
                           struct d9r_qid qid)
{
    collect_header_reply (io, 13, Rauth, tag);
    collect_qid (io->out, &qid);

    kill_tag (io, tag);
}

void d9r_reply_attach (struct d9r_io *io, int_16 tag,
                           struct d9r_qid qid)
{
    collect_header_reply (io, 13, Rattach, tag);
    collect_qid (io->out, &qid);

    kill_tag (io, tag);
}

void d9r_reply_walk   (struct d9r_io *io, int_16 tag, int_16 qidc,
                           struct d9r_qid *qid)
{
    struct io *out = io->out;
    int_16 i;
    collect_header_reply (io, 2 + qidc * 13, Rwalk, tag);

    tag         = tolew (qidc);
    io_collect (out, (void *)&tag,       2);

    for (i = 0; i < qidc; i++) {
        collect_qid (out, &(qid[i]));
    }

    kill_tag (io, tag);
}

void d9r_reply_stat   (struct d9r_io *io, int_16 tag, int_16 type,
                           int_32 dev, struct d9r_qid qid, int_32 mode,
                           int_32 atime, int_32 mtime, int_64 length,
                           char *name, char *uid, char *gid, char *muid,
                           char *ext)
{
    struct io *out = io->out;
    int_8 *bb;
    int_16 slen = d9r_prepare_stat_buffer
            (io, &bb, type, dev, &qid, mode, atime, mtime, length, name, uid,
             gid, muid, ext);

    collect_header_reply (io, 2 + slen, Rstat, tag);

    tag         = tolew (slen);
    io_collect (out, (void *)&tag,       2);
    io_collect (out, (void *)bb,         slen);

    afree (slen, bb);

    kill_tag (io, tag);
}

void d9r_reply_clunk   (struct d9r_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rclunk, tag);

    kill_tag (io, tag);
}

void d9r_reply_remove  (struct d9r_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rremove, tag);

    kill_tag (io, tag);
}

void d9r_reply_open   (struct d9r_io *io, int_16 tag,
                           struct d9r_qid qid, int_32 iounit)
{
    struct io *out = io->out;
    collect_header_reply (io, 13 + 4, Ropen, tag);

    collect_qid (out, &qid);
    iounit      = tolel (iounit);
    io_collect (out, (void *)&iounit,    4);

    kill_tag (io, tag);
}

void d9r_reply_create (struct d9r_io *io, int_16 tag,
                           struct d9r_qid qid, int_32 iounit)
{
    struct io *out = io->out;

    collect_header_reply (io, 13 + 4, Rcreate, tag);

    collect_qid (out, &qid);
    iounit      = tolel (iounit);
    io_collect (out, (void *)&iounit,    4);

    kill_tag (io, tag);
}

void d9r_reply_read   (struct d9r_io *io, int_16 tag, int_32 count,
                           int_8 *data)
{
    struct io *out = io->out;
    int_32 c;
    collect_header_reply (io, 4 + count, Rread, tag);

    c              = tolel (count);
    io_collect (out, (void *)&c,         4);
    io_collect (out, (void *)data,       count);

    kill_tag (io, tag);
}

void d9r_reply_write   (struct d9r_io *io, int_16 tag, int_32 count) {
    collect_header_reply (io, 4, Rwrite, tag);

    count       = tolel (count);
    io_collect (io->out, (void *)&count,     4);

    kill_tag (io, tag);
}

void d9r_reply_wstat   (struct d9r_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rwstat, tag);

    kill_tag (io, tag);
}

void d9r_reply_flush   (struct d9r_io *io, int_16 tag) {
    collect_header_reply (io, 0, Rflush, tag);

    kill_tag (io, tag);
}
