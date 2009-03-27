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

/*! \defgroup Duat9P 9P2000(.u)
 *
 *  This describes duat's 9p2000(.u) implementation. The implementation is
 *  fairly low-level, meaning it's very close to the structure of the protocol,
 *  which is not always that nice to use.
 *
 *  The interface is designed to avoid global state at all costs, which is why
 *  some functions have quite many arguments. However, since the protocol has
 *  been properly defined and is stable for quite a while, this should not pose
 *  too much of a problem.
 *
 *  You should also read http://www.cs.bell-labs.com/sys/man/5/INDEX.html and
 *  http://v9fs.sourceforge.net/rfc/9p2000.u.html to understand the 9P2000 and
 *  9P2000.u protocols themselves. Trying to use this library without
 *  understanding the protocols will be next to impossible.
 *
 *  @{
 */

/*! \file
 *  \brief Duat 9P2000(.u) Header
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DUAT_9P_H
#define DUAT_9P_H

#include <curie/int.h>
#include <curie/io.h>
#include <curie/tree.h>

/*! \defgroup P9Constants 9p Constants
 *  \brief Constants used in the 9P Protocol
 *
 *  You should check the Plan9 manual pages for a description of these contants.
 *
 *  @{
 */

/*! \brief 'Blank' Tag, usually used for T/Rversion */
#define NO_TAG_9P ((int_16)~0)
/*! \brief 'Blank' FID */
#define NO_FID_9P ((int_32)~0)

/*! \defgroup P9QIDConstants 9p QID Constants
 *  \brief qid.type Constants
 *
 *  @{
 */

/*! \brief File is a Directory */
#define QTDIR       ((int_8) 0x80)
/*! \brief File is a Append-only */
#define QTAPPEND    ((int_8) 0x40)
/*! \brief File can only be open exactly once */
#define QTEXCL      ((int_8) 0x20)
/*! \brief File describes a mount */
#define QTMOUNT     ((int_8) 0x10)
/*! \brief File is an Authorisation Ticket */
#define QTAUTH      ((int_8) 0x08)
/*! \brief File is Temporary */
#define QTTMP       ((int_8) 0x04)
/*! \brief File is a Symlink */
#define QTLINK      ((int_8) 0x02)
/*! \brief File is a regular File */
#define QTFILE      ((int_8) 0x00)

/*! @} */

/*! \defgroup P9ModeConstants 9p Mode Constants
 *  \brief File Mode Constants
 *
 *  @{
 */

/*! \brief File is a Directory*/
#define DMDIR       ((int_32)0x80000000)
/*! \brief File is Append-only */
#define DMAPPEND    ((int_32)0x40000000)
/*! \brief File can only be open exactly once */
#define DMEXCL      ((int_32)0x20000000)
/*! \brief File describes a mount */
#define DMMOUNT     ((int_32)0x10000000)
/*! \brief File is an Authorisation Ticket */
#define DMAUTH      ((int_32)0x08000000)
/*! \brief File is Temporary */
#define DMTMP       ((int_32)0x04000000)
/*! \brief File is a Symlink */
#define DMSYMLINK   ((int_32)0x02000000)
/*! \brief File is a Device File */
#define DMDEVICE    ((int_32)0x00800000)
/*! \brief File is a Named Pipe */
#define DMNAMEDPIPE ((int_32)0x00200000)
/*! \brief File is a Socket */
#define DMSOCKET    ((int_32)0x00100000)

/*! \brief Executing this File is performed as the File's User */
#define DMSETUID    ((int_32)0x00080000)
/*! \brief Executing this File is performed as a member of the File's Group */
#define DMSETGID    ((int_32)0x00040000)
/*! \brief Owner may read the File */
#define DMUREAD     ((int_32)0x00000100)
/*! \brief Owner may write to the File */
#define DMUWRITE    ((int_32)0x00000080)
/*! \brief Owner may execute the File */
#define DMUEXEC     ((int_32)0x00000040)
/*! \brief File's Group Members may read the File */
#define DMGREAD     ((int_32)0x00000020)
/*! \brief File's Group Members may write to the File */
#define DMGWRITE    ((int_32)0x00000010)
/*! \brief File's Group Members may execute the File */
#define DMGEXEC     ((int_32)0x00000008)
/*! \brief Anyone may read the File */
#define DMOREAD     ((int_32)0x00000004)
/*! \brief Anyone may write to the File */
#define DMOWRITE    ((int_32)0x00000002)
/*! \brief Anyone may execute the File */
#define DMOEXEC     ((int_32)0x00000001)
/*! @} */

/*! \defgroup P9OpenConstants 9p open() Constants
 *  \brief open() Parameter Constants
 *
 *  @{
 */

/*! \brief Open the File for reading */
#define P9_OREAD       ((int_8) 0x00)
/*! \brief Open the File for writing */
#define P9_OWRITE      ((int_8) 0x01)
/*! \brief Open the File for reading and writing */
#define P9_OREADWRITE  ((int_8) 0x02)
/*! \brief Open the File to execute it */
#define P9_OEXEC       ((int_8) 0x03)
/*! \brief Truncate the File */
#define P9_OTRUNC      ((int_8) 0x10)
/*! \brief Remove the File after closing it */
#define P9_ORCLOSE     ((int_8) 0x40)
/*! @} */

/*! \brief 9p2000.u Dummy Error Code */
#define P9_EDONTCARE   0

/*! @} */

/*! \brief A 9P2000 QID */
struct d9r_qid {
    /*! \brief The Type of the File */
    int_8  type;
    /*! \brief The 'Version' of the File */
    int_32 version;
    /*! \brief The unique ID for the File's Path */
    int_64 path;
};

/*! \defgroup P9Connections 9p Connections
 *  \brief 9P2000(.u) Connection Handling
 *
 *  Duat connections are designed to be used with curie's multiplexer.
 *
 *  @{
 */

/*! \brief Duat 9P2000(.u) IO Connection
 *
 *  This structure describes an active 9p2000.u connection.
 */
struct d9r_io {
    /*! \brief IO input file.
     *  \internal */
    struct io *in;
    /*! \brief IO output file.
     *  \internal */
    struct io *out;

    /*! \brief 9P Protocol Version */
    enum {
        /*! \brief The connection has not been initialised yet */
        d9r_uninitialised        = 0,
        /*! \brief The connection is using the 9P2000 Protocol */
        d9r_version_9p2000       = 1,
        /*! \brief The connection is using the 9P2000.u Protocol */
        d9r_version_9p2000_dot_u = 2
    }
    /*! \brief The negotiated Protocol Version */
    version;

    /*! \brief Maximum Size for any single Message.
     *  \note It is an error to generate a message that exceeds this size. */
    int_16 max_message_size;

    /*! \brief Active FIDs in this Connection. */
    struct tree *fids;
    /*! \brief Active Tags in this Connection. */
    struct tree *tags;

    /*! \brief Callback for an incoming Tauth Message */
    void (*Tauth)   (struct d9r_io *, int_16, int_32, char *, char *);
    /*! \brief Callback for an incoming Tattach Message */
    void (*Tattach) (struct d9r_io *, int_16, int_32, int_32, char *,
                     char *);
    /*! \brief Callback for an incoming Tflush Message */
    void (*Tflush)  (struct d9r_io *, int_16, int_16);
    /*! \brief Callback for an incoming Twalk Message */
    void (*Twalk)   (struct d9r_io *, int_16, int_32, int_32, int_16, char **);
    /*! \brief Callback for an incoming Topen Message */
    void (*Topen)   (struct d9r_io *, int_16, int_32, int_8);
    /*! \brief Callback for an incoming Tcreate Message */
    void (*Tcreate) (struct d9r_io *, int_16, int_32, char *, int_32, int_8, char *);
    /*! \brief Callback for an incoming Tread Message */
    void (*Tread)   (struct d9r_io *, int_16, int_32, int_64, int_32);
    /*! \brief Callback for an incoming Twrite Message */
    void (*Twrite)  (struct d9r_io *, int_16, int_32, int_64, int_32, int_8 *);
    /*! \brief Callback for an incoming Tclunk Message */
    void (*Tclunk)  (struct d9r_io *, int_16, int_32);
    /*! \brief Callback for an incoming Tremove Message */
    void (*Tremove) (struct d9r_io *, int_16, int_32);
    /*! \brief Callback for an incoming Tstat Message */
    void (*Tstat)   (struct d9r_io *, int_16, int_32);
    /*! \brief Callback for an incoming Twstat Message */
    void (*Twstat)  (struct d9r_io *, int_16, int_32, int_16, int_32,
                     struct d9r_qid, int_32, int_32, int_32, int_64, char *,
                     char *, char *, char *, char *);

    /*! \brief Callback for an incoming Rauth Message */
    void (*Rauth)   (struct d9r_io *, int_16, struct d9r_qid);
    /*! \brief Callback for an incoming Rattach Message */
    void (*Rattach) (struct d9r_io *, int_16, struct d9r_qid);
    /*! \brief Callback for an incoming Rerror Message */
    void (*Rerror)  (struct d9r_io *, int_16, const char *, int_16);
    /*! \brief Callback for an incoming Rflush Message */
    void (*Rflush)  (struct d9r_io *, int_16);
    /*! \brief Callback for an incoming Rwalk Message */
    void (*Rwalk)   (struct d9r_io *, int_16, int_16, struct d9r_qid *);
    /*! \brief Callback for an incoming Ropen Message */
    void (*Ropen)   (struct d9r_io *, int_16, struct d9r_qid, int_32);
    /*! \brief Callback for an incoming Rcreate Message */
    void (*Rcreate) (struct d9r_io *, int_16, struct d9r_qid, int_32);
    /*! \brief Callback for an incoming Rread Message */
    void (*Rread)   (struct d9r_io *, int_16, int_32, int_8 *);
    /*! \brief Callback for an incoming Rwrite Message */
    void (*Rwrite)  (struct d9r_io *, int_16, int_32);
    /*! \brief Callback for an incoming Rclunk Message */
    void (*Rclunk)  (struct d9r_io *, int_16);
    /*! \brief Callback for an incoming Rremove Message */
    void (*Rremove) (struct d9r_io *, int_16);
    /*! \brief Callback for an incoming Rstat Message */
    void (*Rstat)   (struct d9r_io *, int_16, int_16, int_32,
                     struct d9r_qid, int_32, int_32, int_32, int_64, char *,
                     char *, char *, char *, char *);
    /*! \brief Callback for an incoming Rwstat Message */
    void (*Rwstat)  (struct d9r_io *, int_16);

    /*! \brief Callback for when the 9P connection is closed */
    void (*close)   (struct d9r_io *);

    /*! \brief Arbitrary, User-defined Metadata */
    void *aux;
};

/*! \brief Initialise a 9P Connection on the given IO Structures
 *  \param[in] in  Input file.
 *  \param[in] out Output file.
 *  \return A new d9r_io structure.
 *
 *  After this function has been called, the IO structures may not be
 *  manipulated directly.
 */
/*@null@*/ struct d9r_io *d9r_open_io
        (/*@notnull@*/ struct io *in, /*@notnull@*/ struct io *out);

/*! \brief Open the Standard In- and Output of the Process as a 9P Connection */
/*@null@*/ struct d9r_io *d9r_open_stdio();

/*! \brief Close the given 9P Connection
 *
 *  This will also free the structure's memory and close the in and out file
 *  descriptors.
 */
void d9r_close_io (struct d9r_io *);

/*! \brief Initialise the 9P Multiplexer
 *
 *  This function is used to initialise curie's multiplex() function so that 9P
 *  connections can be used with it.
 */
void multiplex_d9r ();

/*! \brief Multiplex the given Connection
 *  \param[in] io   The connection to multiplex.
 *  \param[in] data Arbitrary, user-defined data.
 */
void multiplex_add_d9r (struct d9r_io *io, void *data);

/*! \brief Multiplex the given Connection
 *  \param[in] io   The connection to kill.
 */
void multiplex_del_d9r (struct d9r_io *io);

/*! @} */

/*! \defgroup P9Messages 9p Messages
 *  \brief 9P2000(.u) Message Construction and Handling
 *
 *  These functions are used to construct and send 9P2000.u messages.
 *
 *  All of the functions take a 9P connection structure as the first argument.
 *  The arguments are exactly the same as described in the Plan9 manual pages,
 *  except for messages modified by the 9P2000.u protocol, which take this
 *  latter protocol's parameters. For 9P2000 connections, the extra parameters
 *  will simply be ignored. Stat arguments are expanded to the stat contents.
 *
 *  @{
 */

/*! \brief Send a Tversion Message
 *  \return The tag the request was sent with. */
int_16 d9r_version (struct d9r_io *, int_32, char *);
/*! \brief Send a Tauth Message
 *  \return The tag the request was sent with. */
int_16 d9r_auth    (struct d9r_io *, int_32, char *, char *);
/*! \brief Send a Tattach Message
 *  \return The tag the request was sent with. */
int_16 d9r_attach  (struct d9r_io *, int_32, int_32, char *, char *);
/*! \brief Send a Tflush Message
 *  \return The tag the request was sent with. */
int_16 d9r_flush   (struct d9r_io *, int_16);
/*! \brief Send a Twalk Message
 *  \return The tag the request was sent with. */
int_16 d9r_walk    (struct d9r_io *, int_32, int_32, int_16, char **);
/*! \brief Send a Topen Message
 *  \return The tag the request was sent with. */
int_16 d9r_open    (struct d9r_io *, int_32, int_8);
/*! \brief Send a Tcreate Message
 *  \return The tag the request was sent with. */
int_16 d9r_create  (struct d9r_io *, int_32, const char *, int_32, int_8,
                    const char *);
/*! \brief Send a Tread Message
 *  \return The tag the request was sent with. */
int_16 d9r_read    (struct d9r_io *, int_32, int_64, int_32);
/*! \brief Send a Twrite Message
 *  \return The tag the request was sent with. */
int_16 d9r_write   (struct d9r_io *, int_32, int_64, int_32, int_8 *);
/*! \brief Send a Tclunk Message
 *  \return The tag the request was sent with. */
int_16 d9r_clunk   (struct d9r_io *, int_32);
/*! \brief Send a Tremove Message
 *  \return The tag the request was sent with. */
int_16 d9r_remove  (struct d9r_io *, int_32);
/*! \brief Send a Tstat Message
 *  \return The tag the request was sent with. */
int_16 d9r_stat    (struct d9r_io *, int_32);
/*! \brief Send a Twstat Message
 *  \return The tag the request was sent with. */
int_16 d9r_wstat   (struct d9r_io *, int_32, int_16, int_32,
                        struct d9r_qid, int_32, int_32, int_32, int_64,
                        char *, char *, char *, char *, char *);

/*! \brief Send an Rversion Message */
void d9r_reply_version (struct d9r_io *, int_16, int_32, char *);
/*! \brief Send an Rauth Message */
void d9r_reply_auth    (struct d9r_io *, int_16, struct d9r_qid);
/*! \brief Send an Rattach Message */
void d9r_reply_attach  (struct d9r_io *, int_16, struct d9r_qid);
/*! \brief Send an Rflush Message */
void d9r_reply_flush   (struct d9r_io *, int_16);
/*! \brief Send an Rwalk Message */
void d9r_reply_walk    (struct d9r_io *, int_16, int_16,
                            struct d9r_qid *);
/*! \brief Send an Ropen Message */
void d9r_reply_open    (struct d9r_io *, int_16, struct d9r_qid, int_32);
/*! \brief Send an Rcreate Message */
void d9r_reply_create  (struct d9r_io *, int_16, struct d9r_qid, int_32);
/*! \brief Send an Rread Message */
void d9r_reply_read    (struct d9r_io *, int_16, int_32, int_8 *);
/*! \brief Send an Rwrite Message */
void d9r_reply_write   (struct d9r_io *, int_16, int_32);
/*! \brief Send an Rclunk Message */
void d9r_reply_clunk   (struct d9r_io *, int_16);
/*! \brief Send an Rremove Message */
void d9r_reply_remove  (struct d9r_io *, int_16);
/*! \brief Send an Rstat Message */
void d9r_reply_stat    (struct d9r_io *, int_16, int_16, int_32,
                            struct d9r_qid, int_32, int_32, int_32, int_64,
                            char *, char *, char *, char *, char *);
/*! \brief Send an Rwstat Message */
void d9r_reply_wstat   (struct d9r_io *, int_16);

/*! \brief Send an Rerror Message */
void d9r_reply_error   (struct d9r_io *, int_16, const char *, int_16);

/*! @} */

/*! \defgroup P9TagMetadata 9p Tag Metadata
 *  \brief 9p Tag Metadata Handling
 *
 *  All active tags are tracked in the d9r_io struct, along with some
 *  metadata.
 *
 *  @{
 */

/*! \brief Metadata for a 9p-Tag
 *
 *  This struct can be used to keep track of additional metadata required to
 *  process a request.
 */
struct d9r_tag_metadata {
    /*! \brief Arbitrary, User-defined Metadata */
    void *aux;
};

/*! \brief Retrieve Tag Metadata */
struct d9r_tag_metadata *d9r_tag_metadata (struct d9r_io *, int_16);
/*! @} */

/*! \defgroup P9FIDMetadata 9p FID Handling
 *  \brief 9p FID Metadata Handling
 *
 *  All active fids are tracked in the d9r_io struct, along with some
 *  metadata. The metadata also encompasses the file that the fid points to,
 *  and it has provisions for keeping track of whether it is open and the mode
 *  it is open as.
 *
 *  @{
 */

/*! \brief Metadata for a 9p-FID
 *
 *  This struct can be used to keep track of additional metadata required to
 *  process a request.
 */
struct d9r_fid_metadata {
    /*! \brief Elements in d9r_fid_metadata.path */
    int_16   path_count;
    /*! \brief The Path pointed to by this FID */
    char   **path;

    /*! \brief Whether the File is open or not (set by the User) */
    char     open;
    /*! \brief The Mode of the File (set by the User) */
    int_8    mode;

    /*! \brief This Index should be used to keep track of the current File in a
     *         File Listing */
    int_32   index;

    /*! \brief Size (in bytes) of d9r_fid_metadata.path
     *  \internal */
    int_16   path_block_size;

    /*! \brief Arbitrary, User-defined Metadata */
    void    *aux;
};

/*! \brief Retrieve FID Metadata */
struct d9r_fid_metadata *d9r_fid_metadata (struct d9r_io *, int_32);

/*! \brief Register FID
 *  \param[in] io    The I/O structure to register the FID with.
 *  \param[in] fid   The FID to register.
 *  \param[in] pathc Path element count.
 *  \param[in] path  Path elements.
 */
void register_fid (struct d9r_io *io, int_32 fid, int_16 pathc, char **path);

/*! \brief Unregister FID
 *  \param[in] io    The I/O structure to unregister the FID from.
 *  \param[in] fid   The FID to unregister.
 */
void kill_fid (struct d9r_io *io, int_32 fid);

/*! \brief Find a free FID
 *  \param[in] io    The I/O structure to search in.
 *  \return The first free FID that was found.
 */
int_32 find_free_fid (struct d9r_io *io);

/*! @} */

/*! \defgroup P9UtilityFunctions 9P Utility Functions
 *
 *  These are simple helper functions for some common tasks.
 *
 *  @{
 */

/*! \brief Prepare a buffer containing the protocol's representation of a stat
 *         structure.
 *  \param[in]  io     Used to find out whether to create a plain or a .u
 *                     buffer.
 *  \param[out] buffer A pointer to a memory location that will be set to the
 *                     created buffer.
 *  \param[in]  type   For kernel use.
 *  \param[in]  dev    For kernel use.
 *  \param[in]  qid    The qid of the file.
 *  \param[in]  mode   File permissions and flags.
 *  \param[in]  atime  Time of last access.
 *  \param[in]  mtime  Time of last modification.
 *  \param[in]  length The length of the file (in bytes).
 *  \param[in]  name   The name of the file.
 *  \param[in]  uid    The name of the file's owner.
 *  \param[in]  gid    The file's group.
 *  \param[in]  muid   The name of the user who last modified the file.
 *  \param[in]  ex     9P2000.u extension field.
 *  \return The length of the created buffer.
 *
 *  Use curie's afree() to free the generated buffer.
 */
int_16 d9r_prepare_stat_buffer
        (struct d9r_io *io, int_8 **buffer, int_16 type, int_32 dev,
         struct d9r_qid *qid, int_32 mode, int_32 atime, int_32 mtime,
         int_64 length, char *name, char *uid, char *gid, char *muid, char *ex);

/*! \brief Parse a stat buffer.
 *  \param[in]  io      Used to find out whether to parse a plain or a .u
 *                      buffer.
 *  \param[in]  blength The length of the buffer to parse.
 *  \param[in]  buffer  The buffer to parse.
 *  \param[out] type    For kernel use.
 *  \param[out] dev     For kernel use.
 *  \param[out] qid     The qid of the file.
 *  \param[out] mode    File permissions and flags.
 *  \param[out] atime   Time of last access.
 *  \param[out] mtime   Time of last modification.
 *  \param[out] length  The length of the file (in bytes).
 *  \param[out] name    The name of the file.
 *  \param[out] uid     The name of the file's owner.
 *  \param[out] gid     The file's group.
 *  \param[out] muid    The name of the user who last modified the file.
 *  \param[out] ex      9P2000.u extension field.
 *  \return The number of bytes of data the buffer contained.
 *          0 for a bad buffer.
 */
int_32 d9r_parse_stat_buffer
        (struct d9r_io *io, int_32 blength, int_8 *buffer, int_16 *type,
         int_32 *dev, struct d9r_qid *qid, int_32 *mode, int_32 *atime,
         int_32 *mtime, int_64 *length, char **name, char **uid, char **gid,
         char **muid, char **ex);

/*! @} */

#endif

#ifdef __cplusplus
}
#endif

/*! @} */
