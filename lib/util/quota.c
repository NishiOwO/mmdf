/*
 * $Id: quota.c,v 1.1 1999/09/08 07:58:58 krueger Exp $
 *
 * $Log: quota.c,v $
 * Revision 1.1  1999/09/08 07:58:58  krueger
 * Added file for quota-support
 *
 *
 */
#if 0
#include <linux/quota.h>
{
  
  struct dqblk blk;
  ret = quotactl(QCMD(Q_SYNC, GRPQUOTA), argv[1], 0, (caddr_t)0);
  ret = quotactl(QCMD(Q_GETQUOTA, GRPQUOTA), argv[1], grp->gr_gid, 
                   (caddr_t)(&blk));
  printf("%-8s %6u %6u %6u\n", grp->gr_name,
         blk.dqb_curblocks, blk.dqb_bsoftlimit, 
         blk.dqb_bhardlimit);

  ret = quotactl(QCMD(Q_SYNC, USRQUOTA), argv[1], 0, (caddr_t)0);
  ret = quotactl(QCMD(Q_GETQUOTA, USRQUOTA), argv[1], pw->pw_uid, 
                 (caddr_t)(&blk));
  printf("%-8s %6u %6u %6u\n", pw->pw_name,
         blk.dqb_curblocks, blk.dqb_bsoftlimit, 
         blk.dqb_bhardlimit);
  _syscall4(int, quotactl, int, cmd, const char *, special, int, id, caddr_t, addr);
}
       int getrlimit (int resource, struct rlimit *rlim);
       int getrusage (int who, struct rusage *usage);
       int setrlimit (int resource, const struct rlimit *rlim);

#endif
