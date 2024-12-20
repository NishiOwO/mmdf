#include "util.h"
#include "mmdf.h"

int	*regfdary;
int	numfds = 16;	/* init to 16 as a failsafe value, -DPK- */

mmdf_fdinit()
{
	register	int	i;

#ifdef HAVE_GETDTABLESIZE
	numfds = getdtablesize();
#else	/* HAVE_GETDTABLESIZE */
#  ifdef _NFILE
	numfds = _NFILE;
#  endif	/* _NFILE */
#endif	/* HAVE_GETDTABLESIZE */
	/*NOSTRICT*/
	regfdary =(int *)malloc(numfds * sizeof(int));
	regfdary[0] = 0; 
	regfdary[1] = 1;
	for (i = numfds-1; i > 1; i--)
		regfdary[i] = NOTOK;
}
