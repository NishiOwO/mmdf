/* Modified to compile on Linux Slackware 4.0: Sept 00, Christine Jamison */
/* getbbent.c - subroutines for accessing the BBoards file */

/* LINTLIBRARY */

#include "bboards.h"
#ifndef	MMDFONLY
#include "../h/strings.h"
#include <sys/types.h>
#else /* MMDFONLY */
#include "util.h"
#include "mmdf.h"
#include "strings.h"
#endif /* MMDFONLY */
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/stat.h>


#ifndef	MMDFONLY
#define	NOTOK	(-1)
#define	OK	0
#endif /* not MMDFONLY */


#define	MaxBBAka	100
#define	MaxBBLdr	100
#define	MaxBBDist	100


#define	NCOLON	9		/* currently 10 fields per entry */

#define	COLON	':'
#define	COMMA	','
#define	NEWLINE	'\n'


#define	ARCHIVE	"archive"
#define	CNTFILE	".cnt"
#define	DSTFILE	".dist"
#define	MAPFILE ".map"

/*  */

static int  BBuid = -1;

static unsigned int  BBflags = SB_NULL;

static char BBName[BUFSIZ] = BBOARDS;
static char BBDir[BUFSIZ] = "";
static char BBData[BUFSIZ] = "";

static  FILE *BBfile = NULL;


static struct bboard    BB;
static struct bboard   *bb = &BB;

static int  BBload = 1;

static char BBFile[BUFSIZ];
static char BBArchive[BUFSIZ];
static char BBInfo[BUFSIZ];
static char BBMap[BUFSIZ];
static char *BBAkas[MaxBBAka];
static char *BBLeaders[MaxBBLdr];
static char *BBDists[MaxBBDist];
static char BBAddr[BUFSIZ];
static char BBRequest[BUFSIZ];
static char BBDate[BUFSIZ];
static char BBErrors[BUFSIZ];

#ifdef	MMDFONLY
extern LLog *logptr;
#endif /* MMDFONLY */

/* char   *bbskip (), *getcpy (); */
static char   *bbskip ();
static char  *getcpy ();

char   *crypt (), *getpass ();
struct group  *getgrnam ();
struct passwd *getpwnam (), *getpwuid ();

/*  */

int     setbbfile (file, f)
register char  *file;
register int    f;
{
    if (BBuid == -1)
	return setbbinfo (BBOARDS, file, f);

    (void) strncpy (BBData, file, sizeof(BBData));

    BBflags = SB_NULL;
    (void) endbbent ();

    return setbbent (f);
}

/*  */

int	setbbinfo (user, file, f)
register char  *user,
               *file;
register int	f;
{
    register struct passwd *pw;

    if ((pw = getpwnam (user)) == NULL) {
	(void) snprintf (BBErrors, sizeof(BBErrors), "unknown user: %s", user);
	return 0;
    }

    return setpwinfo (pw, file, f);
}


int	setpwinfo (pw, file, f)
register struct passwd *pw;
register char  *file;
register int	f;
{
    if (!setpwaux (pw, file))
	return 0;

    BBflags = SB_NULL;
    (void) endbbent ();

    return setbbent (f);
}

/*  */

static int  setbbaux (name, file)
register char  *name,
	       *file;
{
    register struct passwd *pw;

    if ((pw = getpwnam (name)) == NULL) {
	(void) snprintf (BBErrors, sizeof(BBErrors), "unknown user: %s", name);
	return 0;
    }

    return setpwaux (pw, file);
}


static int  setpwaux (pw, file)
register struct passwd *pw;
register char  *file;
{
    (void) strncpy (BBName, pw -> pw_name, sizeof(BBName));
    BBuid = pw -> pw_uid;
    (void) strncpy (BBDir, pw -> pw_dir, sizeof(BBDir));
    (void) snprintf (BBData, sizeof(BBData), "%s/%s",
	    *file != '/' ? BBDir : "",
	    *file != '/' ? file : file + 1);

    BBflags = SB_NULL;

    return 1;
}

/*  */

int     setbbent (f)
register int     f;
{
    if (BBfile == NULL) {
	if (BBuid == -1 && !setbbaux (BBOARDS, BBDB))
	    return 0;

	if ((BBfile = fopen (BBData, "r")) == NULL) {
	    (void) snprintf (BBErrors, sizeof(BBErrors), "unable to open: %s", BBData);
	    return 0;
	}
    }
    else
	rewind (BBfile);

    BBflags |= f;
    return (BBfile != NULL);
}


int     endbbent () {
    if (BBfile != NULL && !(BBflags & SB_STAY)) {
	(void) fclose (BBfile);
	BBfile = NULL;
    }

    return 1;
}


long    getbbtime () {
    struct stat st;

    if (BBfile == NULL) {
	if (BBuid == -1 && !setbbaux (BBOARDS, BBDB))
	    return 0;

	if (stat (BBData, &st) == NOTOK) {
	    (void) snprintf (BBErrors, sizeof(BBErrors), "unable to stat: %s", BBData);
	    return 0;
	}
    }
    else
	if (fstat (fileno (BBfile), &st) == NOTOK) {
	    (void) snprintf (BBErrors, sizeof(BBErrors), "unable to fstat: %s", BBData);
	    return 0;
	}

    return ((long) st.st_mtime);
}

/*  */

struct bboard  *getbbent () {
    register int    count;
    register char  *p,
                   *q,
                   *r,
                   *d,
                   *f,
                  **s;
    static char line[BUFSIZ];

    if (BBfile == NULL && !setbbent (SB_NULL))
	return NULL;

retry: ;
    if ((p = fgets (line, sizeof line, BBfile)) == NULL)
	return NULL;

    for (q = p, count = 0; *q != NULL && *q != NEWLINE; q++)
	if (*q == COLON)
	    count++;

    if (count != NCOLON) {
#ifdef	MMDFONLY
	if (q = strchr (p, NEWLINE))
	    *q = NULL;
	ll_log (logptr, LLOGTMP, "bad entry in %s: %s", BBData, p);
#endif /* MMDFONLY */
	goto retry;
    }
    
    bb -> bb_name = p;
    p = q = bbskip (p, COLON);
    p = bb -> bb_file = bbskip (p, COLON);
    bb -> bb_archive = bb -> bb_info = bb -> bb_map = "";
    p = bb -> bb_passwd = bbskip (p, COLON);
    p = r = bbskip (p, COLON);
    p = bb -> bb_addr = bbskip (p, COLON);
    p = bb -> bb_request = bbskip (p, COLON);
    p = bb -> bb_relay = bbskip (p, COLON);
    p = d = bbskip (p, COLON);
    p = f = bbskip (p, COLON);
    (void) bbskip (p, NEWLINE);

    s = bb -> bb_aka = BBAkas;
    while (*q) {
	*s++ = q;
	q = bbskip (q, COMMA);
    }
    *s = NULL;

    s = bb -> bb_leader = BBLeaders;
    if (*r == NULL) {
	if (!(BBflags & SB_FAST))
	    *s++ = BBName;
    }
    else
	while (*r) {
	    *s++ = r;
	    r = bbskip (r, COMMA);
	}
    *s = NULL;

    s = bb -> bb_dist = BBDists;
    while (*d) {
	*s++ = d;
	d = bbskip (d, COMMA);
    }
    *s = NULL;

    if (*f)
	(void) sscanf (f, "%o", &bb -> bb_flags);
    else
	bb -> bb_flags = BB_NULL;
    bb -> bb_count = bb -> bb_maxima = 0;
    bb -> bb_date = NULL;
    bb -> bb_next = bb -> bb_link = bb -> bb_chain = NULL;

    if (BBload)
	BBread ();

    return bb;
}

/*  */

struct bboard  *getbbnam (name)
register char   *name;
{
    register struct bboard *b = NULL;

    if (!setbbent (SB_NULL))
	return NULL;
    BBload = 0;
    while ((b = getbbent ()) && strcmp (name, b -> bb_name))
	continue;
    BBload = 1;
    (void) endbbent ();

    if (b != NULL)
	BBread ();

    return b;
}


struct bboard  *getbbaka (aka)
register char   *aka;
{
    register char **ap;
    register struct bboard *b = NULL;

    if (!setbbent (SB_NULL))
	return NULL;
    BBload = 0;
    while ((b = getbbent ()) != NULL)
	for (ap = b -> bb_aka; *ap; ap++)
	    if (strcmp (aka, *ap) == 0)
		goto hit;
hit: ;
    BBload = 1;
    (void) endbbent ();

    if (b != NULL)
	BBread ();

    return b;
}

/*  */

static int  BBread () {
    register int    i;
    register char  *cp,
                   *dp,
		   *p,
		   *r;
    char    prf[BUFSIZ];
    static char line[BUFSIZ];
    register    FILE * info;

    if (BBflags & SB_FAST)
	return;

    p = strchr (bb -> bb_request, '@');
    r = strchr (bb -> bb_addr, '@');
    BBRequest[0] = NULL;

    if (*bb -> bb_request == '-')
	if (p == NULL && r && *r == '@')
	    (void) snprintf (BBRequest, sizeof(BBRequest), "%s%s%s",
		    bb -> bb_name, bb -> bb_request, r);
	else
	    (void) snprintf (BBRequest, sizeof(BBRequest), "%s%s",
		    bb -> bb_name, bb -> bb_request);
    else
	if (p == NULL && r && *r == '@' && *bb -> bb_request)
	    (void) snprintf (BBRequest, sizeof(BBRequest), "%s%s", bb -> bb_request, r);

    if (BBRequest[0])
	bb -> bb_request = BBRequest;
    else
	if (*bb -> bb_request == NULL)
	    bb -> bb_request = *bb -> bb_addr ? bb -> bb_addr
		: bb -> bb_leader[0];

    if (*bb -> bb_addr == '@') {
	(void) snprintf (BBAddr, sizeof(BBAddr), "%s%s", bb -> bb_name, bb -> bb_addr);
	bb -> bb_addr = BBAddr;
    }
    else
	if (*bb -> bb_addr == NULL)
	    bb -> bb_addr = bb -> bb_name;

    if (*bb -> bb_file == NULL)
	return;
    if (*bb -> bb_file != '/') {
	(void) snprintf (BBFile, sizeof(BBFile), "%s/%s", BBDir, bb -> bb_file);
	bb -> bb_file = BBFile;
    }

    if ((cp = strrchr (bb -> bb_file, '/')) == NULL || *++cp == NULL)
	(void) strncpy (prf, "", sizeof(prf)), cp = bb -> bb_file;
    else
	(void) snprintf (prf, sizeof(prf), "%.*s", cp - bb -> bb_file, bb -> bb_file);
    if ((dp = strchr (cp, '.')) == NULL)
	dp = cp + strlen (cp);

    (void) snprintf (BBArchive, sizeof(BBArchive), "%s%s/%s", prf, ARCHIVE, cp);
    bb -> bb_archive = BBArchive;
    (void) snprintf (BBInfo, sizeof(BBInfo), "%s.%.*s%s", prf, dp - cp, cp, CNTFILE);
    bb -> bb_info = BBInfo;
    (void) snprintf (BBMap, sizeof(BBMap), "%s.%.*s%s", prf, dp - cp, cp, MAPFILE);
    bb -> bb_map = BBMap;

    if ((info = fopen (bb -> bb_info, "r")) == NULL)
	return;

    if (fgets (line, sizeof line, info) && (i = atoi (line)) > 0)
	bb -> bb_maxima = (unsigned) i;
    if (!feof (info) && fgets (line, sizeof line, info)) {
	(void) strncpy (BBDate, line, sizeof(BBDate));
	if (cp = strchr (BBDate, NEWLINE))
	    *cp = NULL;
	bb -> bb_date = BBDate;
    }

    (void) fclose (info);
}

/*  */

int     ldrbb (b)
register struct bboard  *b;
{
    register char  *p,
                  **q,
                  **r;
    static int  uid = 0,
                gid = 0;
    static char username[10] = "";
    register struct passwd *pw;
    register struct group  *gr;

    if (b == NULL)
	return 0;
    if (BBuid == -1 && !setbbaux (BBOARDS, BBDB))
	return 0;

    if (username[0] == NULL) {
	if ((pw = getpwuid (uid = getuid ())) == NULL)
	    return 0;
	gid = getgid ();
	(void) strncpy (username, pw -> pw_name, sizeof(username));
    }

    if (uid == BBuid)
	return 1;

    q = b -> bb_leader;
    while (p = *q++)
	if (*p == '=') {
	    if ((gr = getgrnam (++p)) == NULL)
		continue;
	    if (gid == gr -> gr_gid)
		return 1;
	    r = gr -> gr_mem;
	    while (p = *r++)
		if (strcmp (username, p) == 0)
		    return 1;
	}
	else
	    if (strcmp (username, p) == 0)
		return 1;

    return 0;
}

/*  */

int     ldrchk (b)
register struct bboard  *b;
{
    if (b == NULL)
	return 0;

    if (*b -> bb_passwd == NULL)
	return 1;

    if (strcmp (b -> bb_passwd,
		crypt (getpass ("Password: "), b -> bb_passwd)) == 0)
	return 1;

    fprintf (stderr, "Sorry\n");
    return 0;
}

/*  */

struct bboard  *getbbcpy (bp)
register struct bboard  *bp;
{
    register char **p,
                  **q;
    register struct bboard *b;

    if (bp == NULL)
	return NULL;

    b = (struct bboard *) malloc ((unsigned) sizeof *b);
    if (b == NULL)
	return NULL;

    b -> bb_name = getcpy (bp -> bb_name);
    b -> bb_file = getcpy (bp -> bb_file);
    b -> bb_archive = getcpy (bp -> bb_archive);
    b -> bb_info = getcpy (bp -> bb_info);
    b -> bb_map = getcpy (bp -> bb_map);
    b -> bb_passwd = getcpy (bp -> bb_passwd);
    b -> bb_flags = bp -> bb_flags;
    b -> bb_count = bp -> bb_count;
    b -> bb_maxima = bp -> bb_maxima;
    b -> bb_date = getcpy (bp -> bb_date);
    b -> bb_addr = getcpy (bp -> bb_addr);
    b -> bb_request = getcpy (bp -> bb_request);
    b -> bb_relay = getcpy (bp -> bb_relay);

    for (p = bp -> bb_aka; *p; p++)
	continue;
    b -> bb_aka =
	q = (char **) calloc ((unsigned) (p - bp -> bb_aka + 1), sizeof *q);
    if (q == NULL)
	return NULL;
    for (p = bp -> bb_aka; *p; *q++ = getcpy (*p++))
	continue;
    *q = NULL;

    for (p = bp -> bb_leader; *p; p++)
	continue;
    b -> bb_leader =
	q = (char **) calloc ((unsigned) (p - bp -> bb_leader + 1), sizeof *q);
    if (q == NULL)
	return NULL;
    for (p = bp -> bb_leader; *p; *q++ = getcpy (*p++))
	continue;
    *q = NULL;

    for (p = bp -> bb_dist; *p; p++)
	continue;
    b -> bb_dist = 
	q = (char **) calloc ((unsigned) (p - bp -> bb_dist + 1), sizeof *q);
    if (q == NULL)
	return NULL;
    for (p = bp -> bb_dist; *p; *q++ = getcpy (*p++))
	continue;
    *q = NULL;

    b -> bb_next = bp -> bb_next;
    b -> bb_link = bp -> bb_link;
    b -> bb_chain = bp -> bb_chain;

    return b;
}

/*  */

int     getbbdist (bb, action)
register struct bboard  *bb;
register int     (*action) ();
{
    register int    result;
    register char **dp;

    BBErrors[0] = NULL;
    for (dp = bb -> bb_dist; *dp; dp++)
	if (result = getbbitem (bb, *dp, action))
	    return result;

    return result;
}

char    *getbberr () {
    return (BBErrors[0] ? BBErrors : NULL);
};

/*  */

static int  getbbitem (bb, item, action)
register struct bboard  *bb;
register char   *item;
register int     (*action) ();
{
    register int    result;
    register char  *cp,
                   *dp,
                   *hp,
                   *np;
    char    mbox[BUFSIZ],
            buffer[BUFSIZ],
            file[BUFSIZ],
            host[BUFSIZ],
            prf[BUFSIZ];
    register FILE *fp;

    switch (*item) {
	case '*': 
	    switch (*++item) {
		case '/': 
		    hp = item;
		    break;

		case NULL: 
		    if ((cp = strrchr (bb -> bb_file, '/')) == NULL || *++cp == NULL)
			(void) strncpy (prf, "", sizeof(prf)), cp = bb -> bb_file;
		    else
			(void) snprintf (prf, sizeof(prf), "%.*s", cp - bb -> bb_file, bb -> bb_file);
		    if ((dp = strchr (cp, '.')) == NULL)
			dp = cp + strlen (cp);
		    (void) snprintf (file, sizeof(file), "%s.%.*s%s", prf, dp - cp, cp, DSTFILE);
		    hp = file;
		    break;

		default: 
		    (void) snprintf (file, sizeof(file), "%s/%s", BBDir, item);
		    hp = file;
		    break;
	    }

	    if ((fp = fopen (hp, "r")) == NULL)
		return bblose ("unable to read file %s", hp);
	    while (fgets (buffer, sizeof buffer, fp)) {
		if (np = strchr (buffer, '\n'))
		    *np = NULL;
		if (result = getbbitem (bb, buffer, action)) {
		    (void) fclose (fp);
		    (void) bblose ("error with file %s, item %s", hp, buffer);
		    return result;
		}
	    }
	    (void) fclose (fp);
	    return OK;

	default: 
	    if (hp = strrchr (item, '@')) {
		*hp++ = NULL;
		(void) strncpy (mbox, item, sizeof(mbox));
		(void) strncpy (host, hp, sizeof(host));
		*--hp = '@';
	    }
	    else {
		(void) snprintf (mbox, sizeof(mbox), "%s%s", DISTADR, bb -> bb_name);
		(void) strncpy (host, item, sizeof(host));
	    }
	    if (result = (*action) (mbox, host))
		(void) bblose ("action (%s, %s) returned 0%o", mbox, host, result);
	    return result;
    }
}

/*  */

/* VARARGS1 */

static int  bblose (fmt, a, b, c)
char   *fmt,
       *a,
       *b,
       *c;
{
    if (BBErrors[0] == NULL)
	(void) snprintf (BBErrors, sizeof(BBErrors), fmt, a, b, c);

    return NOTOK;
}

/*  */

void	make_lower (s1, s2)
register char   *s1,
		*s2;
{
    if (s1 == NULL || s2 == NULL)
	return;

    for (; *s2; s2++)
	*s1++ = isupper (*s2) ? tolower (*s2) : *s2;
    *s1 = NULL;
}

/*  */

static char *bbskip (p, c)
register char  *p,
		c;	
{
    if (p == NULL)
	return NULL;

    while (*p && *p != c)
	p++;
    if (*p)
	*p++ = NULL;

    return p;
}


static	char   *getcpy (s)
register char   *s;
{
    register char  *p;

    if (s == NULL)
	return NULL;

    if (p = malloc ((unsigned) (strlen (s) + 1)))
	(void) strcpy (p, s);
    return p;
}
