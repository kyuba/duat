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

/*! \defgroup Duat9PServer 9P2000(.u) Server
 *  \ingroup Duat9P
 *
 *  Simple 9P server.
 *
 *  @{
 */

/*! \file
 *  \brief Duat 9P2000(.u) Server Header
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DUAT_9P_SERVER_H
#define DUAT_9P_SERVER_H

#include <duat/filesystem.h>

/*! \brief Initialise 9P Server Multiplexer */
void multiplex_d9s ();

/*! \brief Serve a VFS Tree on two Generic IO Structures
 *  \param[in]     in     The input structure.
 *  \param[in]     out    The output structure.
 *  \param[in,out] root   The filesystem root to serve.
 */
void multiplex_add_d9s_io (struct io *in, struct io *out, struct dfs *root);

/*! \brief Serve a VFS Tree on a Socket
 *  \param[in]     socket The socket to serve on.
 *  \param[in,out] root   The filesystem root to serve.
 */
void multiplex_add_d9s_socket (char *socket, struct dfs *root);

/*! \brief Serve a VFS Tree on Standard I/O
 *  \param[in,out] root   The filesystem root to serve.
 */
void multiplex_add_d9s_stdio (struct dfs *root);

#endif

#ifdef __cplusplus
}
#endif

/*! @} */
