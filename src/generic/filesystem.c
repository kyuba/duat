/*
 *  filesystem.c
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

#include <curie/tree.h>
#include <curie/memory.h>
#include <curie/immutable.h>
#include <duat/filesystem.h>

struct dfs *dfs_create () {
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs));
    struct dfs *rv = get_pool_mem (&pool);

    if (rv == (struct dfs *)0) return (struct dfs *)0;

    (void)dfs_mk_directory_r(rv, 0, (char **)0);
    if (rv->root == (struct dfs_directory *)0) {
        free_pool_mem (rv);
        return (struct dfs *)0;
    }

    return rv;
}

static void initialise_dfs_node_common (struct dfs_node_common *c)
{
    c->mode = 0644;
    c->atime = 0;
    c->mtime = 0;
    c->length = 0;
    c->uid = "anonymous";
    c->gid = "freeman";
    c->muid = "anonymous";
}

static struct memory_pool directory_pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_directory));

struct dfs_directory *dfs_mk_directory_r (struct dfs *fs, int_16 pcount, char **path)
{
    struct dfs_directory *rv, *parent = (struct dfs_directory *)0;

    if (pcount > 0) {
        struct dfs_directory *node = fs->root,
                             *pnode = node;
        int_16 i = 0;

        while ((node != (struct dfs_directory *)0) &&
               (i < pcount))
        {
            struct tree_node *n = tree_get_node_string (node->nodes, path[i]);
            if (n == (struct tree_node *)0)
                break;

            pnode = node;
            node = node_get_value (n);
            i++;
        }

        if (i == pcount) {
            return node;
        } else if (i == (pcount - 1)) {
            parent = pnode;
        } else {
            return (struct dfs_directory *)0;
        }
    }

    if ((rv = get_pool_mem (&directory_pool)) == (struct dfs_directory *)0) {
        return (struct dfs_directory *)0;
    }

    rv->nodes = tree_create();
    if (rv->nodes == (struct tree *)0)
    {
        free_pool_mem (rv);
        return (struct dfs_directory *)0;
    }

    initialise_dfs_node_common(&(rv->c));
    rv->c.type = dft_directory;
    rv->c.mode = 0755;

    if (pcount == 0)
    {
        rv->c.name = "/";
        fs->root = rv;
        return rv;
    } else {
        rv->c.name = (char *)str_immutable_unaligned(path[pcount - 1]);
        if (parent != (struct dfs_directory *)0)
        {
            tree_add_node_string_value (parent->nodes, path[pcount - 1],
                                        (void *)rv);
        }
    }

    return rv;
}

struct dfs_directory *dfs_mk_directory (struct dfs_directory *dir, char *name)
{
    struct dfs_directory *rv = get_pool_mem (&directory_pool);

    if (rv == (struct dfs_directory *)0) return (struct dfs_directory *)0;

    rv->nodes = tree_create();
    if (rv->nodes == (struct tree *)0)
    {
        free_pool_mem (rv);
        return (struct dfs_directory *)0;
    }

    initialise_dfs_node_common(&(rv->c));
    rv->c.type = dft_directory;
    rv->c.name = (char *)str_immutable_unaligned(name);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}


struct dfs_file *dfs_mk_file (struct dfs_directory *dir, char *name, char *tfile, int_8 *tbuffer, int_64 tlength, void *aux, void (*on_read)(struct dfs_file *, int_64, int_32, int_16), int_32 (*on_write)(struct dfs_file *, int_64, int_32, int_8 *))
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_file));
    struct dfs_file *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_file *)0) return (struct dfs_file *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.type = dft_file;
    rv->c.name = (char *)str_immutable_unaligned(name);

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
    rv->c.type = dft_symlink;
    rv->c.name = (char *)str_immutable_unaligned(name);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

struct dfs_device *dfs_mk_device (struct dfs_directory *dir, char *name, int_16 majour, int_16 minor)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_device));
    struct dfs_device *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_device *)0) return (struct dfs_device *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.type = dft_device;
    rv->c.name = (char *)str_immutable_unaligned(name);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

struct dfs_socket *dfs_mk_socket (struct dfs_directory *dir, char *name)
{
    static struct memory_pool pool = MEMORY_POOL_INITIALISER(sizeof (struct dfs_socket));
    struct dfs_socket *rv = get_pool_mem (&pool);

    if (rv == (struct dfs_socket *)0) return (struct dfs_socket *)0;

    initialise_dfs_node_common(&(rv->c));
    rv->c.type = dft_socket;
    rv->c.name = (char *)str_immutable_unaligned(name);

    tree_add_node_string_value (dir->nodes, name, (void *)rv);

    return rv;
}

/* user/group maps */

static struct tree dfs_user_map = TREE_INITIALISER;
static struct tree dfs_group_map = TREE_INITIALISER;

void   dfs_update_user  (char *user, int_32 id) {
    struct tree_node *node = tree_get_node_string (&dfs_user_map, user);
    if (node != (struct tree_node *)0) {
        node_get_value(node) = (void *)(int_pointer)id;
    } else {
        tree_add_node_string_value(&dfs_user_map, user,
                                    (void *)(int_pointer)id);
    }
}

void   dfs_update_group (char *group, int_32 id) {
    struct tree_node *node = tree_get_node_string (&dfs_group_map, group);
    if (node != (struct tree_node *)0) {
        node_get_value(node) = (void *)(int_pointer)id;
    } else {
        tree_add_node_string_value(&dfs_group_map, group,
                                    (void *)(int_pointer)id);
    }
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
