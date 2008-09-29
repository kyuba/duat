/*
 *  9p-server.c
 *  libduat
 *
 *  Created by Magnus Deininger on 27/09/2008.
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

#include <duat/9p-server.h>
#include <curie/multiplex.h>
#include <curie/network.h>
#include <curie/memory.h>

static void Tattach (struct d9r_io *io, int_16 tag, int_32 fid, int_32 afid,
                     char *uname, char *aname)
{
    struct d9r_fid_metadata *md = d9r_fid_metadata (io, fid);
    struct dfs *fs = (struct dfs *)io->aux;
    struct d9r_qid qid = { 0, 1, (int_64)io->aux };

    if (md != (struct d9r_fid_metadata *)0)
    {
        md->aux = fs->root;
    }

    d9r_reply_attach (io, tag, qid);
}

static void Twalk (struct d9r_io *io, int_16 tag, int_32 fid, int_32 afid,
                   int_16 c, char **names)
{
    struct dfs *fs = io->aux;
    struct d9r_qid qid[c];
    struct d9r_fid_metadata *md = d9r_fid_metadata (io, fid);
    struct dfs_directory *d;

    if (md != (struct d9r_fid_metadata *)0)
    {
        d = md->aux;
    }
    else
    {
        d = fs->root;
    }

    int_16 i = 0;

    while (i < c) {
        if (d->c.type == dft_directory) {
            struct tree_node *node
                    = tree_get_node_string(d->nodes, names[i]);

            if (node == (struct tree_node *)0)
            {
                break;
            }

            if ((d = node_get_value(node)) == (struct dfs_directory *)0)
            {
                break;
            }

            qid[i].type    = 0;
            qid[i].version = 1;
            qid[i].path    = (int_64)d;

            i++;
        } else {
            break;
        }
    }

    if ((i == 0) && (c != 0))
    {
        d9r_reply_error (io, tag, "Bad path element.", P9_EDONTCARE);
        return;
    }

    if (i == c)
    {
        md = d9r_fid_metadata (io, afid);
        md->aux = d;
    }

    d9r_reply_walk (io, tag, i, qid);
}

static void Tstat (struct d9r_io *io, int_16 tag, int_32 fid)
{
    struct d9r_fid_metadata *md = d9r_fid_metadata (io, fid);
    struct dfs_node_common *c = md->aux;
    struct d9r_qid qid = { 0, 1, (int_64)c };
    int_32 modex = 0;

    switch (c->type)
    {
        case dft_directory:
            qid.type = QTDIR;
            modex = DMDIR;
            break;
        case dft_symlink:
            qid.type = QTLINK;
            modex = DMSYMLINK;
            break;
        case dft_device:
            modex = DMDEVICE;
            break;
        case dft_socket:
            modex = DMSOCKET;
            break;
        case dft_file:
            break;
    }

    d9r_reply_stat (io, tag, 0, 0, qid, modex | c->mode, c->atime, c->mtime,
                    c->length, c->name, c->uid, c->gid, c->muid, (char *)0);
}

static void Topen (struct d9r_io *io, int_16 tag, int_32 fid, int_8 mode)
{
    struct d9r_fid_metadata *md = d9r_fid_metadata (io, fid);
    struct dfs_node_common *c = md->aux;
    struct d9r_qid qid = { 0, 1, (int_64)c };

    switch (c->type)
    {
        case dft_directory:
            qid.type = QTDIR;
            break;
        case dft_symlink:
            qid.type = QTLINK;
            break;
        default:
            break;
    }

    d9r_reply_open (io, tag, qid, 0x1000);
}

static void Tcreate (struct d9r_io *io, int_16 tag, int_32 fid, char *name, int_32 perm, int_8 mode)
{
    struct d9r_fid_metadata *md = d9r_fid_metadata (io, fid);
    struct dfs_node_common *c = md->aux;
    struct dfs_directory *d;
    struct d9r_qid qid = { 0, 1, 2 };

    if (c->type != dft_directory)
    {
        d9r_reply_error (io, tag,
                         "Cannot create nodes under anything but a directory.",
                         P9_EDONTCARE);
        return;
    }

    d = (struct dfs_directory *)c;

    if (perm & DMDIR)
    {
        qid.type = QTDIR;
        qid.path = (int_64)dfs_mk_directory(d, name);
    }
    else
    {
        qid.path = (int_64)dfs_mk_file
                (d, name, (char *)0, (int_8 *)0, 0, (void *)0, (void *)0, (void *)0);
    }

    d9r_reply_create (io, tag, qid, 0x1000);
}

struct Tread_dir_map
{
    int_32 index;
    int_16 tag;
    char replied;
    struct d9r_io *io;
};

static void Tread_dir (struct tree_node *node, void *v)
{
    struct Tread_dir_map *m = (struct Tread_dir_map *)v;

    if (m->index == 0)
    {
        struct dfs_node_common *c
                = (struct dfs_node_common *)node_get_value (node);

        if (c != (struct dfs_node_common *)0)
        {
            int_8 *bb;
            int_16 slen = 0;
            int_32 modex = 0;
            struct d9r_qid qid = { 0, 1, (int_64)c };

            switch (c->type)
            {
                case dft_directory:
                    qid.type = QTDIR;
                    modex = DMDIR;
                    break;
                case dft_symlink:
                    qid.type = QTLINK;
                    modex = DMSYMLINK;
                    break;
                case dft_device:
                    modex = DMDEVICE;
                    break;
                case dft_socket:
                    modex = DMSOCKET;
                    break;
                case dft_file:
                    break;
            }

            slen = d9r_prepare_stat_buffer
                    (m->io, &bb, 0, 0, &qid, modex | c->mode, c->atime,
                     c->mtime, c->length, c->name, c->uid, c->gid, c->muid,
                     (char *)0);
            d9r_reply_read (m->io, m->tag, slen, bb);
            afree (slen, bb);

            m->replied = (char)1;
        }
    }

    m->index--;
}

static void Tread (struct d9r_io *io, int_16 tag, int_32 fid, int_64 offset, int_32 length)
{
    struct d9r_fid_metadata *md = d9r_fid_metadata (io, fid);
    struct dfs_node_common *c = md->aux;

    switch (c->type)
    {
        case dft_directory:
            {
                struct Tread_dir_map m
                        = { .tag = tag, .replied = (char)0, .io = io };

                struct dfs_directory *dir = (struct dfs_directory *)c;
                if (offset == (int_64)0) md->index = 0;
                m.index = md->index;

                tree_map (dir->nodes, Tread_dir, (void *)&m);

                if (m.replied == (char)0)
                {
                    d9r_reply_read (io, tag, 0, (int_8 *)0);
                }

                (md->index)++;
            }
            break;
        case dft_file:
            {
                struct dfs_file *file = (struct dfs_file *)c;

                if (length > (file->c.length + offset))
                {
                    length = file->c.length - offset;
                }
                d9r_reply_read (io, tag, length, (file->data + offset));
            }
            break;
        default:
            d9r_reply_read (io, tag, 0, (int_8 *)0);
            break;
    }
}

static void Twrite (struct d9r_io *io, int_16 tag, int_32 fid, int_64 offset, int_32 count, int_8 *data)
{
    char dd[count + 1];
    int i;

    for (i = 0; i < count; i++) {
        dd[i] = (char)data[i];
    }
    dd[i] = 0;

    d9r_reply_write (io, tag, count);
}

void on_connect(struct io *in, struct io *out, void *p) {
    struct d9r_io *io = d9r_open_io (in, out);

    if (io == (struct d9r_io *)0) return;

    io->Tattach = Tattach;
    io->Twalk   = Twalk;
    io->Tstat   = Tstat;
    io->Topen   = Topen;
    io->Tcreate = Tcreate;
    io->Tread   = Tread;
    io->Twrite  = Twrite;
    io->aux     = p;

    multiplex_add_d9r (io, (void *)0);
}

void multiplex_d9s ()
{
    static char initialised = 0;

    if (initialised == (char)0)
    {
        multiplex_network();
        multiplex_d9r();

        initialised = (char)1;
    }
}

void multiplex_add_d9s_socket (char *socketname, struct dfs *fs)
{
    multiplex_add_socket (socketname, on_connect, (void *)fs);
}
