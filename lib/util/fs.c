/*
 * $Id: fs.c,v 1.1 1999/08/24 14:10:03 krueger Exp $
 *
 * $Log: fs.c,v $
 * Revision 1.1  1999/08/24 14:10:03  krueger
 * Added fs.c for checking disk-space
 *
 *
 */

#ifdef __MAIN__
#  define HAVE_STATFS 1
#else /* __MAIN__ */
#  include "util.h"
#  include "ll_log.h"
#endif /* __MAIN__ */

#include <sys/types.h>

#ifdef HAVE_STATFS
#  ifdef HAVE_SYS_STATVFS_H
#    include <sys/statvfs.h>
#    define STATVFS statvfs
#    define F_FRSIZE f_frsize
#  else /* HAVE_SYS_STATVFS_H */
#    define STATVFS statfs
#    define F_FRSIZE f_bsize
#    ifdef HAVE_SYS_VFS_H
#      include <sys/vfs.h>
#    ifdef HAVE_SYS_STATFS_H
#      include <sys/statfs.h>
#    endif /* HAVE_SYS_STATFS_H */
#    endif /* HAVE_SYS_VFS_H */
#    ifdef HAVE_SYS_MOUNT_H
#      include <sys/mount.h>
#    endif /* HAVE_SYS_MOUNT_H */
#  endif /* HAVE_SYS_STATVFS_H */
#  ifndef F_BAVAIL
#    define F_BAVAIL f_bavail
#  endif /* F_BAVAIL */
#  ifndef F_FAVAIL
#    define F_FAVAIL f_ffree
#  endif /* F_FAVAIL */
#endif /* HAVE_STATFS */


#ifdef __MAIN__
int main(
  argc, argv)
int argc;
char *argv[];
{
  
  check_disc_space(0, argv[1]);
  
  return 0;
}
#endif /* __MAIN__ */


#ifdef __STDC__
int check_disc_space(
  int msg_size,
  char *spool_directory
  )
#else
int check_disc_space(
  msg_size,
  spool_directory
  )
int msg_size;
char *spool_directory;
#endif
{
  int rc=1;

  if(spool_directory==NULL) {
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "check_disc_space(%d,(null))",
            msg_size);
#endif
    return 0;
  }
  
#ifdef HAVE_STATFS
  struct STATVFS statbuf;

  memset(&statbuf, 0, sizeof(statbuf));
  if( (STATVFS(spool_directory, &statbuf) != 0) ||
      ( statbuf.F_BAVAIL  < (msg_size + min_spool_free) / statbuf.F_FRSIZE) ||
      ( statbuf.F_FAVAIL < min_inode_free ) ) rc = 0;
#ifdef DEBUG
    ll_log (logptr, LLOGBTR,
            "check_disc_space(%d,%s), space = %d blocks, inodes = %d",
            msg_size, spool_directory,
            statbuf.F_BAVAIL, statbuf.F_FAVAIL);
#endif
#if 0  
  printf("# f_bavail= %ld\n", statbuf.F_BAVAIL * statbuf.F_FRSIZE);
  printf("# f_favail  = %ld\n", statbuf.F_FAVAIL);
#endif
  
#endif /* HAVE_STATFS */
  return rc;
}
