#include "util.h"
#include "cmd.h"

extern int errno;

cmdbsrch (str, argc, cmd, entries)        /* binary version of cmdsrch */
    char *str;                  /* test string  */
    int argc;                   /* number of available arguments        */
    Cmd cmd[];                  /* table of known commands */
    int entries;		/* size of cmd table */
{
    register int hi, lo, mid;
    register int diff;

    hi = entries-1;
    lo = 0;

    for (mid=(hi+lo)/2 ; hi >= lo; mid=(hi+lo)/2)
    {
	if ((diff = lexrel(str,cmd[mid].cmdname))==0)
	{
	    if (argc < cmd[mid].cmdnargs)
	    {
		errno = EINVAL;
		return(NOTOK);
	    }
	    return(cmd[mid].cmdtoken);
	}

	if (diff < 0)
	    hi = mid - 1;
	else
	    lo = mid+1;
    }

    return(NO);
}
