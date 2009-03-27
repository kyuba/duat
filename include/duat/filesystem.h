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
 *  The virtual filesystem is used when serving data with the 9p-server code.
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

/*! \brief VFS Flag: Set User ID */
#define DFSSETUID    ((int_32)0x00080000)
/*! \brief VFS Flag: Set Group ID */
#define DFSSETGID    ((int_32)0x00040000)
/*! \brief VFS Flag: User's allowed to read file */
#define DFSUREAD     ((int_32)0x00000100)
/*! \brief VFS Flag: User's allowed to write file */
#define DFSUWRITE    ((int_32)0x00000080)
/*! \brief VFS Flag: User's allowed to execute file */
#define DFSUEXEC     ((int_32)0x00000040)
/*! \brief VFS Flag: Group's allowed to read */
#define DFSGREAD     ((int_32)0x00000020)
/*! \brief VFS Flag: Group's allowed to write */
#define DFSGWRITE    ((int_32)0x00000010)
/*! \brief VFS Flag: Group's allowed to execute */
#define DFSGEXEC     ((int_32)0x00000008)
/*! \brief VFS Flag: Others are allowed to read */
#define DFSOREAD     ((int_32)0x00000004)
/*! \brief VFS Flag: Others are allowed to write */
#define DFSOWRITE    ((int_32)0x00000002)
/*! \brief VFS Flag: Others are allowed to execute */
#define DFSOEXEC     ((int_32)0x00000001)

/*! \brief Common Node Items */
struct dfs_node_common {
    /*! \brief File Type Code */
    enum {
        dft_directory, /*!< File Type: Directory */
        dft_file,      /*!< File Type: Regular File */
        dft_symlink,   /*!< File Type: Symbolic Link */
        dft_device,    /*!< File Type: Device File */
        dft_pipe,      /*!< File Type: Named Pipe */
        dft_socket     /*!< File Type: Socket */
    }
    /*! \brief File Type */
    type;

    /*! \brief File Mode */
    int_32 mode;

    /*! \brief Time of last Access */
    int_32 atime;

    /*! \brief Time of last Modification */
    int_32 mtime;

    /*! \brief Length of the File */
    int_64 length;

    /*! \brief Name of the File */
    char *name;

    /*! \brief Owner Name */
    char *uid;

    /*! \brief Group Name */
    char *gid;

    /*! \brief Last User that modified the File */
    char *muid;
};

/*! \brief VFS Node: Directory */
struct dfs_directory {
    /*! \brief Common VFS Node Attributes */
    struct dfs_node_common c;

    /*! \brief Directory Nodes */
    struct tree *nodes;

    /*! \brief Parent Directory Link */
    struct dfs_directory *parent;
};

/*! \brief VFS Node: Regular File */
struct dfs_file {
    /*! \brief Common VFS Node Attributes */
    struct dfs_node_common c;

    /*! \brief File Path */
    char *path;

    /*! \brief File Data Contents */
    int_8 *data;

    /*! \brief Auxiliary Data */
    void *aux;

    /*! \brief Callback on File Reads */
    void (*on_read)(struct d9r_io *, int_16, struct dfs_file *, int_64, int_32);

    /*! \brief Callback on File Writes */
    int_32 (*on_write)(struct dfs_file *, int_64, int_32, int_8 *);
};

/*! \brief VFS Node: Symbolic Link */
struct dfs_symlink {
    /*! \brief Common VFS Node Attributes */
    struct dfs_node_common c;

    /*! \brief Symbolic Link Contents */
    char *symlink;
};

/*! \brief VFS: Device Type */
enum dfs_device_type {
    dfs_character_device, /*!< Character Device */
    dfs_block_device      /*!< Block Device */
};

/*! \brief VFS Node: Device File */
struct dfs_device {
    /*! \brief Common VFS Node Attributes */
    struct dfs_node_common c;

    /*! \brief VFS Node: Device Type */
    enum dfs_device_type type;

    /*! \brief VFS Node: Majour Number */
    int_16 majour;

    /*! \brief VFS Node: Minor Number */
    int_16 minor;
};

/*! \brief VFS Node: Socket File */
struct dfs_socket {
    /*! \brief Common VFS Node Attributes */
    struct dfs_node_common c;
};

/*! \brief VFS Node: Named Pipe */
struct dfs_pipe {
    /*! \brief Common VFS Node Attributes */
    struct dfs_node_common c;
};

/*! \brief VFS Root Node */
struct dfs {
    /*! \brief VFS Root Node Pointer */
    struct dfs_directory *root;

    /*! \brief Callback on Client Disconnects */
    void (*close)(struct d9r_io *, void *);

    /*! \brief Auxiliary Data for the Callback */
    void *aux;
};

/*! \brief Create VFS Root
 *  \param[in,out] close Callback on client disconnects.
 *  \param[in]     aux   Auxiliary data for the callback.
 *  \return The created VFS root.
 */
struct dfs *dfs_create (void (*close)(struct d9r_io *, void *), void *aux);

/*! \brief Create Directory
 *  \param[in] parent The parent directory to create the node in.
 *  \param[in] name   The name of the node to create.
 *  \return The created VFS node.
 */
struct dfs_directory *dfs_mk_directory
        (struct dfs_directory *parent, char *name);

/*! \brief Create File
 *  \param[in] parent   The parent directory to create the node in.
 *  \param[in] name     The name of the node to create.
 *  \param[in] tname    Target file name (not supported yet).
 *  \param[in] tbuffer  Data buffer.
 *  \param[in] tlength  Data buffer length.
 *  \param[in] aux      Auxiliary data for the callbacks.
 *  \param[in] on_read  Callbacks for file reads.
 *  \param[in] on_write Callbacks for file writes.
 *  \return The created VFS node.
 */
struct dfs_file *dfs_mk_file
        (struct dfs_directory *parent, char *name, char *tname, int_8 *tbuffer,
         int_64 tlength, void *aux,
         void (*on_read)(struct d9r_io *, int_16, struct dfs_file *, int_64,
                int_32),
         int_32 (*on_write)(struct dfs_file*, int_64, int_32, int_8 *));

/*! \brief Create Symbolic Link
 *  \param[in] parent The parent directory to create the node in.
 *  \param[in] name   The name of the node to create.
 *  \param[in] target Symbolic link target.
 *  \return The created VFS node.
 */
struct dfs_symlink *dfs_mk_symlink
        (struct dfs_directory *parent, char *name, char *target);

/*! \brief Create Device File
 *  \param[in] parent The parent directory to create the node in.
 *  \param[in] name   The name of the node to create.
 *  \param[in] type   Device file type.
 *  \param[in] majour Device file majour number.
 *  \param[in] minor  Device file minor number.
 *  \return The created VFS node.
 */
struct dfs_device *dfs_mk_device
        (struct dfs_directory *parent, char *name, enum dfs_device_type type,
         int_16 majour, int_16 minor);

/*! \brief Create Socket
 *  \param[in] parent The parent directory to create the node in.
 *  \param[in] name   The name of the node to create.
 *  \return The created VFS node.
 */
struct dfs_socket *dfs_mk_socket
        (struct dfs_directory *parent, char *name);

/*! \brief Create Named Pipe
 *  \param[in] parent The parent directory to create the node in.
 *  \param[in] name   The name of the node to create.
 *  \return The created VFS node.
 */
struct dfs_socket *dfs_mk_pipe
        (struct dfs_directory *parent, char *name);

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
