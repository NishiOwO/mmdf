/*
 * $Id: table_test.c,v 1.3 2001/04/26 22:02:13 krueger Exp $
 *
 *  This program ist used to check 
 */

#include "util.h"
#include "mmdf.h"
#include "ch.h"
#include "dm.h"

extern Table **tb_list;
extern int Tdebug;


do_channel_get(hostr)
char *hostr;
{
  Chan *chanptr = NULL;
  int pos=0;
  
  printf("do_channel_get(%s)\n", hostr);
  chanptr = ch_h2chan (hostr, pos);
  if(chanptr != (Chan *)NOTOK) {
    if(chanptr == (Chan *)MAYBE)
      printf("Return maybe\n");
    else if (chanptr == (Chan *)OK)
      printf("Return maybe\n");
    else printf("%-5sChannel: %s\n", "", chanptr->ch_name);
  }
  
}

do_domain_get(subdomain)
char *subdomain;
{
  Domain *dmptr = NULL;
  char official[LINESIZE];
  char dmbuf[LINESIZE];
  
  printf("do_domain_get(%s)\n", subdomain);
  dmptr = dm_s2dom (subdomain, official, dmbuf);
  if(dmptr!= (Domain *)NOTOK) {
    if(dmptr == (Domain *)MAYBE)
      printf("Return maybe\n");
    else if (dmptr == (Domain *)OK)
      printf("Return OK\n");
    else {
      printf("%-5sDomain: %s\n", "", dmptr->dm_name);
      printf("%-5s      : %s\n", "", official);
      printf("%-5s      : %s\n", "", dmbuf);
    }
  }
  
}

do_table_get(key)
char *key;
{
  int ind;
  int first=1;
  char value[LINESIZE];
  Table *tblptr;
  
  for (ind = 0; (tblptr = tb_list[ind]) != (Table *) 0; ind++) {
    first = 1;
    printf("Checking table '%s' %s\n", tblptr -> tb_name, tblptr -> tb_show);
    
    while( tb_k2val(tblptr, first, key, value) != NOTOK){
      first=0;
      printf("%-10sRES='%s'\n", "", value);
    }
  }
  return;
}

main(argc, argv)
int argc;
char *argv[];
{
  char key[LINESIZE];
  char buf[LINESIZE];
  int flags;
  
  Tdebug = 2;
  
  if(argc!=2) {
    printf("usage: %s key\n", argv[0]);
    return 1;
  }
  strcpy(key, argv[1]);
  mmdf_init(argv[0], 0);

  aliasfetch(TRUE, key, buf, &flags);
  printf("%-10sRES='%s'\n", key, buf);
  do_table_get(key);
  do_channel_get(key);
  do_domain_get(key);
  
    
  return 0;
}

