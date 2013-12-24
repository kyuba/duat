/**\file
 * \brief Duat VFS client implementation
 *
 * \copyright
 * Copyright (c) 2008-2013, Kyuba Project Members
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


#include <curie/memory.h>
#include <curie/io.h>
#include <curie/multiplex.h>
#include <sievert/immutable.h>
#include <sievert/tree.h>
#include <duat/filesystem.h>

#define BUFFERSIZE 4096

struct dfs *dfs_create (void (*close)(struct d9r_io *, void *), void *aux)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs));
    struct dfs *rv = get_pool_mem (&pool);

    if (rv == (struct dfs *)0) return (struct dfs *)0;

    rv->close = close;
    rv->aux   = aux;
    rv->root  = dfs_mk_directory((struct dfs_directory *)0, "/");
    if (rv->root == (struct dfs_directory *)0) {
        free_pool_mem (rv);
        return (struct dfs *)0;
    }

    return rv;
}

static void initialise_dfs_node_common (struct dfs_node_common *c)
{
    c->mode = 0644;
    c->atime = 1223234093; /* fairly random, and current, timestamp */
    c->mtime = 1223234093; /* fairly random, and current, timestamp */
    c->length = sizeof (*c);
    c->uid  = "root";
    c->gid  = "root";
    c->muid = "root";
}

struct dfs_directory *dfs_mk_directory (struct dfs_directory *dir, char *name)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_directory));

    struct dfs_directory *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_directory *)0) return (struct dfs_directory *)0;

    rv->nodes = tree_create();
    if (rv->nodes == (struct tree *)0)
    {
        free_pool_mem (rv);
        return (struct dfs_directory *)0;
    }

    initialise_dfs_node_common(&(rv->c));
    rv->c.length = (int_64)sizeof(struct dfs_directory);
    rv->c.type = dft_directory;
    rv->c.name = (char *)str_immutable(name);

    if (dir != (struct dfs_directory *)0)
    {
        rv->parent = dir;
        tree_add_node_string_value (dir->nodes, name, (void *)rv);
    }
    else
    {
        rv->parent = rv;
    }

    return rv;
}


struct dfs_file *dfs_mk_file (struct dfs_directory *dir, char *name, char *tfile, int_8 *tbuffer, int_64 tlength, void *aux, void (*on_read)(struct d9r_io *, int_16, struct dfs_file *, int_64, int_32), int_32 (*on_write)(struct dfs_file *, int_64, int_32, int_8 *))
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_file));
    struct dfs_file *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_file *)0) return (struct dfs_file *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.type = dft_file;
    rv->c.name = (char *)str_immutable(name);

    rv->data = tbuffer;
    rv->c.length = tlength;
    rv->on_read = on_read;
    rv->on_write = on_write;

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

struct dfs_symlink *dfs_mk_symlink (struct dfs_directory *dir, char *name, char *linkcontent)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_symlink));
    struct dfs_symlink *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_symlink *)0) return (struct dfs_symlink *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.length = (int_64)sizeof(struct dfs_symlink);
    rv->c.type = dft_symlink;
    rv->c.name = (char *)str_immutable(name);
    rv->symlink = (char *)str_immutable(linkcontent);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

struct dfs_device *dfs_mk_device (struct dfs_directory *dir, char *name, enum dfs_device_type type, int_16 majour, int_16 minor)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_device));
    struct dfs_device *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_device *)0) return (struct dfs_device *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.length = (int_64)sizeof(struct dfs_device);
    rv->c.type = dft_device;
    rv->c.name = (char *)str_immutable(name);
    rv->type = type;
    rv->majour = majour;
    rv->minor = minor;

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

struct dfs_socket *dfs_mk_pipe (struct dfs_directory *dir, char *name)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_socket));
    struct dfs_socket *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_socket *)0) return (struct dfs_socket *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.length = (int_64)sizeof(struct dfs_socket);
    rv->c.type = dft_pipe;
    rv->c.name = (char *)str_immutable(name);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

struct dfs_socket *dfs_mk_socket (struct dfs_directory *dir, char *name)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_socket));
    struct dfs_socket *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_socket *)0) return (struct dfs_socket *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.length = (int_64)sizeof(struct dfs_socket);
    rv->c.type = dft_socket;
    rv->c.name = (char *)str_immutable(name);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

/* user/group maps */

static struct tree dfs_user_map = TREE_INITIALISER;
static struct tree dfs_group_map = TREE_INITIALISER;

void   dfs_update_user  (char *user, int_32 id) {
    struct tree_node *node = tree_get_node_string (&dfs_user_map, user);
    if (node != (struct tree_node *)0) {
        tree_remove_node_string (&dfs_user_map, user);
    }

    tree_add_node_string_value(&dfs_user_map, user,
                                (void *)(int_pointer)id);
}

void   dfs_update_group (char *group, int_32 id) {
    struct tree_node *node = tree_get_node_string (&dfs_group_map, group);
    if (node != (struct tree_node *)0) {
        tree_remove_node_string (&dfs_group_map, group);
    }

    tree_add_node_string_value(&dfs_group_map, group,
                                (void *)(int_pointer)id);
}

int_32 dfs_get_user     (char *user) {
    struct tree_node *node = tree_get_node_string (&dfs_user_map, user);
    if (node != (struct tree_node *)0) {
        return (int_32)(int_pointer)node_get_value(node);
    }
    return (int_32)0;
}

int_32 dfs_get_group    (char *group) {
    struct tree_node *node = tree_get_node_string (&dfs_group_map, group);
    if (node != (struct tree_node *)0) {
        return (int_32)(int_pointer)node_get_value(node);
    }
    return (int_32)0;
}

enum pwd_field
{
    pf_account = 0,
    pf_password = 1,
    pf_uid = 2,
    pf_gid = 3,
    pf_comment = 4,
    pf_dir = 5,
    pf_shell = 6,
};

static void on_read_pwd (struct io *in, void *aux)
{
    unsigned int p = in->position;
    char *b = in->buffer;
    enum pwd_field status = pf_account;
    int cuid = 0;
    char unamebuffer[BUFFERSIZE];
    unsigned int unamepos = 0;

    unamebuffer[0] = 0;

    for (unsigned int l = in->length; p < l; p++)
    {
        switch (b[p])
        {
            case ':':
                status++;
                break;
            case '\n':
                status = pf_account;
                unamebuffer[unamepos] = 0;
                unamepos = 0;

                if (unamebuffer[0] != 0)
                {
                    dfs_update_user (unamebuffer, cuid);
                }

                cuid = 0;

                in->position = p;
                break;
            default:
                switch (status)
                {
                    case pf_account:
                        if (unamepos < (BUFFERSIZE-1))
                        {
                            unamebuffer[unamepos] = b[p];
                            unamepos++;
                        }
                        break;
                    case pf_uid:
                        cuid = cuid * 10 + (b[p] - '0');
                        break;
                    case pf_password:
                    case pf_gid:
                    case pf_comment:
                    case pf_dir:
                    case pf_shell:
                    default:
                        break;
                }
        }
    }
}

enum grp_field
{
    gf_name = 0,
    gf_password = 1,
    gf_gid = 2,
    gf_ulist = 3
};

static void on_read_grp (struct io *in, void *aux)
{
    unsigned int p = in->position;
    char *b = in->buffer;
    enum grp_field status = gf_name;
    int cgid = 0;
    char gnamebuffer[BUFFERSIZE];
    unsigned int gnamepos = 0;

    gnamebuffer[0] = 0;

    for (unsigned int l = in->length; p < l; p++)
    {
        switch (b[p])
        {
            case ':':
                status++;
                break;
            case '\n':
                status = pf_account;
                gnamebuffer[gnamepos] = 0;
                gnamepos = 0;

                if (gnamebuffer[0] != 0)
                {
                    dfs_update_group (gnamebuffer, cgid);
                }

                cgid = 0;

                in->position = p;
                break;
            default:
                switch (status)
                {
                    case gf_name:
                        if (gnamepos < (BUFFERSIZE-1))
                        {
                            gnamebuffer[gnamepos] = b[p];
                            gnamepos++;
                        }
                        break;
                    case gf_gid:
                        cgid = cgid * 10 + (b[p] - '0');
                        break;
                    case gf_password:
                    case gf_ulist:
                    default:
                        break;
                }
        }
    }
}

void dfs_update_ids()
{
    struct io *inpwd = io_open_read("/etc/passwd");
    struct io *ingrp = io_open_read("/etc/group");

    multiplex_add_io (inpwd, on_read_pwd, (void *)0, (void *)0);
    multiplex_add_io (ingrp, on_read_grp, (void *)0, (void *)0);
}
