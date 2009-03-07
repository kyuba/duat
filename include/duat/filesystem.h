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

/*! \defgroup DuatVFS Virtual Filesystem
 *
 *  @{
 */

/*! \file
 *  \brief Duat Virtual Filesystem Header
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DUAT_FILESYSTEM_H
#define DUAT_FILESYSTEM_H

#include <duat/9p.h>

#define DFSSETUID    ((int_32)0x00080000)
#define DFSSETGID    ((int_32)0x00040000)
#define DFSUREAD     ((int_32)0x00000100)
#define DFSUWRITE    ((int_32)0x00000080)
#define DFSUEXEC     ((int_32)0x00000040)
#define DFSGREAD     ((int_32)0x00000020)
#define DFSGWRITE    ((int_32)0x00000010)
#define DFSGEXEC     ((int_32)0x00000008)
#define DFSOREAD     ((int_32)0x00000004)
#define DFSOWRITE    ((int_32)0x00000002)
#define DFSOEXEC     ((int_32)0x00000001)

struct dfs_node_common {
    enum {
        dft_directory,
        dft_file,
        dft_symlink,
        dft_device,
        dft_pipe,
        dft_socket
    } type;
    int_32 mode;
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
    struct dfs_directory *parent;
};

struct dfs_file {
    struct dfs_node_common c;
    char *path;
    int_8 *data;
    void *aux;
    void (*on_read)(struct d9r_io *, int_16, struct dfs_file *, int_64, int_32);
    int_32 (*on_write)(struct dfs_file *, int_64, int_32, int_8 *);
};

struct dfs_symlink {
    struct dfs_node_common c;
    char *symlink;
};

enum dfs_device_type {
    dfs_character_device,
    dfs_block_device
};

struct dfs_device {
    struct dfs_node_common c;
    enum dfs_device_type type;
    int_16 majour;
    int_16 minor;
};

struct dfs_socket {
    struct dfs_node_common c;
};

struct dfs_pipe {
    struct dfs_node_common c;
};

struct dfs {
    struct dfs_directory *root;
};

struct dfs *dfs_create ();
struct dfs_directory *dfs_mk_directory
        (struct dfs_directory *, char *);
struct dfs_file *dfs_mk_file
        (struct dfs_directory *, char *, char *, int_8 *, int_64, void *,
         void (*)(struct d9r_io *, int_16, struct dfs_file *, int_64, int_32),
         int_32 (*)(struct dfs_file*, int_64, int_32, int_8 *));
struct dfs_symlink *dfs_mk_symlink
        (struct dfs_directory *, char *, char *);
struct dfs_device *dfs_mk_device
        (struct dfs_directory *, char *, enum dfs_device_type, int_16, int_16);
struct dfs_socket *dfs_mk_socket
        (struct dfs_directory *, char *);
struct dfs_socket *dfs_mk_pipe
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

/*! \brief Update User and Group IDs
 *
 *  This function updates all the group and user ids using the /etc/group and
 *  /etc/passwd files. It actually only opens the files and adds them for the
 *  multiplexer to be processed, which means all the IDs will get updated
 *  while the multiplexer is running.
 */
void dfs_update_ids();

#endif

#ifdef __cplusplus
}
#endif

/*! @} */
