/* lock-tokens-table.h : internal interface to ops on `lock-tokens' table
 *
 * ====================================================================
 * Copyright (c) 2000-2004 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

#ifndef SVN_LIBSVN_FS_LOCK_TOKENS_TABLE_H
#define SVN_LIBSVN_FS_LOCK_TOKENS_TABLE_H

#include "svn_fs.h"
#include "svn_error.h"
#include "../trail.h"
#include "../fs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Open a `lock-tokens' table in ENV.  If CREATE is non-zero, create
   one if it doesn't exist.  Set *LOCK_TOKENS_P to the new table.
   Return a Berkeley DB error code.  */
int svn_fs_bdb__open_lock_tokens_table (DB **locks_tokens_p,
                                        DB_ENV *env,
                                        svn_boolean_t create);


/* Add a lock-token to the `lock-tokens' table in FS, as part of TRAIL. 
   Use PATH as the key and LOCK_TOKEN as the value.

   Warning: if PATH already exists as a key, then its value will be
   overwritten. */
svn_error_t *
svn_fs_bdb__lock_token_add (svn_fs_t *fs,
                            const char *path,
                            const char *lock_token,
                            trail_t *trail);


/* Remove the lock-token whose key is PATH from the `lock-tokens'
   table of FS, as part of TRAIL.

   Return SVN_ERR_FS_NO_SUCH_LOCK if PATH does not exist as a table key. */
svn_error_t *
svn_fs_bdb__lock_token_delete (svn_fs_t *fs,
                               const char *path,
                               trail_t *trail);


/* Retrieve the lock-token *LOCK_TOKEN_P pointed to by PATH from the
   `lock-tokens' table of FS, as part of TRAIL.  Perform all
   allocations in TRAIL->pool.

   Return SVN_ERR_FS_NO_SUCH_LOCK if PATH does not exist as a table key. */
svn_error_t *
svn_fs_bdb__lock_token_get (const char **lock_token_p,
                            svn_fs_t *fs,
                            const char *path,
                            trail_t *trail);


/* Retrieve a hash of lock-tokens in *LOCK_TOKENS_P, representing all
   locks that exist at or below PATH in FS.  Do all work in TRAIL, and
   do all allocations in TRAIL->pool.  If no locks exist below PATH,
   return an empty hash. */
svn_error_t *
svn_fs_bdb__lock_tokens_get (apr_hash_t **lock_tokens_p,
                             svn_fs_t *fs,
                             const char *path,
                             trail_t *trail);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_LIBSVN_FS_LOCK_TOKENS_TABLE_H */
