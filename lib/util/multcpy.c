#include "util.h"
#ifdef HAVE_VARARGS_H
#include <stdarg.h>

/*                                                                      */
/*      Copy the list of strings to the given location.                 */
/*                                                                      */

char *
multcpy (register char* to, ...)
{
	va_list ap;
	register char   *from;

	va_start(ap, to);

	while(from = va_arg(ap, char *)){
		while(*to++ = *from++);
		to--;
	}
	va_end(ap);
	return (to);            /* return pointer to end of str         */
}

#endif/* HAVE_VARARGS_H */
