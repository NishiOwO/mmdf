/*
 * $Id: mydbm.c,v 1.4 2002/09/30 19:28:40 krueger Exp $
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

int mydelete(key)
datum key;
{
  return(gdbm_delete (dbf, key));
}

mydbmclose()
{
  gdbm_close(dbf);
}

#else /* HAVE_GDBM */
#include <sys/fcntl.h>
#include <ndbm.h>
#ifdef HAVE_DBM_OPEN
static DBM *dbf;
extern int errno;

int mydbminit(file, mode)
char *file,
     *mode;
{
  int filemode=10644;
  
  if(!strcmp(mode, "r"))
    dbf = dbm_open (file, DBM_RDONLY, filemode);
  
  if(!strcmp(mode, "rw"))
    dbf = dbm_open (file, O_RDWR, filemode);

  if(dbf==0) return -errno;
  return 0;
}

int mystore(key, content)
datum key, content;
{
  return(dbm_store (dbf, key, content,1));
}

datum myfetch(key)
datum key;
{
  return(dbm_fetch (dbf, key));
}

datum myfirstkey()
{
  return(dbm_firstkey(dbf));
}

datum mynextkey(key)
datum key;
{
  return(dbm_nextkey(dbf));
}

int mydelete(key)
datum key;
{
  return(dbm_delete (dbf, key));
}

mydbmclose()
{
  dbm_close(dbf);
  return 0;
}
#else /* HAVE_DBM_OPEN */
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

int mydelete(key)
datum key;
{
  return(dbmdelete (key));
}

mydbmclose()
{
  return(dbmclose());
}
#endif /* HAVE_DBM_OPEN */
#endif /* HAVE_GDBM */
