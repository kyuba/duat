/*
 * This file is part of the kyuba.org Duat project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008-2010, Kyuba Project Members
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

/*! \defgroup Duat9PClient 9P2000(.u) Client
 *  \ingroup Duat9P
 *
 *  @{
 */

/*! \file
 *  \brief Duat 9P2000(.u) Client Header
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DUAT_9P_CLIENT_H
#define DUAT_9P_CLIENT_H

#include <duat/9p.h>

/*! \brief Initialise 9P Client Multiplexer */
void multiplex_d9c ();

/*! \brief Open Client Connection (with two I/O structures)
 *  \param[in]     in     The input structure.
 *  \param[in]     out    The output structure.
 *  \param[in,out] on_attach Callback when the connection is established.
 *  \param[in,out] on_error  Callback when an error occured.
 *  \param[in,out] on_close  Callback when the connection is terminated.
 *  \param[in]     aux       Auxiliary data to pass to the callbacks.
 */
void multiplex_add_d9c_io
        (struct io *in, struct io *out,
         void (*on_attach) (struct d9r_io *, void *),
         void (*on_error)  (struct d9r_io *, const char *, void *),
         void (*on_close)  (struct d9r_io *, void *),
         void *aux);

/*! \brief Open Client Connection (on a Socket)
 *  \param[in]     socket    The socket to serve on.
 *  \param[in,out] on_attach Callback when the connection is established.
 *  \param[in,out] on_error  Callback when an error occured.
 *  \param[in,out] on_close  Callback when the connection is terminated.
 *  \param[in]     aux       Auxiliary data to pass to the callbacks.
 */
void multiplex_add_d9c_socket
        (const char *socket,
         void (*on_attach) (struct d9r_io *, void *),
         void (*on_error)  (struct d9r_io *, const char *, void *),
         void (*on_close)  (struct d9r_io *, void *),
         void *aux);

/*! \brief Open Client Connection (on Standard I/O)
 *  \param[in,out] on_attach Callback when the connection is established.
 *  \param[in,out] on_error  Callback when an error occured.
 *  \param[in,out] on_close  Callback when the connection is terminated.
 *  \param[in]     aux       Auxiliary data to pass to the callbacks.
 */
void multiplex_add_d9c_stdio
        (void (*on_attach) (struct d9r_io *, void *),
         void (*on_error)  (struct d9r_io *, const char *, void *),
         void (*on_close)  (struct d9r_io *, void *),
         void *aux);

/*! \brief Open File for Reading over 9P.
 *  \param[in,out] io   The 9P connection to open a file on.
 *  \param[in]     path The file to open.
 *  \return New File I/O structure.
 *
 *  You should always use the multiplexer to handle the resulting I/O structure,
 *  direct methods may not work properly.
 */
struct io *io_open_read_9p
        (struct d9r_io *io, const char *path);

/*! \brief Open File for Writing over 9P.
 *  \param[in,out] io   The 9P connection to open a file on.
 *  \param[in]     path The file to open.
 *  \return New File I/O structure.
 *
 *  You should always use the multiplexer to handle the resulting I/O structure,
 *  direct methods may not work properly.
 */
struct io *io_open_write_9p
        (struct d9r_io *io, const char *path);

/*! \brief Create File and open for Writing over 9P.
 *  \param[in,out] io   The 9P connection to open a file on.
 *  \param[in]     path The directorey to create the file in.
 *  \param[in]     file The file to create.
 *  \param[in]     mode The mode to create the file with.
 *  \return New File I/O structure.
 *
 *  You should always use the multiplexer to handle the resulting I/O structure,
 *  direct methods may not work properly.
 */
struct io *io_open_create_9p
        (struct d9r_io *io, const char *path, const char *file, int mode);

#if 0

/* not implemented */

struct io *d9c_stat
        (struct d9r_io *io, const char *,
         void (*Rstat) (int_16, int_32, struct d9r_qid, int_32, int_32, int_32,
                        int_64, char *, char *, char *, char *, char *, void *),
         void *);

#endif

#endif

#ifdef __cplusplus
}
#endif

/*! @} */
