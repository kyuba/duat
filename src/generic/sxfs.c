/*
 *  sxfs.c
 *  libduat
 *
 *  Created by Magnus Deininger on 01/10/2008.
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

#include <curie/io.h>
#include <curie/sexpr.h>
#include <curie/multiplex.h>
#include <curie/immutable.h>
#include <duat/sxfs.h>

static void dfs_sxfs_recurse (struct dfs_directory *dir, struct sexpr *sx)
{
    static struct sexpr *sym_dir              = (struct sexpr *)0;
    static struct sexpr *sym_file             = (struct sexpr *)0;
    static struct sexpr *sym_block_device     = (struct sexpr *)0;
    static struct sexpr *sym_character_device = (struct sexpr *)0;
    static struct sexpr *sym_link             = (struct sexpr *)0;

    if (sym_dir == (struct sexpr *)0)
    {
        sym_dir              = make_symbol ("directory");
        sym_file             = make_symbol ("file");
        sym_block_device     = make_symbol ("block-device");
        sym_character_device = make_symbol ("character-device");
        sym_link             = make_symbol ("link");
    }

    if (consp(sx))
    {
        struct sexpr *scar = car(sx);

        if (truep(equalp(scar, sym_dir)))
        {
            struct dfs_directory *dx
                    = dfs_mk_directory (dir, (char *)sx_string(car(cdr(sx))));

            if (dx != (struct dfs_directory *)0)
            {
                sx = cdr(cdr(sx));

                while (!eolp(sx))
                {
                    dfs_sxfs_recurse(dx, car(sx));
                    sx = cdr(sx);
                }
            }
        }
        else if (truep(equalp(scar, sym_file)))
        {
            int_64 len = 0;
            int_8 *data
                    = (int_8 *)str_immutable_unaligned(sx_string(car(cdr(cdr(sx)))));

            while (data[len] != (int_8)0) len++;

            (void)dfs_mk_file (dir, (char *)sx_string(car(cdr(sx))), (char *)0,
                               data, len, (void *)0, (void *)0, (void *)0);
        }
        else if (truep(equalp(scar, sym_block_device)))
        {
            (void)dfs_mk_device (dir, (char *)sx_string(car(cdr(sx))),
                                 dfs_block_device,
                                 (int_16)sx_integer(car(cdr(cdr(sx)))),
                                 (int_16)sx_integer(car(cdr(cdr(cdr(sx))))));
        }
        else if (truep(equalp(scar, sym_character_device)))
        {
            (void)dfs_mk_device (dir, (char *)sx_string(car(cdr(sx))),
                                 dfs_character_device,
                                 (int_16)sx_integer(car(cdr(cdr(sx)))),
                                 (int_16)sx_integer(car(cdr(cdr(cdr(sx))))));
        }
        else if (truep(equalp(scar, sym_link)))
        {
            (void)dfs_mk_symlink (dir, (char *)sx_string(car(cdr(sx))),
                                  (char *)sx_string(car(cdr(cdr(sx)))));
        }
    }
}

static void dfs_sxfs_on_read(struct sexpr *sx, struct sexpr_io *io, void *d)
{
    struct dfs *fs = (struct dfs *)d;
    dfs_sxfs_recurse (fs->root, sx);
    sx_destroy (sx);
}

struct dfs *dfs_create_from_sxfs_io (struct sexpr_io *io)
{
    struct dfs *dfs;

    if ((dfs = dfs_create()) == (struct dfs *)0)
    {
        sx_close_io (io);
        return (struct dfs *)0;
    }

    multiplex_sexpr();
    multiplex_add_sexpr (io, dfs_sxfs_on_read, (void *)dfs);

    return dfs;
}

struct dfs *dfs_create_from_sxfs (char *path)
{
    struct io *out, *in;
    struct sexpr_io *io;

    if ((out = io_open (-1)) == (struct io *)0)
    {
        return (struct dfs *)0;
    }

    if ((in = io_open_read (path)) == (struct io *)0)
    {
        io_close (in);
        return (struct dfs *)0;
    }

    if ((io = sx_open_io (in, out)) == (struct sexpr_io *)0)
    {
        return (struct dfs *)0;
    }

    return dfs_create_from_sxfs_io(io);
}
