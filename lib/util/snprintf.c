#include "util.h"
#ifndef HAVE_SNPRINTF
#  ifdef HAVE_VARARGS_H
#  include <varargs.h>

/* Copyright (C) 1991, 1995 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/*#include <stdarg.h>*/
#include <stdio.h>

int
snprintf
#ifdef __STDC__
  (char *string, /*_IO_*/size_t maxlen, const char *format, ...)
#else
(string, maxlen, format, va_alist)
     char *string;
/*     _IO_size_t maxlen;*/
     size_t maxlen;
     char *format;
     va_dcl
#endif
{
  int ret;
  va_list args;
/*  _IO_va_start(args, format);*/
  va_start(args);
#ifdef HAVE_VSNPRINTF
  ret = vsnprintf(string, maxlen, format, args);
#else
  ret = vsprintf(string, format, args);
#endif
  va_end(args);
  return ret;
}
#  endif/* HAVE_VARARGS_H */
#endif /* HAVE_SNPRINTF */
