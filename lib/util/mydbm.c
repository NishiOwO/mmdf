/*
 * $Id: mydbm.c,v 1.2 1998/09/18 11:39:45 krueger Exp $
 *
 * When using the gdbm-library, gdbm provides an dbminit() functions that
 * always open the database in RW-mode and locks it. So no other program
 * can access to it. 
 */

#include "config.h"

#ifdef HAVE_LIBGDBM
#include <gdbm.h>
static GDBM_FILE dbf;
static datum curkey;

int mydbminit(name, mode)
char *name;
char *mode;
{
  int filemode=10644;
  
  /* printf("dbminit(%s, %s, %d)\n", name, mode, filemode); */
  
  if(!strcmp(mode, "r"))
    dbf = gdbm_open (name, 0, GDBM_READER, filemode, (void *)0);
  
  if(!strcmp(mode, "rw"))
    dbf = gdbm_open (name, 0, GDBM_WRITER|GDBM_READER, filemode, (void *)0);

  /* if(gdbm_errno) printf("gdbm_open: [%d] %s\n", gdbm_errno, */
/*                         gdbm_strerror(gdbm_errno)); */
  
  return -gdbm_errno;
}

int mystore(key, content)
datum key, content;
{
  return(gdbm_store(dbf, key, content, GDBM_REPLACE));
}

datum myfetch(key, content)
datum key, content;
{
  return(gdbm_fetch(dbf, key));
}

datum myfirstkey()
{
  return(gdbm_firstkey(dbf));
}

datum mynextkey(datum key)
{
  return(gdbm_nextkey(dbf, key));
  
/*   datum newkey = gdbm_nextkey(dbf, curkey); */
/*   curkey = newkey; */
  
/*   return(curkey); */
}

mydbmclose()
{
  gdbm_close(dbf);
}

#else /* HAVE_GDBM */
#include <ndbm.h>
extern datum fetch(),
  firstkey(),
  nextkey();

int mydbminit(file, mode)
char *file,
     *mode;
{
  return(dbminit(file));
}

int mystore(key, content)
datum key, content;
{
  return(store (key, content));
}

datum myfetch(key)
datum key;
{
  return(fetch (key));
}

datum myfirstkey()
{
  return(firstkey());
}

datum mynextkey(key)
datum key;
{
  return(nextkey(key));
}

mydbmclose()
{
  return(dbmclose());
}

#endif /* HAVE_GDBM */
