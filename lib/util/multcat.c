#include "util.h"
#ifdef HAVE_VARARGS_H
#include <stdarg.h>

/*                                                                      */
/*              Concatenate a series of arrays                          */

/*VARARGS0*/
char *multcat (char* str, ...)
{
	va_list ap;
	register char  *oldstr, *ptr;
	char    *newstr;
	unsigned  newlen;

	va_start(ap, str);
	for (newlen = 1; oldstr = va_arg(ap, char *);)
		newlen += strlen (oldstr);
	va_end(ap);

	if ((ptr = newstr = malloc (newlen+1)) == 0)
	    return((char *)0);

	va_start(ap, str);
	for (; oldstr = va_arg(ap, char *); ptr--)
		while(*ptr++ = *oldstr++);
	va_end(ap);

	return (newstr);
}
#endif/* HAVE_VARARGS_H */
