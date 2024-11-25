/*
**		S _ A R G I N I T
**
**  This function parses and uses the information found on the command
**  line.
**
**
**	R E V I S I O N  H I S T O R Y
**
**	03/31/83  GWH	Split the SEND program into component parts
**
*/

#define DEFINE_STUFFS
#include "./s.h"
#include "./s_externs.h"

char *UsageMessage = 
"Usage:  snd addrs -a hdr:val -{bct} addrs -f file -h host -s subj -n\n";

arginit (argc, argv)              /* parse and use the argument list    */
int     argc;
char   *argv[];
{
    char *curfld;
    char *cp;
    register short curarg;

/*  strcpy (host, locname); */  /* setup default host reference */
#ifdef JNTMAIL
	snprintf(host, sizeof(host), "%s.%s",ap_dmflip(locdomain),locname );
#else
	snprintf(host, sizeof(host), "%s.%s", locname, locdomain );
#endif

/*  if an argument is not a switch, assume that it is an address.
 *  several of the switches merely change which header addresses are
 *  to be added to.  after an argument is processed, the pointer to
 *  it is nulled, so that it will no longer be visible by a system-
 *  status command ("ss" on the UDel machine).
 */

    for (curfld = to, curarg = 1; curarg < argc; )
    {                             /* regular scan of arg list           */
				  /* default to adding to To field      */
	if (argv[curarg][0] != '-')
	    aliasmap(curfld, argv[curarg], host);
				  /* add it to current field            */
	else {                    /* a switch                           */ 
	    switch (argv[curarg][1])
	    {
	    	case 'a':
	    	    if (++curarg >= argc)
	    		break;
	    	    if (cp = strchr(argv[curarg], ':'))
	    		*cp++ = '\0';
		    addheader(argv[curarg], cp);
		    break;

		case 'b':         /* add to end of bcc field            */
		    curfld = bcc;
		    break;

	    	case 'd':	  /* grandfather the direct edit option */
	    	    break;	  /* NOOP */
	    	    
		case 'c':         /* add to end of cc field             */
		    ccflag = FALSE;
		    curfld = cc;
		    break;

		case 'f':         /* add file to end of message body    */
	    	    if (++curarg >= argc)
	    		break;
		    strncpy (inclfile, argv[curarg], sizeof(inclfile));
		    break;        /* file is named in next argument     */

		case 'h':         /* default hostname for addresses     */
	    	    if (++curarg >= argc)
	    		break;
		    strncpy (host, argv[curarg], sizeof(host));
		    break;        /* host is named in next argument     */

		case 'n':	  /* send compatibility */
		    break;

		case 's':         /* add to end of Subject field        */
		    subjflag = FALSE;
	    	    if (++curarg >= argc)
	    		break;
		    strncat (subject, argv[curarg],
				(S_BSIZE - strlen(subject) - 2));
		    break;        /* Subject is contained in next arg   */

		case 't':         /* add to end of To field             */
		    toflag = FALSE;
		    curfld = to;
		    break;

		default:
		    snd_abort ("unknown flag in '%s'\n%s", argv[curarg],
				UsageMessage);
	    }
	}
	if (argv[curarg])
		argv[curarg++][0] = '\0';
    }
    if (to[0] != '\0')            /* at least one To was specified      */
	toflag = FALSE;           /* so don't prompt for it             */
}
