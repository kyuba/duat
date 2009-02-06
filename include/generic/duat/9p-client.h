/*
 *  9p-client.h
 *  libduat
 *
 *  Created by Magnus Deininger on 24/11/2008.
 *  Copyright 2008, 2009 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, 2009, Magnus Deininger All rights reserved.
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

#ifndef DUAT_9P_CLIENT_H
#define DUAT_9P_CLIENT_H

#include <duat/9p.h>

void multiplex_d9c ();

void multiplex_add_d9c_io
        (struct io *, struct io *,
         void (*) (struct d9r_io *, void *),
         void (*) (struct d9r_io *, const char *, void *),
         void *);
void multiplex_add_d9c_socket
        (const char *,
         void (*) (struct d9r_io *, void *),
         void (*) (struct d9r_io *, const char *, void *),
         void *);
void multiplex_add_d9c_stdio
        (void (*) (struct d9r_io *, void *),
         void (*) (struct d9r_io *, const char *, void *),
         void *);

struct io *io_open_read_9p
        (struct d9r_io *io, const char *);
struct io *io_open_write_9p
        (struct d9r_io *io, const char *);
struct io *io_open_create_9p
        (struct d9r_io *io, const char *, const char *, int);

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
