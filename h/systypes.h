/*
 * systypes.h -- the sole existance of this file is so that we can
 * include <sys/types.h> as many times as we want without generating
 * compiler warnings about things being redefined.  Usually the
 * developers of OS distributions do #ifndef tricks for us so that
 * we don't have to do it ourselves.  And, indeed, Sun does play
 * this game in their files, but other distributions don't.  Namely
 * SysV r3.  [DSH]
 *
 */
#ifndef __SYSTYPESH__
#define __SYSTYPESH__
#include <sys/types.h>
#endif
