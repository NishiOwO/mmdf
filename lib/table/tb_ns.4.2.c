/*
 *      TB_NS.C
 *
 * Original version by Phil Cockcroft to use MF and MD records.
 * Also had several speed improvements such as caching.  Later
 * reworked by Phil to do MX lookups.
 *
 * Mostly rewritten by Craig Partridge to move routing decisions
 * into SMTP channel.
 *
 */

#include "util.h"
#include "mmdf.h"
#include "ch.h"
#include "ns.h"
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

extern char *strncpy();
extern int   h_errno;

/* T_UNSPEC was defined only in more recent versions of BIND */

#ifdef T_UNSPEC
#define BSD4_3
#define	getshort _getshort
#endif T_UNSPEC

/*
 * there turn out to be some servers that return FORMERR to MX queries!?
 */

#undef ROBUST

/*
 * if you want caching, define NSCACHE.  Make it a prime number!
 */

#define NSCACHE 101

/*
 * next definition should go away as all servers use MX
 * right now if they don't we just try the address.  MF and MD are
 * dead!
 */

#define OLDSERVER

#ifndef MAXADDR
#define MAXADDR		30
#endif

#ifndef MAXADDR_PER
#define MAXADDR_PER	6
#endif

#ifndef MAXDATA
#define MAXDATA (4 * PACKETSZ)		/* tcp tried after udp */
#endif

#ifndef MAXMX
#define MAXMX	(MAXADDR)		/* shouldn't be < MAXADDR */
#endif

extern  struct  ll_struct *logptr;
extern  char    *strdup(), *strcpy();
extern  char *locfullmachine, *locfullname;

union ansbuf {			/* potentially huge */
    HEADER ab1;
    char ab2[MAXDATA];
}; 

union querybuf {		/* just for outbound stuff */
    HEADER qb1;			/* didn't want to clobber stack */
    char qb2[2 * MAXDNAME];
}; 

LOCVAR  struct in_addr mx_addrs[MAXADDR];	/* cache of MX addrs */
LOCVAR  int max_mxa= -1, on_mxa= -1;	/* indicies into cache of MX addrs */
LOCVAR  char dn_name[MAXDNAME];
LOCVAR  char local[MAXDNAME];

#ifdef NSCACHE
struct ns_cache {
    int nc_rval;		/* OK/NOTOK/MAYBE */
    int nc_count;
    char *nc_key;
    char *nc_data;
    struct ns_cache *nc_next;
};

LOCVAR  struct ns_cache *dmncache[NSCACHE];
LOCVAR  struct ns_cache *chncache[NSCACHE];
#endif

/*
 * table fetch routine for ns
 */

ns_fetch(table, key, value, first)
Table   *table;          /* What "table" are we searching */
char    *key;
char    *value;         /* Where to put result */
int     first;          /* now used */
{
    register char *tmp;
    int	type;
    int rval;

#ifdef  DEBUG
    ll_log(logptr, LLOGFTR, "ns_fetch (%o, %s, %d)",
	table->tb_flags, key, first);
    ll_log(logptr, LLOGFTR, "ns_fetch: timeout (%d), rep (%d), servers (%d)",
	_res.retrans, _res.retry, _res.nscount);
#endif

	/* _res.options |= ((int) RES_DEBUG); /* In case it's needed again -- DSH */

    if (value == 0)
    {
	ll_log(logptr,LLOGFAT,"null buffer passed to ns_fetch()");
	return(NOTOK);
    }

    /* so we don't return garbage on errors */
    *value = '\0';

    type = (table->tb_flags & TB_TYPE);

    if (first)
    {
	max_mxa = on_mxa = -1;

	if (!cachehit(key,&rval,type))
	{
	    if (type == TB_CHANNEL)
	    {
		if ((rval = ns_getmx(key, &max_mxa, mx_addrs, MAXADDR))==OK)
		    on_mxa = 0;

#ifdef NSCACHE
		cachechn(key,rval,max_mxa,mx_addrs);
#endif
	    }	
	    else
	    {
		rval = ns_getcn(key, dn_name, sizeof(dn_name));
#ifdef NSCACHE
		cachedmn(key,rval,dn_name);
#endif
	    }
	}

	if (rval != OK)
	{
#ifdef DEBUG
	    if (rval == MAYBE)
		ll_log(logptr, LLOGFTR, "nameserver query timed out");
	    else
		ll_log(logptr, LLOGFTR, "nameserver query failed");
#endif
	    return(rval);
	}
    }

    /* O.K. now give answer */

    switch (type)
    {
	case TB_CHANNEL:
	    /* if NS failure we returned MAYBE above */
	    if ((max_mxa <= 0) || (on_mxa >= max_mxa))
		return(NOTOK);

	    tmp = (char *)&(mx_addrs[on_mxa++]);

	    (void) sprintf(value,"%u.%u.%u.%u",
		    ((unsigned)tmp[0]) & 0xff, ((unsigned)tmp[1]) & 0xff,
		    ((unsigned)tmp[2]) & 0xff, ((unsigned)tmp[3]) & 0xff);
	    break;

	default:
	    /* can't get multiple names */
	    if (!first)
		return(NOTOK);

	    /* give them the name */
	    (void) strcpy(value,dn_name);
	    break;
    }
    
#ifdef  DEBUG
    ll_log(logptr, LLOGFTR, "NS returns '%s'", value);
#endif
    return(OK);
}

/*
 * see if name is not cannonical name.
 * We actually do an MX query and look for a CNAME RR.  By
 * doing an MX query we ensure that the local BIND cache has
 * been primed with MX's for later MX query (which may not be
 * done in the same process).
 */

LOCFUN
ns_getcn(key, name, namelen)
char *key, *name;
int namelen;
{
    register int n;
    register int count;
    register char *cp;
    u_short dsize, type;
    char *eom;
    union querybuf qbuf;
    union ansbuf abuf;
    HEADER *hp;
    extern char *ns_skiphdr();

#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_getcn(%s)",key);
#endif

    /*
     * We usually query for MX.  This should get us CNAME RRs if any
     * and if no CNAME RRs, gets the MXs into local server's cache
     * (a nice deal all around).  But there are certain cases in
     * which aliases have MXs associated.  (This is wrong, but does
     * happen).  In this case we may see problems.
     */

    /* turn off resolver name-completion tricks */
#ifdef RES_DNSRCH
    _res.options &= ~((int) (RES_DEFNAMES | RES_DNSRCH));
#else
    _res.options &= ~((int) RES_DEFNAMES);
#endif


#ifdef ROBUST
    n = res_mkquery(QUERY, key, C_IN, T_CNAME, (char *)0, 0, (char *)0,
	(char *)&qbuf, sizeof(qbuf));
#else
    n = res_mkquery(QUERY, key, C_IN, T_MX, (char *)0, 0, (char *)0,
	(char *)&qbuf, sizeof(qbuf));
#endif

    /* what else can we do? */
    if (n < 0) {
#ifdef DEBUG
	ll_log(logptr, LLOGFTR, "res_mkquery: n=%d, errno=%d, h_errno=%d", n, errno, h_errno);
#endif
	return(NOTOK);
    }

    n = res_send((char *)&qbuf,n,(char *)&abuf, sizeof(abuf));

    if (n < 0)
    {
#ifdef DEBUG
	ll_log(logptr, LLOGFTR,
		"ns_getcn: bad return from res_send, n=%d, errno=%d, h_errno=%d",
		n, errno, h_errno);
#endif
	return(MAYBE);
    }

    hp = (HEADER *)&abuf;

    if (hp->rcode != NOERROR)
	return(ns_error(hp));

    if ((count=ntohs(hp->ancount)) == 0)
	goto realname;

    /* only get here on NOERRR with ancount != 0 */
#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_getcn: parsing answer to query, hp->ancount=%d", count);
#endif

    /* skip header */
    eom = ((char *)&abuf)+n;

    if ((cp = ns_skiphdr((char *)&abuf, hp, eom)) == 0)
	return(MAYBE);

    while ((cp < eom) && (count--))
    {
	if ((n = dn_expand((char *)&abuf,eom, cp, name, namelen))<0)
	    return(MAYBE);

	cp += n;
	type = getshort(cp);

	/* skip to datasize */
	cp += (2 * sizeof(u_short)) + sizeof(u_long);
	dsize = getshort(cp);
	cp += sizeof(u_short);

	if (type == T_CNAME)
	{
	    if ((n = dn_expand((char *)&abuf, eom, cp, name, namelen))<0)
		return(MAYBE);

#ifdef DEBUG
		ll_log(logptr, LLOGFTR, "ns_getcn: %s -> %s",key,name);
#endif
	    return(OK);
	}

	/* skip to next RR */
	cp += dsize;
    }

    /*
     * name is real name
     */

realname:
    (void) strncpy(name,key,namelen);
#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_getcn: %s -> %s",key,name);
#endif
    return(OK);
}

/*
 * build a list of addresses of MX hosts to try....
 */

LOCFUN
ns_getmx(key, max, mxtab, tsize)
char *key;
int *max;
struct in_addr mxtab[];
int tsize;
{
    register char *cp;
    register int i, j, n;
    HEADER *hp;
    struct hostent *he;
    union querybuf qbuf;
    union ansbuf abuf;
    u_short type, dsize;
    int pref, localpref, tryagains;
    int count, mxcount;
    int sawmx;			/* are we actually processing mx's? */
    char *eom;
    char buf[MAXDNAME];		/* for expanding in dn_expand */
    char newkey[MAXDNAME]; 	/* in case we get a CNAME RR back... */
    struct {			/* intermediate table */
	char *mxname;
	u_short mxpref;
    } mx_list[MAXMX];
    extern char *ns_skiphdr();

#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_getmx(%s, %x, %x, %d)",key,max,mxtab,tsize);
#endif

restart:

    mxcount = 0;
    *max = 0;
    localpref = -1;
    sawmx = 0;
    tryagains = 0;

    /* turn off resolver name-completion tricks */
#ifdef RES_DNSRCH
    _res.options &= ~((int) (RES_DEFNAMES | RES_DNSRCH));
#else
    _res.options &= ~((int) RES_DEFNAMES);
#endif

    n = res_mkquery(QUERY, key, C_IN, T_MX, (char *)0, 0, (char *)0,
	(char *)&qbuf, sizeof(qbuf));

    /* what else can we do? */
    if (n < 0) {
	ll_log(logptr, LLOGFTR, "res_mkquery, n=%d, errno=%d, h_errno=%d", n, errno, h_errno);
	return(NOTOK);
    }

#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_getmx: sending ns query (%d bytes)",n);
#endif

    n = res_send((char *)&qbuf,n,(char *)&abuf, sizeof(abuf));

    if (n < 0) {
#ifdef DEBUG
	ll_log(logptr, LLOGFTR,
		"ns_getmx: bad return from res_send, n=%d, errno=%d, h_errno=%d",
		n, errno, h_errno);
#endif
	return(MAYBE);
    }

    hp = (HEADER *)&abuf;

#ifdef OLDSERVER
    if ((hp->rcode != NOERROR) && (hp->rcode != FORMERR))
#else
    if (hp->rcode != NOERROR) 
#endif /* OLDSERVER */
	return(ns_error(hp));

#ifdef OLDSERVER
    if ((ntohs(hp->ancount) == 0) || (hp->rcode == FORMERR)) {
#else
    if (ntohs(hp->ancount) == 0) {
#endif /* OLDSERVER */
	mxcount = 1;
	mx_list[0].mxname = strdup(key);
	mx_list[0].mxpref = 0;
	goto doaddr;
    }

    /* read MX list */
    sawmx = 1;
    count = ntohs(hp->ancount);

    /* need local machine name */
    if (local[0] == '\0') {
	if ((locfullmachine != 0) && (*locfullmachine != '\0'))
	    (void) strcpy(local, locfullmachine);
	else
	    (void) strcpy(local, locfullname);
    }

    /* skip header */
    eom = ((char *)&abuf) + n;
    if ((cp = ns_skiphdr((char *)&abuf, hp, eom))==0)
	goto quit;

#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_getmx: %d answers to query",count);
#endif

    while ((cp < eom) && (count--)) {
	n = dn_expand((char *)&abuf,eom, cp, buf, sizeof(buf));
	if (n < 0)
	    goto quit;

	cp += n;
	type = getshort(cp);
	/* get to datasize */
	cp += (2 * sizeof(u_short)) + sizeof(u_long);
	dsize = getshort(cp);
	cp += sizeof(u_short);

	/*
	 * is it an MX ? 
	 * note it could be a CNAME if we didn't use a domain lookup
	 */

	if (type == T_CNAME) {
	    if (key == newkey) {
		/* CNAME -> CNAME: illegal */
		ll_log(logptr,LLOGTMP,"ns_getmx: recursive CNAME answer to MX query");
		return(NOTOK);
	    }
	    ll_log(logptr,LLOGTMP,"ns_getmx: CNAME answer to MX query");
	    n = dn_expand((char *)&abuf,eom, cp, newkey, sizeof(newkey));
	    cp += dsize;
	    if (n < 0)
		continue;	/* pray? */
#ifdef DEBUG
	    ll_log(logptr,LLOGFTR,"ns_getmx: %s -> %s (new query)",key,newkey);
#endif DEBUG
	    key = newkey;
	    goto restart;
	}

	if (type != T_MX) {
	    ll_log(logptr,LLOGTMP,"ns_getmx: RR of type %d in response",type);
	    cp += dsize;
	    continue;       /* keep trying */
	}

	pref = getshort(cp);
	cp += sizeof(u_short);

	n = dn_expand((char *)&abuf,eom, cp, buf, sizeof(buf));
	if (n < 0)
	    goto quit;

	cp += n;

	/* is it local? */
	if ((lexequ(local,buf)) && ((localpref < 0) || (pref < localpref))) {
	    localpref = pref;
	    for(i=(mxcount-1); i >= 0; i--) {
		if (mx_list[i].mxpref < localpref)
		    break;

		(void) free(mx_list[i].mxname);
		mxcount--;
	    }
	    continue;
	}

	/* now, see if we keep it */
	if ((localpref >= 0) && (pref >= localpref))
	    continue;

	/*  find where it belongs */
	for(i=0; i < mxcount; i++)
	    if (mx_list[i].mxpref > pref)
		break;

	/* not of interest */
	if (i == MAXMX)
	    continue;

	/* shift stuff to make space */
	if (mxcount == MAXMX) {
		(void) free(mx_list[MAXMX-1].mxname);
		mxcount--;
	}

	for (j=mxcount; j>i; j--) {
	    mx_list[j].mxname = mx_list[j-1].mxname;
	    mx_list[j].mxpref = mx_list[j-1].mxpref;
	}

	mx_list[i].mxname = strdup(buf);
	mx_list[i].mxpref = pref;
	mxcount++;
    }

    /*
     * should read additional RR section for addresses and cache them
     * but let's hold on that.
     */

doaddr:
    /* now build the address list */
#ifdef DEBUG
    ll_log(logptr, LLOGFTR,"ns_getmx: using %d mx hosts",mxcount);
#endif

    for(i=0,j=0; (i < mxcount) && (j < tsize); i++) {
	/*
	 * note that gethostbyname() is slow -- we should cache so
	 * we don't ask for an address repeatedly
	 */

	he = gethostbyname(mx_list[i].mxname);

	if (he == 0) {
#ifdef DEBUG
	    ll_log(logptr, LLOGFTR, "ns_getmx: no addresses for %s",
		mx_list[i].mxname);
#endif
	    if (h_errno == TRY_AGAIN) {
		tryagains++;
		continue;
	    }

	    /*
	     * were trying special case of no MXs and got no address
	     * name is clearly bad
	     */
	    if (!sawmx)
		return(NOTOK);

	    continue;
	}

	for(n=0; (j < tsize) && (n < MAXADDR_PER); n++, j++) {
	    if (he->h_addr_list[n] == 0)
		break;

	    bcopy(he->h_addr_list[n],(char *)&mxtab[j],sizeof(struct in_addr));
	}
#ifdef DEBUG
	ll_log(logptr, LLOGFTR, "ns_getmx: %d addresses saved for %s",
		n, mx_list[i].mxname);
#endif
    }
    *max = j;

    for(i=0; i < mxcount; i++)
	(void) free(mx_list[i].mxname);

    /* decide which return value to give */

    /* if we have a list of addresses to try -> OK */
    if (*max != 0)
	return(OK);

    /* if we timed out on all the address lookups for MX's -> MAYBE */
    if ((tryagains > 0) && (tryagains == mxcount))
	return(MAYBE);

    /* lousy address */
    return(NOTOK);

quit:                                  /* escape hatch for parsing problems */
    for(i=0; i < mxcount; i++)
	(void) free(mx_list[i].mxname);

#ifdef DEBUG
	ll_log(logptr, LLOGFTR, "ns_getmx: problems parsing response to query");
#endif

    return(MAYBE);
}

/*
 * figure out proper error code to return given an error
 */

LOCFUN
ns_error(hp)
register HEADER *hp;
{

#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_error: server returned code %d",hp->rcode);
#endif

    switch (hp->rcode)
    {
	case NXDOMAIN:
	    return(NOTOK); /* even if not authoritative */

	case SERVFAIL:
	    return(MAYBE);

	default:
	    break;
    }

    return(NOTOK);
}

/*
 * skip header of query and return pointer to first answer RR.
 */

LOCFUN
char *ns_skiphdr(answer, hp, eom)
char *answer;
HEADER *hp;
register char *eom;
{
    register int qdcount;
    register char *cp;
    register int n;
    char tmp[MAXDNAME];

    qdcount = ntohs(hp->qdcount);

    cp = answer + sizeof(HEADER);

    while ((qdcount-- > 0) && (cp < eom))
    {
	n = dn_expand(answer,eom,cp,tmp,sizeof(tmp));
	if (n < 0)
	    return(0);
	cp += (n + QFIXEDSZ);
    }

    return((cp < eom)? cp : 0);
}

/*
 * routine to set the resolver timeouts
 * takes maximum number of seconds you are willing to wait
 */

ns_settimeo(ns_time)
int     ns_time;
{
    static int called = 0;
    static struct state oldres;

    if ((_res.options & RES_INIT) == 0)
	    res_init ();

    /* always start afresh */
    if (called)
    {
	bcopy((char *)&oldres,(char *)&_res,sizeof(oldres));
    }
    else
    {
	called = 1;
	bcopy((char *)&_res,(char *)&oldres,sizeof(oldres));
    }

    /*
     * bind uses an exponential backoff
     */

    _res.retrans = ns_time >> _res.retry;

#ifdef DEBUG
    ll_log(logptr, LLOGFTR, "ns_timeo: servers(%d), retrans(%d), retry(%d)",
	_res.nscount, _res.retrans, _res.retry);
#endif
}

/*
 * Caching stuff starts here....
 * cache stores following pairs:
 *        key -> mx address list
 *        key -> canonical name
 *
 * also stores rvals for keys, so if we failed or timedout, we only
 * do that once in the process lifetime.
 *
 * must store complete answers to a table fetch  -- storing by RR's
 * leads to incomplete information in the cache -- and thus busted
 * routing.
 */

/*
 * see if key/type pair is in cache.  If so, set rval appropriately
 */
#ifdef NSCACHE

static cachehash(key)
register char *key;
{
    register int i;
    register unsigned c;
    register unsigned sum;
    extern char chrcnv[];

    sum = 0;

    for(i=0; *key != '\0'; i++, key++)
    {
	c = chrcnv[*key & 0x7f] & 0xff;
	sum +=  c* i;
    }

    return(sum % NSCACHE);
}

cachehit(key,rval,tbltype)
char *key;
int *rval;
int tbltype;
{
    register int i;
    register struct ns_cache *cp;
    register struct in_addr *ip1, *ip2;

    *rval = OK;

    i = cachehash(key);

    ll_log(logptr,LLOGFTR,"ns: key %s -> %d\n",key,i);

    if (tbltype == TB_CHANNEL)
	cp = chncache[i];
    else if ((tbltype == TB_DOMAIN))
	cp = dmncache[i];
    else
    {
	ll_log(logptr,LLOGFTR,"ns: no cache!\n");
	cp = 0;
    }

    for(; cp != 0; cp = cp->nc_next)
    {
	if (!lexequ(key,cp->nc_key))
	    continue;

#ifdef DEBUG
	ll_log(logptr,LLOGFTR,"ns: cache hit, key %s\n",key);
#endif /* DEBUG */

	if ((*rval = cp->nc_rval) == OK)
	{
	    switch (tbltype)
	    {
		case TB_CHANNEL:
		    /* fill mx cache */
		    max_mxa = cp->nc_count;
		    on_mxa = 0;

		    ip1 = (struct in_addr *)cp->nc_data;
		    ip2 = mx_addrs;
		    for(i=0; i < max_mxa; i++)
			*ip2++ = *ip1++;
		    break;

		case TB_DOMAIN:
		    /* fill in dn_name */
		    if (cp->nc_count)
			(void) strcpy(dn_name,cp->nc_data);
		    else
			(void) strcpy(dn_name,cp->nc_key);
	    }
	}

	return(1);
    }

    /* no one should look at rval, but... */
    *rval = NOTOK;
    return(0);
}


cachechn(key,rval,count,list)
char *key;
int rval, count;
struct in_addr *list;
{
    register struct ns_cache *cp;
    register struct in_addr *ip1, *ip2;
    int i;
    extern char *malloc(), *calloc();

    /* NOSTRICT */
    if ((cp = (struct ns_cache *)malloc(sizeof(*cp)))==0)
	return;

    /* NOSTRICT */
    if ((cp->nc_data = calloc((unsigned)count,sizeof(*list)))==0)
    {
	(void) free((char *)cp);
	return;
    }
    if ((cp->nc_key = strdup(key))==0)
    {
	(void) free(cp->nc_data);
	(void) free((char *)cp);
	return;
    }

    cp->nc_count = count;
    cp->nc_rval = rval;

    ip1 = list;
    ip2 = (struct in_addr *)cp->nc_data;

    while (count--)
	*ip2++ = *ip1++;

    /* stuff it in */
    i = cachehash(cp->nc_key);

    cp->nc_next = chncache[i];
    chncache[i] = cp;
}

cachedmn(key,rval,name)
char *key, *name;
int rval;
{
    register struct ns_cache *cp;
    int i;
    extern char *malloc(), *strdup();

    /* NOSTRICT */
    if ((cp = (struct ns_cache *)malloc(sizeof(*cp)))==0)
	return;

    if ((cp->nc_key = strdup(key))==0)
    {
	(void) free((char *)cp);
	return;
    }

    if ((name != 0) && !lexequ(key,name))
    {
	if ((cp->nc_data = strdup(key)) == 0)
	{
	    (void) free(cp->nc_key);
	    (void) free((char *)cp);
	    return;
	}
	cp->nc_count = 1;
    }
    else
    {
	cp->nc_data = 0;
	cp->nc_count = 0;
    }

    cp->nc_rval = rval;

    /* stuff it in */
    i = cachehash(cp->nc_key);

    cp->nc_next = dmncache[i];
    dmncache[i] = cp;
}
#endif NSCACHE