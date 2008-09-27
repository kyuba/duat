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

struct duat_filesystem;

void dfs_add_extended ();
void dfs_add_simple ();

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
