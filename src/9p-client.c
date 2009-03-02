/*
 * This file is part of the kyuba.org Duat project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008, 2009, Kyuba Project Members
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include <curie/memory.h>
#include <curie/multiplex.h>
#include <curie/network.h>
#include <curie/io.h>
#include <duat/9p-client.h>

#define ROOT_FID 1

enum d9c_status_code
{
    d9c_attaching,
    d9c_walking_read,
    d9c_opening_read,
    d9c_ready_read,
    d9c_walking_create,
    d9c_walking_write,
    d9c_opening_write,
    d9c_ready_write,
    d9c_ready,
    d9c_ready_write_working,
    d9c_closing_write,
    d9c_error
};

struct d9c_status
{
    enum d9c_status_code   code;
    void                 (*attach) (struct d9r_io *, void *);
    void                 (*error)  (struct d9r_io *, const char *, void *);
    void                  *aux;
};

struct d9c_tag_status
{
    enum d9c_status_code   code;
    int_32                 fid;
    int                    mode;
    struct io             *io;
    const char            *npath;
    int_64                 offset;
};

struct d9c_wx
{
    struct d9r_io         *io;
    struct d9c_tag_status *status;
};

static void invoke_write
        (struct d9r_io *io, struct d9c_tag_status *status, int_32 count)
{
    status->offset += (int_64)count;
    int_64 noff = status->offset;
    struct io *sio = status->io;

    sio->position += count;

    int_32 xlen = sio->length - sio->position;

    if (xlen == 0)
    {
        status->code = d9c_ready_write;
    }
    else
    {
        if (xlen > 0x1000) xlen = 0x1000;

        struct d9r_tag_metadata *md =
                d9r_tag_metadata
                (io, d9r_write (io, status->fid, noff, xlen,
                 (int_8 *)(sio->buffer + sio->position)));

        if (md != (struct d9r_tag_metadata *)0)
        {
            md->aux = (void *)status;
        }

        status->code = d9c_ready_write_working;
    }
}

static void d9c_write_on_read (struct io *f, void *aux)
{
    struct d9c_wx *wx = (struct d9c_wx *)aux;

    if (wx->status->code == d9c_ready_write)
    {
        invoke_write (wx->io, wx->status, 0);
    }
}

static void d9c_write_on_close (struct io *f, void *aux)
{
    struct d9c_wx *wx = (struct d9c_wx *)aux;
    struct d9c_tag_status *status = wx->status;
    struct d9r_io *io = wx->io;

    status->code = d9c_closing_write;

    struct d9r_tag_metadata *md =
            d9r_tag_metadata (io, d9r_clunk (io, status->fid));

    if (md != (struct d9r_tag_metadata *)0)
    {
        md->aux = (void *)status;
    }
}

static void Rattach (struct d9r_io *io, int_16 tag, struct d9r_qid qid)
{
    struct d9c_status *status = (struct d9c_status *)(io->aux);

    status->code = d9c_ready;

    if (status->attach != (void *)0)
    {
        status->attach (io, status->aux);
    }
}

static void Rwalk   (struct d9r_io *io, int_16 tag, int_16 qidn,
                     struct d9r_qid *qid)
{
    struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

    if (md->aux != (void *)0)
    {
        struct d9c_tag_status *status = (struct d9c_tag_status *)(md->aux);

        switch (status->code)
        {
            case d9c_walking_read:
            {
                struct d9r_tag_metadata *md =
                        d9r_tag_metadata (io, d9r_open (io, status->fid,
                                                        P9_OREAD));

                if (md != (struct d9r_tag_metadata *)0)
                {
                    status->code = d9c_opening_read;
                    md->aux = (void *)status;
                }

                break;
            }
            case d9c_walking_create:
            {
                struct d9r_tag_metadata *md =
                        d9r_tag_metadata
                            (io, d9r_create 
                                     (io, status->fid, status->npath,
                                      status->mode, P9_OWRITE, (char *)0));

                if (md != (struct d9r_tag_metadata *)0)
                {
                    status->code = d9c_opening_write;
                    md->aux = (void *)status;
                }

                break;
            }
            case d9c_walking_write:
            {
                struct d9r_tag_metadata *md =
                        d9r_tag_metadata (io, d9r_open (io, status->fid,
                                                        P9_OWRITE));

                if (md != (struct d9r_tag_metadata *)0)
                {
                    status->code = d9c_opening_write;
                    md->aux = (void *)status;
                }

                break;
            }

            default:
                break;
        }
    }
}

static void Rread   (struct d9r_io *io, int_16 tag, int_32 count, int_8 *data)
{
    struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

    if (md->aux != (void *)0)
    {
        struct d9c_tag_status *status = (struct d9c_tag_status *)(md->aux);
        int_64 noff = status->offset + (int_64)count;

        if (count == 0)
        {
            multiplex_del_io (status->io);
        }
        else
        {
            io_write (status->io, (const char *)data, count);
        }

        struct d9r_tag_metadata *md =
                d9r_tag_metadata (io, d9r_read (io, status->fid, noff,
                                                0x1000));

        if (md != (struct d9r_tag_metadata *)0)
        {
            md->aux = (void *)status;
        }
    }
}

static void Rwrite  (struct d9r_io *io, int_16 tag, int_32 count)
{
    struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

    if (md->aux != (void *)0)
    {
        invoke_write (io, (struct d9c_tag_status *)(md->aux), count);
    }
}

static void Ropen   (struct d9r_io *io, int_16 tag, struct d9r_qid qid,
                     int_32 fid)
{
    struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

    if (md->aux != (void *)0)
    {
        struct d9c_tag_status *status = (struct d9c_tag_status *)(md->aux);

        switch (status->code)
        {
            case d9c_opening_read:
                status->code = d9c_ready_read;

                md = d9r_tag_metadata (io, d9r_read (io, status->fid, 0,
                                                        0x1000));

                if (md != (struct d9r_tag_metadata *)0)
                {
                    md->aux = (void *)status;
                }

/*                Rread (io, tag, 0, (int_8 *)0);*/

                break;

            case d9c_opening_write:
                status->code = d9c_ready_write;

                struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

                if (md->aux != (void *)0)
                {
                    struct memory_pool pool
                            = MEMORY_POOL_INITIALISER
                                (sizeof(struct memory_pool));

                    struct d9c_wx *wx = (struct d9c_wx *)get_pool_mem (&pool);
                    if (wx != (struct d9c_wx *)0)
                    {
                        multiplex_add_io
                                (status->io, d9c_write_on_read,
                                 d9c_write_on_close, (void *)wx);
                    }
                }

                Rwrite (io, tag, 0);

            default:
                break;
        }
    }
}

static void Rerror  (struct d9r_io *io, int_16 tag, const char *string, int_16 code)
{
    struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

    if (md->aux != (void *)0)
    {
        struct d9c_tag_status *mds = (struct d9c_tag_status *)(md->aux);

        switch (mds->code)
        {
            case d9c_attaching:
            {
                struct d9c_status *status = (struct d9c_status *)(io->aux);

                status->code = d9c_error;

                if (status->error != (void *)0)
                {
                    status->error (io, string, status->aux);
                }
                break;
            }
            case d9c_walking_read:
            case d9c_walking_create:
            case d9c_walking_write:
            case d9c_opening_read:
            case d9c_opening_write:
            case d9c_ready_read:
            case d9c_ready_write:
            {
                io_finish (mds->io);
                kill_fid (io, mds->fid);
            }

            default:
                break;
        }
    }
}

static void Rclunk  (struct d9r_io *io, int_16 tag)
{
    struct d9r_tag_metadata *md = d9r_tag_metadata (io, tag);

    if (md->aux != (void *)0)
    {
        struct d9c_tag_status *mds = (struct d9c_tag_status *)(md->aux);

        switch (mds->code)
        {
            case d9c_closing_write:
            {
                kill_fid (io, mds->fid);
            }

            default:
                break;
        }
    }
}

/*
static void Rflush  (struct d9r_io *, int_16);
static void Rcreate (struct d9r_io *, int_16, struct d9r_qid, int_32);
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

void initialise_io
        (struct d9r_io *io,
         void (*attach) (struct d9r_io *, void *),
         void (*error)  (struct d9r_io *, const char *, void *),
         void *aux)
{
    struct memory_pool pool
            = MEMORY_POOL_INITIALISER (sizeof (struct d9c_status));
    struct d9c_status *status = get_pool_mem (&pool);
    struct d9r_tag_metadata *md;
    int_16 tag;

    if (status == (struct d9c_status *)0)
    {
        d9r_close_io (io);
        return;
    }

    status->code   = d9c_attaching;
    status->attach = attach;
    status->error  = error;
    status->aux    = aux;

    io->aux        = (void *)status;

    io->Rattach = Rattach;
    io->Rerror  = Rerror;
    io->Rwalk   = Rwalk;
    io->Ropen   = Ropen;
    io->Rread   = Rread;
    io->Rwrite  = Rwrite;
    io->Rclunk  = Rclunk;
/*    io->Rstat   = Rstat;
    io->Rcreate = Rcreate;
    io->Rstat   = Rstat;
    io->Rwstat  = Rwstat;*/

    multiplex_add_d9r (io, (void *)0);

    d9r_version (io, 0x2000, "9P2000");
    tag = d9r_attach  (io, ROOT_FID, NO_FID_9P, "none", "none");
    md  = d9r_tag_metadata (io, tag);

    if (md != (struct d9r_tag_metadata *)0)
    {
        struct memory_pool pool
                = MEMORY_POOL_INITIALISER (sizeof (struct d9c_tag_status));
        struct d9c_tag_status *tagstatus = get_pool_mem (&pool);

        tagstatus->code = d9c_attaching;

        md->aux = (void *)tagstatus;
    }
}

void multiplex_add_d9c_socket
        (const char *socket,
         void (*attach) (struct d9r_io *, void *),
         void (*error)  (struct d9r_io *, const char *, void *),
         void *aux)
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

        return;
    }

    multiplex_add_d9c_io  (in, out, attach, error, aux);
}

void multiplex_add_d9c_io
        (struct io *in, struct io *out,
         void (*attach) (struct d9r_io *, void *),
         void (*error)  (struct d9r_io *, const char *, void *),
         void *aux)
{
    struct d9r_io *io = d9r_open_io(in, out);

    if (io == (struct d9r_io *)0) return;

    initialise_io (io, attach, error, aux);
}

void multiplex_add_d9c_stdio
        (void (*attach) (struct d9r_io *, void *),
         void (*error)  (struct d9r_io *, const char *, void *),
         void *aux)
{
    struct d9r_io *io = d9r_open_stdio();

    if (io == (struct d9r_io *)0) return;

    initialise_io (io, attach, error, aux);
}

static struct io *io_open_9p
        (struct d9r_io *io9, const char *path, enum d9c_status_code code,
         const char *npath, int mode)
{
    struct io *io = io_open_special();

    if (io == (struct io *)0) return (struct io *)0;

    struct memory_pool pool
            = MEMORY_POOL_INITIALISER (sizeof (struct d9c_tag_status));
    struct d9c_tag_status *status = get_pool_mem (&pool);

    if (status == (struct d9c_tag_status *)0)
    {
        io_close (io);

        return (struct io *)0;
    }

    status->code   = code;
    status->mode   = mode;
    status->npath  = npath;
    status->offset = (int_64)0;
    int i = 0, j = 0;
    int_32 fid = find_free_fid (io9);

    while (path[0] == '/')
    {
        path++;
    }

    while (path[i])
    {
        if (path[i] == '/') j++;
        i++;
    }

    status->fid = fid;
    status->io  = io;

    if (j > 0)
    {
        j++;

        char  pathn[i];
        char *pathx[j];

        pathx[0] = pathn;

        for (j = 0, i = 0; path[i]; i++)
        {
            if (path[i] == '/')
            {
                pathn[i] = (char)0;
                j++;
                pathx[j] = pathn + i + 1;
            }
            else
            {
                pathn[i] = path[i];
            }
        }
        j++;
        pathn[i] = (char)0;

        struct d9r_tag_metadata *md =
                d9r_tag_metadata (io9, d9r_walk (io9, ROOT_FID, fid, j, pathx));

        if (md != (struct d9r_tag_metadata *)0)
        {
            md->aux = (void *)status;
        }
    }
    else
    {
        struct d9r_tag_metadata *md =
                d9r_tag_metadata (io9, d9r_walk (io9, ROOT_FID, fid, 1,
                                                 (char **)&path));

        if (md != (struct d9r_tag_metadata *)0)
        {
            md->aux = (void *)status;
        }
    }

    return io;
}

struct io *io_open_read_9p (struct d9r_io *io, const char *path)
{
    return io_open_9p (io, path, d9c_walking_read, (const char *)0, 0);
}

struct io *io_open_write_9p (struct d9r_io *io, const char *path)
{
    return io_open_9p (io, path, d9c_walking_write, (const char *)0, 0);
}

struct io *io_open_create_9p
        (struct d9r_io *io, const char *path, const char *npath, int mode)
{
    return io_open_9p (io, path, d9c_walking_create, npath, mode);
}
