#include <stdio.h>
#include "util.h"
#include "mmdf.h"
#include "ap.h"

#define BSIZE		1024

struct ALIAS {
	char *key;
	int cntr;
	char *names;
	struct ALIAS *next_int;
} *als, alias_ptr, *old_als;

int aflag;
char *adrptr;

void aliasinit( aliasfile )
char *aliasfile;
{
	char alsfilename[128];
	FILE *afd;
	char *akey, *list;
	char tmpbuf[BSIZE];
	aflag = 1;

	(void) strncpy( alsfilename, aliasfile, sizeof(alsfilename));
		/*
		**  Open the alias file and store the keys and lists
		**  in core
		*/

	afd = fopen( alsfilename, "r" );
	if( afd <= (FILE *)NULL ) {
		aflag = 0;
		perror(" Can't open alias file ");
		return;
	}
	als = &alias_ptr;
	old_als = NULL;
	while(1) {
		if(fgets(tmpbuf, sizeof(tmpbuf), afd) != NULL) {
			get_key(tmpbuf, &akey, &list);
			if ( old_als > (struct ALIAS *)NULL ) {
				if(( als = 
				    (struct ALIAS *) malloc(sizeof(alias_ptr))) 
						<= (struct ALIAS *)NULL ) {
					perror( "MALLOC ERROR!");
					break;
				}
				old_als->next_int = als;
			}
			als->key = akey;
			als->cntr = 0;
			als->names = list;
			old_als = als;
		}
		else 
		{
			old_als = NULL;
			break;
		}
	}
}

int linelen, destlen, addrlen;

void aliasmap(dest, src, thehost)
char *dest, *src, thehost[];
{
    char addr[BSIZE],
	 name[ADDRSIZE],
	 address[ADDRSIZE],
	 temphost[FILNSIZE],
	 defhost[FILNSIZE],
	 hostname[FILNSIZE];
    char *ptr;
    char *sadrptr;
    int sub_flag;

    if (thehost != 0)                /* save official version of default   */
	(void) strncpy (defhost, thehost, sizeof(defhost));

    for (ptr = dest, linelen = 0; *ptr != '\0'; ptr++, linelen++)
	if (*ptr == '\n')         /* get length of last line of header  */
	    linelen = -1;         /* make it zero for next character    */
    destlen = ptr - dest;

    for (adrptr = src;; )         /* adrptr is state variable, used in  */
    {                             /*   next_address                     */
	switch (m_next_address (addr))
	{
	    case -1:              /* all done                           */

		/* clean markers in aliases structure */
		als = &alias_ptr;
		while(als != NULL ){
			als->cntr = 0;
			als = als->next_int;
		}
		return;

	    case 0:               /* empty address */
		continue;
	}

	name[0] = address[0] = temphost[0] = '\0';

	parsadr (addr, name, address, temphost);
	if (address[0] == '\0')
	{                         /* change the default host */
	    if (temphost[0] != '\0')
		(void) strncpy (defhost, temphost, sizeof(defhost));
	    continue;
	}
	if( aflag && temphost[0]==0){
		als = &alias_ptr;
		sub_flag = 0;
		while( als != NULL ){
			if( lexequ(address, als->key) && als->cntr == 0 ){
				als->cntr++;
				sadrptr = adrptr;
				aliasmap(dest, als->names, thehost);
				adrptr = sadrptr;
				sub_flag = 1;
				break;
			}
			else
				if( lexequ(address, als->key) && als->cntr != 0 ){
					sub_flag = 1;
					break;
				}
			als = als->next_int;
		}
		if(sub_flag == 1)
			continue;
	}
	if (temphost[0] == '\0')
	    (void) strncpy (hostname, defhost, sizeof(hostname));
	else
	    (void) strncpy (hostname, temphost, sizeof(hostname));

	if (name[0] != '\0')      /* put into canonical form            */
	{                         /* name <mailbox@host>                */
	    for (ptr = hostname; !isnull (*ptr); ptr++)
		*ptr = uptolow (*ptr);
				  /* force hostname to lower            */
	    for (ptr = name; ; ptr++)
	    {                     /* is quoting needed for specials?    */
		switch (*ptr)
		{
		    default:
			continue;

		    case '\0':    /* nope                               */
			snprintf (addr, sizeof(addr), "%s <%s@%s>",
				name, address, hostname);
			goto doaddr;

		    case '<':     /* quoting needed                     */
		    case '>':
		    case '@':
		    case ',':
		    case ';':
		    case ':':
			snprintf (addr, sizeof(addr), "\"%s\" <%s@%s>",
				    name, address, hostname);
			goto doaddr;
		}
	    }
	}
	else
	{
	    snprintf (addr, sizeof(addr), "%s@%s", address, hostname);
	}

doaddr:
	addrlen = strlen (addr);

	if ((destlen + addrlen) < (BSIZE - 4))
	{                         /* it will fit into dest buffer       */
	    if (destlen == 0)
		(void) strcpy (dest, addr);
	    else          /* monitor line length                */
	    {
		if ((linelen + addrlen) < 70)
		    sprintf (&dest[destlen], ", %s", addr);
		else
		{
		    sprintf (&dest[destlen], ",\n %s", addr);
		    linelen = 0;
		    destlen += 1;
		}
		destlen += 2;
		linelen += 2;
	    }
	    destlen += addrlen;
	    linelen += addrlen;
	}
    	else
    	{
    		fprintf(stderr,"WARNING: Header line exceeds buffer size!\n");
    		fprintf(stderr,"\tIt was truncated to legal length.\n");
    		break;
    	}
    }
}

int get_key( s, okey, olist )
char *s, **okey, **olist;
{
	char tmps[512];
	char *ptmps;
	int len_list;
/*
**  This routine takes the input string s and divides it into two parts,
**  the first of which consists of the first word in the original string.
**  It is returned in okey. The second part is a list of the remaining names.
**  They are returned in the olist. Malloc is used to allocate the necessary
**  storage for the sublists.
*/

	/* throw away initial white space */

	while( *s == ' ' || *s == '\t' )
		*s++;

	/* get the first word. It may be terminated by spaces, tabs, commas, */
	/* newlines or end-of-file 					     */

	ptmps = tmps;
	while( *s != ' ' && *s != '\t' && *s != ',' && *s != '\n' && *s != EOF){
		*ptmps++ = *s++;
	}
	*ptmps = 0;
	len_list = strlen(tmps);
	if(( *okey = malloc( (unsigned) (len_list) + 1 )) <= (char *)NULL ) {
		perror("MALLOC ERROR!");
		return(-1);
	}

	(void) strcpy( *okey, tmps );

	/* and now the rest of the list */

	ptmps = s;
	while( *s != '\n' && *s != EOF && *s != '\0' )
		*s++;
	*s = 0;
	len_list = strlen( ptmps );
	if(len_list <= 0 )
		return( -1 ); /* no list - ERROR - ERROR */
	if((*olist = malloc( (unsigned) (len_list) +1 )) <= (char *)NULL) {
		perror("MALLOC ERROR!");
		return( -1 );
	}

	(void) strcpy( *olist, ptmps );
	return(0);
}

int m_next_address (addr)
char    *addr;
{
    int     i = -1;               /* return -1 = end; 0 = empty         */

    for (;;)
	switch (*adrptr)
	{
	    default: 
		addr[++i] = *adrptr++;
		continue;

	    case '"':
		for (addr[++i] = *adrptr++; !isnull (*adrptr); )
		{
		    addr[++i] = *adrptr;
		    if (*adrptr++ == '"')
			break;
		}
		continue;

	    case '(':
		for (addr[++i] = *adrptr++; !isnull (*adrptr); )
		{
		    addr[++i] = *adrptr;
		    if (*adrptr++ == ')')
			break;
		}
		continue;

	    case '\n':
	    case ',': 
		addr[++i] = '\0';
		adrptr++;
		return (i);

	    case '\0': 
		if (i >= 0)
		    addr[++i] = '\0';
		return (i);
	}
}
