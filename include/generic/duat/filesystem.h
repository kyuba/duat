/*
 *  filesystem.h
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DUAT_FILESYSTEM_H
#define DUAT_FILESYSTEM_H

#include <curie/int.h>

struct dfs_node_common {
    enum {
        dft_directory,
        dft_file,
        dft_symlink,
        dft_device,
        dft_socket
    } filetype;
    int_32 atime;
    int_32 mtime;
    int_64 length;
    char *name;
    char *uid;
    char *gid;
    char *muid;
};

struct dfs_directory {
    struct dfs_node_common c;
    struct tree *nodes;
};

struct dfs_file {
    struct dfs_node_common c;
    char *path;
    int_8 *data;
    void *aux;
    int_32 (*on_write)(struct dfs_file*, int_64, int_32, int_8 *);
};

struct dfs_symlink {
    struct dfs_node_common c;
    char *symlink;
};

struct dfs_device {
    struct dfs_node_common c;
    int_16 majour;
    int_16 minor;
};

struct dfs_socket {
    struct dfs_node_common c;
};

struct dfs {
    struct dfs_directory *root;
};

struct dfs *dfs_create ();
struct dfs_directory *dfs_mk_directory
        (struct dfs *, int_16, char **);
struct dfs_file *dfs_mk_file
        (struct dfs_directory *, char *, char *, int_8 *, int_64, void *,
         int_32 (*)(int_64, int_32, int_8 *, void *));
struct dfs_symlink *dfs_mk_symlink
        (struct dfs_directory *, char *, char *);
struct dfs_device *dfs_mk_device
        (struct dfs_directory *, char *, int_16, int_16);
struct dfs_socket *dfs_mk_socket
        (struct dfs_directory *, char *);

/*! \brief Set a User's UID
 *  \param[in] user The user whose ID to update.
 *  \param[in] uid  The new user ID.
 *
 *  The map manipulated by this function is used for the user IDs in the
 *  9P2000.u stat structures.
 */
void   dfs_update_user  (char *user, int_32 uid);

/*! \brief Set a Group's GID
 *  \param[in] group The group whose ID to update.
 *  \param[in] gid   The new group ID.
 *
 *  The map manipulated by this function is used for the group IDs in the
 *  9P2000.u stat structures.
 */
void   dfs_update_group (char *group, int_32 gid);

/*! \brief Retrieve a User's UID
 *  \param[in] user The user whose ID to retrieve.
 *  \return The user's ID. */
int_32 dfs_get_user     (char *user);

/*! \brief Retrieve a Group's GID
 *  \param[in] group The group whose ID to retrieve.
 *  \return The group's ID. */
int_32 dfs_get_group    (char *group);

#endif

#ifdef __cplusplus
}
#endif
