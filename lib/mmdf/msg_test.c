/*
 *  Test program for reading addresse-file
 */

#include "util.h"
#include "mmdf.h"
#include <signal.h>
#include <sys/stat.h>
#include "ch.h"
#include "msg.h"
#include "adr_queue.h"

extern char *quedfldir;
extern char *tquedir;
extern char	*aquedir;

main()
{
  Msg  themsg;
  struct adr_struct theadr;
  Chan *curchan;
  char retadr[LINESIZE];

  if (chdir (quedfldir) < OK) {
	printf ("couldn't chdir to tquedir\n");
	return (-1);
  }

  printf("aquedir=%s\n", aquedir);
  
  (void) strncpy (themsg.mg_mname, "msg.aa05152", sizeof(themsg.mg_mname));
  if (mq_rinit ((Chan *) 0, &themsg, retadr) != OK) {
    printf("Cannot read '%s'\n", themsg.mg_mname);
    return 1;
  }
  printf("mg_mname    : %s\n",   themsg.mg_mname);
  printf("mg_time     : %ld %s", themsg.mg_time, ctime(&themsg.mg_time));
  printf("mg_large    : 0x%02x\n",   themsg.mg_large);
  printf("mg_stat     : 0x%02x\n",   themsg.mg_stat);
#ifdef HAVE_ESMTP_DSN
  printf("mg_dsn_ret  : %d\n", themsg.mg_dsn_ret);
  printf("mg_dsn_envid: %s\n", themsg.mg_dsn_envid);
#endif /* HAVE_ESMTP_DSN */

  printf("retaddr     : %s\n", retadr);
  
  while(mq_radr (&theadr) == OK) {
    printf("adr_tmp    : %c\n", theadr.adr_tmp);
    printf("adr_delv   : %c\n", theadr.adr_delv);
    printf("adr_fail   : %c\n", theadr.adr_fail);
    printf("adr_que    : %s\n", theadr.adr_que);
    printf("adr_host   : %s\n", theadr.adr_host);
    printf("adr_local  : %s\n", theadr.adr_local);
    printf("adr_buf    : %s\n", theadr.adr_buf);
    printf("adr_pos    : %ld\n", theadr.adr_pos);
  }
  mq_rkill (OK);
}
