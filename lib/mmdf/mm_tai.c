#include "util.h"
#include "mmdf.h"
#include "cmd.h"
#include "ch.h"
#include "ap.h"

#define BACKWARD_COMPAT 1
int verbose_tai = 0;

extern int tb_noop();
#define tb_file_init tb_noop
#define tb_dbm_init tb_noop

#ifdef HAVE_NIS
extern int tb_nis_init();
#else /* HAVE_NIS */
#  define tb_nis_init tb_noop
#endif /* HAVE_NIS */

#ifdef HAVE_REGEXEC
extern int tb_regexp_init();
#else /* HAVE_REGEXEC */
#  define tb_regexp_init tb_noop
#endif /* HAVE_REGEXEC */

#ifdef HAVE_RBL
extern int tb_rbl_init();
#else /* HAVE_RBL */
#  define tb_rbl_init tb_noop
#endif /* HAVE_RBL */

#ifdef HAVE_LDAP
extern int tb_ldap_init();
#else /* HAVE_LDAP */
#  define tb_ldap_init tb_noop
#endif /* HAVE_LDAP */

#ifdef HAVE_SQLSUPPORT
extern int tb_sql_init();
#else /* HAVE_SQLSUPPORT */
#  define tb_sql_init tb_noop
#endif /* HAVE_SQLSUPPORT */
extern int tb_test_init();

#if 0
extern int tb_file_init();
extern int tb_dbm_init();
#endif
extern int tb_ns_init();

extern void cnvtbytestr();

extern LLog *logptr, chanlog;

extern char
	    *locmachine,        /* UCL local machine name       */
	    *locfullmachine,    /* UCL local machine name       */
	    *locname,
	    *locfullname,
	    *locdomain,
	    *sitesignature,
	    *mmdflogin,
	    *mmdfgroup,
	    *supportaddr;

extern char
	    *logdfldir,
	    *phsdfldir,
	    *tbldfldir,
	    *tbldbm,
	    *quedfldir,
	    *cmddfldir,
	    *chndfldir,
	    *lckdfldir,
	    *mldfldir;

extern int
	    queprot,
	    warntime,
	    failtime,
	    maxhops,
	    maxqueue,
	    mgt_addid,
            mgt_addipaddr,
            mgt_addipname,
	    lnk_listsize,
	    mid_enable,
	    mailsleep,
	    sentprotect,
        mbox_quota_soft,  /* MailBox quota limit (in bytes) */
        mbox_quota_hard;  /* MailBox quota limit (in bytes) */


#ifdef HAVE_ESMTP
long
        message_size_limit = -1;
#   ifdef HAVE_ESMTP_8BITMIME
int     accept_8bitmime    = 0;
#   endif /* HAVE_ESMTP_8BITMIME */
#   ifdef HAVE_ESMTP_DSN
int     dsn = 0;
#   endif /* HAVE_ESMTP_DSN */
#endif /* HAVE_ESMTP */

extern LLog
	    msglog,
	    chanlog,
	    authlog;

extern char *def_trn;

extern char
	    *tquedir,
	    *aquedir,
	    *mquedir;

extern char
	    *namsubmit,
	    *pathsubmit,
	    *namdeliver,
	    *pathdeliver,
	    *nampkup,
	    *pathpkup,
	    *nammail,
	    *pathmail;

extern char
	    *ch_dflnam,
	    *dlvfile,
	    *mldflfil,
	    *delim1,
	    *delim2;

extern char                     /* NIFTP channel bits                   */
		*pn_quedir;

extern char
		*authrequest,   /* Authorisation request		*/
		*Uuxstr,	/* uux command string			*/
		*Uuname;	/* uucp node name			*/
extern char
        *valid_channels; /* list of known channel for smtpsrvr */

/**/

#define MMLOCHOST        1
#define MMSIGN           2
#define MMLOGIN          3
#define MMSUPPORT        4
#define MMLOGDIR         5
#define MMPHSDIR         6
#define MMTBLDIR         7
#define MMDBM            8
#define MMQUEDIR         9
#define MMCMDDIR        10
#define MMCHANDIR       11
#define MMDLVRDIR       12
#define MMTEMPT         13
#define MMADDRQ         14
#define MMMSGQ          15
#define MMQUEPROT       16
#define MMMSGLOG        17
#define MMCHANLOG       18
#define	Gonzo1          /* 19 */
#define	Gonzo2          /* 20 */
#define MMWARNTIME      21
#define MMFAILTIME      22
#define MMMAXSORT       23
#define MMSLEEP         24
#define MMSUBMIT        25
#define MMDELIVER       26
#define MMPICKUP        27
#define MMV6MAIL        28
#define MMMBXPROT       29
#define MMMBXNAME       30
#define MMMBXPREF       31
#define MMMBXSUFF       32
#define MMDLVFILE       33
#define	MMAXHOPS	34
#define	MADDID		35
#define MLISTSIZE	36
#define MMDFLCHAN       37
#define UUxstr          38
#define MMTBL           39
#define MMLOCDOMAIN     40
#define MMDOMAIN        41
#define	ALIAS           42
#define MMLOCMACHINE    43
#define NIQUEDIR        44
#define MMMAILIDS       45
#define MMLCKDIR        46
#define AUTHLOG         47
#define AUTHREQUEST	    48
#define MMCHAN          49
#define UUname          50
#define MADDIPADDR      51
#define MADDIPNAME      52
#ifdef HAVE_ESMTP
#  define MMSGSIZELIMIT 53
#  define M8BITMIME     54
#  define MDSN          55
#endif
#define MMGROUP         56
#define MMAXMBSZ        57
#define MMVALIDCHN      58
#define MMNOOP         100
  
/**/

/* These tables must be in alphabetical order because they're searched
 * by cmdbsrch(), which does a binary search.
 *
 * -- DSH
 */
Cmd cmdtab[] =
{
    {"",            MMNOOP,     0},
#ifdef HAVE_ESMTP_8BITMIME
    {"accept8bitmime",M8BITMIME, 1},
#endif
    {"alias",	    ALIAS,      1},
    {"authlog",     AUTHLOG,    1},
    {"authrequest", AUTHREQUEST, 1},
#ifdef HAVE_ESMTP_DSN
    {"dsn",         MDSN,       1},
#endif
    {"maddid",	    MADDID,     1},
    {"maddipaddr",  MADDIPADDR, 1},
    {"maddipname",  MADDIPNAME, 1},
    {"maddrq",      MMADDRQ,    1},
    {"mchanlog",    MMCHANLOG,  1},
    {"mchn",        MMCHAN,     1},
    {"mchndir",     MMCHANDIR,  1},
    {"mcmddir",     MMCMDDIR,   1},
    {"mdbm",        MMDBM,      1},
    {"mdeliver",    MMDELIVER,  1},
    {"mdflchan",    MMDFLCHAN,  1},
    {"mdlv",        MMDLVFILE,  1},
    {"mdlvrdir",    MMDLVRDIR,  1},
    {"mdmn",        MMDOMAIN,   1},
    {"mfailtime",   MMFAILTIME, 1},
    {"mgroup",      MMGROUP,    1},
    {"mlckdir",     MMLCKDIR,   1},
    {"mldomain",    MMLOCDOMAIN, 1},
    {"mlistsize",   MLISTSIZE,  1},
    {"mlname",      MMLOCHOST,  1},
    {"mlochost",    MMLOCHOST,  1},
    {"mlocmachine", MMLOCMACHINE, 1},
    {"mlogdir",     MMLOGDIR,   1},
    {"mlogin",      MMLOGIN,    1},
    {"mmailid",     MMMAILIDS,  1},
    {"mmaxhops",    MMAXHOPS,   1},
    {"mmaxmbsz",    MMAXMBSZ,   2},
    {"mmaxsort",    MMMAXSORT,  1},
    {"mmbxname",    MMMBXNAME,  1},
    {"mmbxpref",    MMMBXPREF,  1},
    {"mmbxprot",    MMMBXPROT,  1},
    {"mmbxsuff",    MMMBXSUFF,  1},
    {"mmsglog",     MMMSGLOG,   1},
    {"mmsgq",       MMMSGQ,     1},
    {"mphsdir",     MMPHSDIR,   1},
    {"mpkup",       MMPICKUP,   1},
    {"mquedir",     MMQUEDIR,   1},
    {"mqueprot",    MMQUEPROT,  1},
#ifdef HAVE_ESMTP
    {"msgsizelimit",MMSGSIZELIMIT, 1},
#endif
    {"msig",        MMSIGN,     1},
    {"msleep",      MMSLEEP,    1},
    {"msubmit",     MMSUBMIT,   1},
    {"msupport",    MMSUPPORT,  1},
    {"mtbl",        MMTBL,      1},
    {"mtbldir",     MMTBLDIR,   1},
    {"mtempt",      MMTEMPT,    1},
    {"mtmpt",       MMTEMPT,    1},
    {"mv6mail",     MMV6MAIL,   1},
    {"mvalidchn",   MMVALIDCHN, 1},
    {"mwarntime",   MMWARNTIME, 1},
    {"niquedir",    NIQUEDIR,   1},
    {"uuname",      UUname,     1},
    {"uuxstr",      UUxstr,     1},
    {0,             0,          0}
};

#define CMDTABENT ((sizeof(cmdtab)/sizeof(Cmd))-1)

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * we don't need to copy and reallocate memory. We are just moving
 * pointers within in tai_init() allready allocate memory space. 
 * mm_tai(), ... are just moving pointer within tai_data.
 * if we free tai_data we will run into serious problems !
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

int mm_tai (argc, argv)     /* process mmdf tailor info     */
    int argc;
    char *argv[];
{
    int retval;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "m_tai (%s: %d)", argv[0], argc);
#endif

    retval = YES;

    switch (cmdbsrch (argv[0], --argc, cmdtab, CMDTABENT))
    {
	case NO:
	case MMNOOP:
	default:        /* command not known to us      */
	    retval = NO;
	    break;

	case MMLOCMACHINE:
	    locmachine = argv[1];
	    break;

	case MMLOCHOST:
	    locname = argv[1];
	    break;

	case MMLOCDOMAIN:
	    locdomain = argv[1];
	    break;

	case MMSIGN:
	    sitesignature = argv[1];
	    break;

	case MMLOGIN:
	    mmdflogin = argv[1];
	    break;

	case MMGROUP:
	    mmdfgroup = argv[1];
	    break;

	case MMSUPPORT:
	    supportaddr = argv[1];
	    break;

	case MMLOGDIR:
	    logdfldir = argv[1];
	    break;

	case MMPHSDIR:
	    phsdfldir = argv[1];
	    break;

	case MMTBLDIR:
	    tbldfldir = argv[1];
	    break;

	case MMDBM:
	    tbldbm = argv[1];
	    break;

	case MMQUEDIR:
	    quedfldir = argv[1];
	    break;

	case MMCMDDIR:
	    cmddfldir = argv[1];
	    break;

	case MMCHANDIR:
	    chndfldir = argv[1];
	    break;

	case MMDLVRDIR:
	    mldfldir = argv[1];
	    break;

	case MMLCKDIR:
	    lckdfldir = argv[1];
	    break;

	case MMTEMPT:
	    tquedir = argv[1];
	    break;

	case MMADDRQ:
	    aquedir = argv[1];
	    break;

	case MMMSGQ:
	    mquedir = argv[1];
	    break;

	case MMQUEPROT:
	    sscanf (argv[1], "%o", &queprot);
	    break;

	case MMMSGLOG:
	    retval = tai_log (argc, &argv[1], &msglog);
	    break;

	case MMCHANLOG:
	    retval = tai_log (argc, &argv[1], &chanlog);
	    break;

	case AUTHLOG:
	    retval = tai_log (argc, &argv[1], &authlog);
	    break;

	case AUTHREQUEST:
	    authrequest = argv[1];
	    break;

	case MMWARNTIME:
	    warntime = atoi (argv[1]);
	    break;

	case MMFAILTIME:
	    failtime = atoi (argv[1]);
	    break;

	case MMMAXSORT:
	    maxqueue = atoi (argv[1]);
	    break;

	case MMSLEEP:
	    mailsleep = atoi (argv[1]);
	    break;

	case MMSUBMIT:
	    retval = tai_pgm (argc, &argv[1], &namsubmit, &pathsubmit);
	    break;

	case MMDELIVER:
	    retval = tai_pgm (argc, &argv[1], &namdeliver, &pathdeliver);
	    break;

	case MMPICKUP:
	    retval = tai_pgm (argc, &argv[1], &nampkup, &pathpkup);
	    break;

	case MMV6MAIL:
	    retval = tai_pgm (argc, &argv[1], &nammail, &pathmail);
	    break;

	case MMMBXPROT:
	    sscanf (argv[1], "%o", &sentprotect);
	    break;

	case MMMBXNAME:
	    mldflfil = argv[1];
	    break;

	case MMMBXPREF:
	    delim1 = argv[1];
	    break;

	case MMMBXSUFF:
	    delim2 = argv[1];
	    break;

	case MMDLVFILE:
	    dlvfile = argv[1];
	    break;

    	case MMAXHOPS:
    	    maxhops = atoi(argv[1]);
    	    break;

    	case MADDID:
    	    mgt_addid = atoi(argv[1]);
    	    break;

        case MADDIPADDR:
            mgt_addipaddr = atoi(argv[1]);
            break;

        case MADDIPNAME:
            mgt_addipname = atoi(argv[1]);
            break;

    	case MLISTSIZE:
	    lnk_listsize = atoi(argv[1]);
    	    break;

        case MMAXMBSZ:
          cnvtbytestr(argv[1], &mbox_quota_soft);
          cnvtbytestr(argv[2], &mbox_quota_hard);
          break;
          
        case UUname:
	    Uuname = argv[1];
	    break;

	case UUxstr:
	    Uuxstr = argv[1];
	    break;

	case MMTBL:
	    retval = tb_tai (argc, &argv[1]);
	    break;

	case MMDFLCHAN:
	    ch_dflnam = argv[1];
	    break;

	case MMCHAN:
	    retval = ch_tai (argc, &argv[1]);
	    break;

	case MMMAILIDS:         /* enable/disable use of Mail-Ids */
	    mid_enable = atoi(argv[1]);
	    break;

	case MMDOMAIN:
	    retval = dm_tai (argc, &argv[1]);
	    break;

	case NIQUEDIR:
	    pn_quedir = argv[1];
	    break;

	case ALIAS:
      al_tai (argc, &argv[1]);
      break;

        case MMVALIDCHN:
          valid_channels = argv[1];
          break;
        
#ifdef HAVE_ESMTP
        case MMSGSIZELIMIT:
          message_size_limit = atol(argv[1]);
          break;
#   ifdef HAVE_ESMTP_8BITMIME
        case M8BITMIME:
          accept_8bitmime = atoi(argv[1]);
          break;
#   endif /* HAVE_ESMTP_8BITMIME */
#   ifdef HAVE_ESMTP_DSN
        case MDSN:
          dsn = atoi(argv[1]);
          break;
#   endif /* HAVE_ESMTP_DSN */
#endif /* HAVE_ESMTP */
    }

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "m_tai (ret=%d)", retval);
#endif

    return (retval);
}
/**/

extern Table    **tb_list;
extern int  tb_maxtables;
extern int  tb_numtables;

#define CMDTBASE    1
#define CMDTNAME    2
#define CMDTFILE    3
#define CMDTSHOW    4
#define CMDTFLAGS   5
#define CMDTNOOP    6
#define CMDTTYPE    7

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",         CMDTNOOP,   0},
    {"base",     CMDTBASE,   1},
    {"file",     CMDTFILE,   1},
    {"flags",	 CMDTFLAGS,  1},
    {"name",     CMDTNAME,   1},
    {"show",     CMDTSHOW,   1},
    {"type",     CMDTTYPE,   1},
    {0,          0,          0}
};

#define CMDTBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)

#define CMDTFFILE	1
#define CMDTFDBM	2
#define CMDTFNS		3
#define CMDTFDOMAIN     4
#define CMDTFCHANNEL    5
#define CMDTFPARTIAL    7
#define CMDTFABORT      8
#define CMDTFROUTE      9
#define CMDTFREJECT    10
#ifdef HAVE_NIS
#  define CMDTFNIS     11
#endif /* HAVE_NIS */

LOCVAR Cmd
	tbflags [] =
{
    {"abort",    CMDTFABORT,     0},
    {"channel",  CMDTFCHANNEL,   0},
    {"dbm",	     CMDTFDBM,	0},
    {"domain",   CMDTFDOMAIN,    0},
    {"file",	 CMDTFFILE,	0},
#ifdef HAVE_NIS
    {"nis",      CMDTFNIS,       0},
#endif /* HAVE_NIS */
    {"ns",  	 CMDTFNS,	0},
    {"partial",  CMDTFPARTIAL,   0},
    {"reject",   CMDTFREJECT,    0},
    {"route",    CMDTFROUTE,     0},
    {0,          0,              0}
};

#define TBENT ((sizeof(tbflags)/sizeof(Cmd))-1)

#define CMDTFTFILE   1
#define CMDTFTDBM    2
#define CMDTFTNS     3
#define CMDTFTNIS    4
#define CMDTFTREGEXP 5
#define CMDTFTRBL    6
#define CMDTFTLDAP   7
#define CMDTFTSQL    8
#define CMDTFTTEST   9

LOCVAR Cmd
	tbtype [] =
{
  { "dbm",   CMDTFTDBM,     0 },
  { "file",  CMDTFTFILE,    0 },
#ifdef HAVE_LDAP
  { "ldap",  CMDTFTLDAP,    0 },
#endif /* HAVE_LDAP */
#ifdef HAVE_NIS
  { "nis",   CMDTFTNIS,     0 },
#endif /* HAVE_NIS */
  { "ns",    CMDTFTNS,      0 },
#ifdef HAVE_RBL
  { "rbl",   CMDTFTRBL,     0 },
#endif /* HAVE_RBL */
#ifdef HAVE_REGEXEC
  { "regexp",CMDTFTREGEXP,  0 },
#endif /* HAVE_REGEXEC */
#ifdef HAVE_SQLSUPPORT
  { "sql",   CMDTFTSQL,     0 },
#endif /* HAVE_SQLSUPPORT */
  { "test",  CMDTFTTEST,    0 },
  { 0,      0, 0 }
};
#define TBTYPE ((sizeof(tbtype)/sizeof(Cmd))-1)

struct tblinistruct
{
  int tb_type;
  int (*tb_init)(); 
};
typedef struct tblinistruct TblInit;

LOCVAR TblInit tbl_init[] =
{
  { TBT_FILE, tb_noop },
  { TBT_FILE, tb_file_init },
  { TBT_DBM,  tb_dbm_init },
  { TBT_NS,   tb_ns_init },
  { TBT_NIS,  tb_nis_init },
  { TBT_REGEX,tb_regexp_init },
  { TBT_RBL,  tb_rbl_init },
  { TBT_LDAP, tb_ldap_init },
  { TBT_SQL,  tb_sql_init },
  { TBT_TEST, tb_test_init },
  { 0, 0, }
};

int tb_noop(tblptr)
    Table *tblptr;
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "###tb_noop (%p: %d)", tblptr);
#endif
  return;
}

int tb_tai (argc, argv)
    int argc;
    char *argv[];
{
    int ind;
    int type;
    register Table *tbptr;
#ifdef HAVE_NIS
    extern Table tb_mailids;
    extern Table tb_users;
#endif /* HAVE_NIS */
#ifdef BACKWARD_COMPAT
    int tb_ind = 0;
    char tb_bw_text[LINESIZE];
#endif /* BACKWARD_COMPAT */
    
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tb_tai (%s: %d)", argv[0], argc);
#endif

    if (tb_numtables >= tb_maxtables)
    {
	tai_error ("exceed table size", argv[0], argc, argv);
	return (NOTOK);
    }
#ifdef DEBUG
    if (logptr -> ll_level == LLOGFTR)
    {
        ll_log (logptr, LLOGFTR, "old tables (%d)", tb_numtables);
        for (ind = 0; tb_list[ind] != (Table *) 0; ind++)
    	    ll_log (logptr, LLOGFTR, "tb(%d) '%s'", 
		ind, tb_list[ind] -> tb_name);
    }
#endif

    /*NOSTRICT*/
    tb_list[tb_numtables++] = tbptr =
			(Table *) malloc ((unsigned) (sizeof (Table)));
    tb_list[tb_numtables] = (Table *) 0;
    memset(tbptr, 0, (unsigned) (sizeof (Table)));
    tbptr -> tb_name = strdup("notablename");
    tbptr -> tb_flags      = TB_FILE;
    tbptr -> tb_type       = TBT_FILE;

    /* find first the type-field */
#ifdef BACKWARD_COMPAT
    memset(tb_bw_text, 0, sizeof(tb_bw_text));
#endif /* BACKWARD_COMPAT */
    for (ind = 0; ind < argc; ind++)
    {
      if((cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT) == CMDTTYPE) &&
         (ind<argc-1)) {
        type = cmdbsrch (argv[ind+1], argc - ind - 1, tbtype, TBTYPE);
        tbptr -> tb_type = tbl_init[type].tb_type;
        
        if(tbl_init[type].tb_init != 0)
          tbl_init[type].tb_init(tbptr);
      }
#ifdef BACKWARD_COMPAT
      if((cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT) == CMDTFLAGS) &&
         (ind<argc-1)) {
    
        switch (cmdbsrch (argv[ind+1], 0, tbflags, TBENT))
        {
            case CMDTFDBM:
              tbptr -> tb_type = TBT_DBM;
              strncat(tb_bw_text, "flags=dbm, use type=dbm",
                      sizeof(tb_bw_text)-strlen(tb_bw_text));
              break;

            case CMDTFNS:
              tbptr -> tb_type = TBT_NS;
              strncat(tb_bw_text, "flags=ns, use type=ns",
                      sizeof(tb_bw_text)-strlen(tb_bw_text));
              break;
#ifdef HAVE_NIS
            case CMDTFNIS:
              tbptr -> tb_type = TBT_NIS;
              strncat(tb_bw_text, "flags=, use type=nis",
                      sizeof(tb_bw_text)-strlen(tb_bw_text));
              break;
#endif /* HAVE_NIS */
        }
      }
#endif /* BACKWARD_COMPAT */
    }

    /* parse now the other arguments */
    for (ind = 0; ind < argc; ind++)
    {
	if (!lexequ (argv[ind], "="))
	{
	    if (ind == 0)
	    {                   /* ok to default first field only */
#ifdef DEBUG
		ll_log (logptr, LLOGBTR, "defaulting base '%s'", argv[ind]);
#endif
		goto dobase;
	    }
	    else
	    {
		tai_error ("unassignable, bare data field",
				argv[ind], argc, argv);
		continue;
	    }
	}
	else
	{
	    ind += 2;
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "tb '%s'(%d)", argv[ind - 1], argc - ind);
#endif
	    switch (cmdbsrch (argv[ind - 1], argc - ind, cmdtbl, CMDTBENT))
	    {
            case CMDTBASE:
              dobase:
              tbptr -> tb_name =  argv[ind];
              tbptr -> tb_show =  argv[ind];
              tbptr -> tb_file =  argv[ind];
              break;

            case CMDTNAME:
              tbptr -> tb_name =  argv[ind];
              break;

            case CMDTFILE:
              tbptr -> tb_file =  argv[ind];
              break;

            case CMDTSHOW:
              tbptr -> tb_show=  argv[ind];
              break;

	    	case CMDTFLAGS:
#ifdef DEBUG
              ll_log (logptr, LLOGFTR, "table flag '%s'", argv[ind]);
#endif
              switch (cmdbsrch (argv[ind], 0, tbflags, TBENT))
              {
                  case CMDTFFILE:
                  case CMDTFDBM:
                  case CMDTFNS:
#ifdef HAVE_NIS
                  case CMDTFNIS:
#endif /* HAVE_NIS */
                    /* these switches are obsolete */
                    break;

                  case CMDTFDOMAIN:
                    tbptr -> tb_flags |= TB_DOMAIN;
                    break;

                  case CMDTFCHANNEL:
                    tbptr -> tb_flags |= TB_CHANNEL;
                    break;

                  case CMDTFPARTIAL:
                    tbptr -> tb_flags |= TB_PARTIAL;
                    break;

                  case CMDTFABORT:
                    tbptr -> tb_flags |= TB_ABORT;
                    break;

                  case CMDTFROUTE:
                    tbptr -> tb_flags |= TB_ROUTE;
                    break;
                
                  case CMDTFREJECT:
                    tbptr -> tb_flags |= TB_REJECT;
                    break;
                
                  default:
                    tai_error ("unknown table flag", argv[ind-1],
                               argc, argv);
                    continue;
              }
              break;

            case CMDTNOOP:
              break;      /* noop */

            case CMDTTYPE:
              break;
              
            default:
              if(tbptr -> tb_tai != NULL) {
                if(tbptr -> tb_tai(tbptr, &ind, argc, &argv[0])){
                  tai_error ("unknown table parm", argv[ind], argc, argv);
                }
              }
              else
                tai_error ("unknown table parm", argv[ind], argc, argv);
              break;
	    }
	}
    }
#ifdef BACKWARD_COMPAT
    if(strlen(tb_bw_text)>0) {
      ll_log (logptr, LLOGGEN, "Table '%s' has old syntax %s",
                      tbptr -> tb_name, tb_bw_text);
      if(verbose_tai)
        printf("**   Table '%s' has old syntax: %s\n",
                      tbptr -> tb_name, tb_bw_text);
    }
#endif /* BACKWARD_COMPAT */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "tb_tai (ret=YES)");
    ll_log (logptr, LLOGFTR, "name=%s, ref=%s, file=%s, flags=%0o",
		tbptr -> tb_name, tbptr -> tb_show, tbptr -> tb_file,
		tbptr -> tb_flags);
#endif

#ifdef HAVE_NIS
    if( strcmp(tbptr -> tb_name, "users") == 0) {
      tb_users.tb_flags = tbptr -> tb_flags;
      tb_users.tb_file  = tbptr -> tb_file;
      tb_users.tb_type  = tbptr -> tb_type;
      tb_users.tb_parameters = tbptr -> tb_parameters;
      tb_users.tb_tai   = tbptr -> tb_tai;
      tb_users.tb_fetch = tbptr -> tb_fetch;
      tb_users.tb_print = tbptr -> tb_print;
      tb_users.tb_check = tbptr -> tb_check;
      free(tb_list[tb_numtables]);
      tb_numtables--;
    }
    if( strcmp(tbptr -> tb_name, "mailids") == 0) {
      tb_mailids.tb_flags = tbptr -> tb_flags;
      tb_mailids.tb_file  = tbptr -> tb_file;
      tb_mailids.tb_type  = tbptr -> tb_type;
      tb_mailids.tb_parameters = tbptr -> tb_parameters;
      tb_mailids.tb_tai   = tbptr -> tb_tai;
      tb_mailids.tb_fetch = tbptr -> tb_fetch;
      tb_mailids.tb_print = tbptr -> tb_print;
      tb_mailids.tb_check = tbptr -> tb_check;
      free(tb_list[tb_numtables]);
      tb_numtables--;
    }
#endif /* HAVE_NIS */
    return (YES);
}
/**/

#include "dm.h"

extern Domain   **dm_list;
extern int  dm_maxtables;
extern int  dm_numtables;

#define CMDDBASE    1
#define CMDDNAME    2
#define CMDDDMN     3
#define CMDDSHOW    4
#define CMDDTABLE   5
#define CMDDNOOP    7
#define CMDDLNAME   8

LOCVAR Cmd
	    dmntbl[] =
{
    {"",         CMDDNOOP,   0},
    {"base",     CMDDBASE,   1},
    {"dmn",      CMDDDMN,    1},
    {"name",     CMDDNAME,   1},
    {"show",     CMDDSHOW,   1},
    {"table",    CMDDTABLE,  1},
    {0,          0,          0}
};

#define DMTABENT ((sizeof(dmntbl)/sizeof(Cmd))-1)

int dm_tai (argc, argv)
    int argc;
    char *argv[];
{
    int ind;
    int tbind,
	showind;
    char *tbname;
    register Domain *dmnptr;

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "dm_tai (%s: %d)", argv[0], argc);
#endif

    if (dm_numtables >= dm_maxtables)
    {
	tai_error ("exceed domain table size", argv[0], argc, argv);
	return (NOTOK);
    }
#ifdef DEBUG
    if (logptr -> ll_level == LLOGFTR)
    {
        ll_log (logptr, LLOGFTR, "old domains (%d)", dm_numtables);
        for (ind = 0; dm_list[ind] != (Domain *) 0; ind++)
    	    ll_log (logptr, LLOGFTR, "dm(%d) '%s'", 
                    ind, dm_list[ind] -> dm_name);
    }
#endif

    /*NOSTRICT*/
    dm_list[dm_numtables++] = dmnptr =
			(Domain *) malloc ((unsigned) (sizeof (Domain)));
    dm_list[dm_numtables] = (Domain *) 0;
    dmnptr -> dm_name     = "nodomainname";
    dmnptr -> dm_domain   =  (char *) 0;
    dmnptr -> dm_show     =  (char *) 0;
    dmnptr -> dm_table    =  (Table *) 0;
    tbind = -1;
    showind = -1;

    for (ind = 0; ind < argc; ind++)
    {
	if (!lexequ (argv[ind], "="))
	{
	    if (ind == 0)
	    {                   /* ok to default first field only */
#ifdef DEBUG
		ll_log (logptr, LLOGBTR, "defaulting base '%s'", argv[ind]);
#endif
		goto dobase;
	    }
	    else
	    {
		tai_error ("unassignable, bare data field", argv[ind], argc, argv);
		continue;
	    }
	}
	else
	{
	    ind += 2;
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "dm '%s'(%d)", argv[ind - 1], argc - ind);
#endif
	    switch (cmdbsrch (argv[ind - 1], argc - ind, dmntbl,DMTABENT))
	    {
		case CMDDBASE:
	dobase:
		    dmnptr -> dm_name   = argv[ind];
		    dmnptr -> dm_domain = argv[ind];
		    showind = ind;
		    break;

		case CMDDNAME:
		    dmnptr -> dm_name =  argv[ind];
		    break;

		case CMDDDMN:
		    dmnptr -> dm_domain =  argv[ind];
		    break;

		case CMDDSHOW:
		    showind = ind;
		    break;

		case CMDDTABLE:
		    tbind = ind;
		    break;

		case CMDDNOOP:
		    break;      /* noop */

		default:
		    argv[ind + 1] = (char *) 0; /* eliminate rest of args */
		    tai_error ("unknown domain parm", argv[ind-1], argc, argv);
		    break;
	    }
	}
    }
    tbname = (tbind >= 0) ? argv[tbind] : dmnptr -> dm_name;
    if ((dmnptr -> dm_table = tb_nm2struct (tbname)) == (Table *) NOTOK)
    {                   /* table does not already exist */
#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "'%s' unknown domain table", tbname);
#endif
	if (tb_numtables >= tb_maxtables)
	{
	    tai_error ("exceed tb_list table size", tbname, argc, argv);
	}
	else
	{
	    /*NOSTRICT*/
	    tb_list[tb_numtables++] = dmnptr -> dm_table =
		(Table *) malloc ((unsigned) (sizeof (Table)));
	    tb_list[tb_numtables] = (Table *) 0;
#ifdef DEBUG
	    ll_log (logptr, LLOGBTR, "allocating dm_table addr (%o)", dmnptr -> dm_table);
#endif
	    dmnptr -> dm_table -> tb_name =  tbname;
	    dmnptr -> dm_table -> tb_file =  tbname;
	    dmnptr -> dm_table -> tb_fp   =  (FILE *) NULL;
	    dmnptr -> dm_table -> tb_pos  =  0L;
	    dmnptr -> dm_table -> tb_show =
		(showind >= 0 ? argv[showind] : tbname);
	}
    }
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "dm_table addr (%o)", dmnptr -> dm_table);
#endif

    dmnptr -> dm_show = (showind >= 0 ? argv[showind] : dmnptr -> dm_name);

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "dm_tai (ret=YES)");
    ll_log (logptr, LLOGFTR, "domain '%s'('%s'), table='%s'(%s)",
		dmnptr -> dm_show, dmnptr -> dm_name,
		dmnptr -> dm_table -> tb_show, dmnptr -> dm_table -> tb_name);
    ll_log (logptr, LLOGBTR, "dm_table addr (%o)", dmnptr -> dm_table);
#endif

    return (YES);
}
/**/
extern Alias   *al_list;

int al_tai (argc, argv)
    int argc;
    char *argv[];
{
    int ind;
    register Alias *alptr, *palptr;

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "al_tai (%s: %d)", argv[0], argc);
#endif

    /*NOSTRICT*/
    alptr = (Alias *) malloc ((unsigned) (sizeof (Alias)));
    alptr -> al_flags = 0;
    alptr -> al_next = NULL;

    for (ind = 0; ind < argc; ind++)
    {
	if (!lexequ (argv[ind], "="))
	{
	    if (lexequ (argv[ind], "trusted"))
		alptr->al_flags |= AL_TRUSTED;
	    else if (lexequ (argv[ind], "nobypass"))
		alptr->al_flags |= AL_NOBYPASS;
	    else if (lexequ (argv[ind], "public"))
		alptr->al_flags |= AL_PUBLIC;
	    else {
		tai_error ("unassignable, alias field", argv[ind], argc, argv);
		continue;
	    }
	} else {
	    ind += 2;
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "al '%s'(%d)", argv[ind - 1], argc - ind);
#endif
	    if (lexequ(argv[ind-1], "table")) {
		if ((alptr->al_table = tb_nm2struct(argv[ind]))
		   == (Table *)NOTOK)
		{
#ifdef DEBUG
		    ll_log (logptr, LLOGBTR, "'%s' unknown alias table", argv[ind]);
#endif
		    if (tb_numtables >= tb_maxtables) {
			tai_error ("exceed tb_list table size", argv[ind], argc, argv);
		    } else {
			/*NOSTRICT*/
			tb_list[tb_numtables++] = alptr->al_table =
			    (Table *) malloc ((unsigned) (sizeof (Table)));
			tb_list[tb_numtables] = (Table *) 0;
			alptr -> al_table -> tb_name =  argv[ind];
			alptr -> al_table -> tb_file =  argv[ind];
			alptr -> al_table -> tb_fp   =  (FILE *) NULL;
			alptr -> al_table -> tb_pos  =  0L;
		    	alptr -> al_table -> tb_show =  argv[ind];
		    }
		}
	    } else {
		argv[ind + 1] = (char *) 0; /* eliminate rest of args */
		tai_error ("unknown alias parm", argv[ind-1], argc, argv);
		break;
	    }
	}
    }
    if (al_list == (Alias *)0)
	al_list = alptr;
    else {
	for (palptr = al_list; palptr->al_next != NULL; palptr = palptr->al_next);
	palptr->al_next = alptr;
    }
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "al_tai (ret=YES)");
#endif
    return (YES);
}
/**/

extern Chan **ch_tbsrch;
extern int  ch_maxchans;
extern int  ch_numchans;

#define CMDCBASE    1
#define CMDCNAME    2
#define CMDCSHOW    3
#undef	Gonzo      /*  4    / * OBSOLETE */
#define CMDCQUE     5
#define CMDCTBL     6
#define CMDCPGM     7
#define CMDCHOST    8
#define CMDCUSER    9
#define CMDCSCRIPT 10
#define CMDCPOLL   11
#define CMDCACCESS 12
#define CMDCNOOP   13
#define CMDCLNAME  14
#define CMDCLDOMN  15
#define CMDCLISRC  16
#define CMDCLOSRC  17
#define CMDCLIDEST 18
#define CMDCLODEST 19
#define CMDCKNOWN  20
#define CMDCCONFSTR 21
#define CMDCAP     22
#define CMDCAUTH   23
#define CMDCTRANS  24
#define CMDCTTL    25
#define CMDCLLOG   26
#define CMDCLLEV   27
#define CMDCEDIT   28
#define CMDCFAILTIM 29
#define CMDCWARNTIM 30

LOCVAR Cmd
	    cmdchan[] =
{
    {"",         CMDCNOOP,   0},
    {"ap",       CMDCAP,     1},
    {"auth",     CMDCAUTH,   1},
    {"base",     CMDCBASE,   1},
    {"confstr",  CMDCCONFSTR,1},
    {"failtime", CMDCFAILTIM,1},
    {"host",     CMDCHOST,   1},
    {"indest",   CMDCLIDEST, 1},
    {"insrc",    CMDCLISRC,  1},
    {"known",    CMDCKNOWN,  1},
    {"ldomain",  CMDCLDOMN,  1},
    {"level",    CMDCLLEV,   1},
    {"lname",    CMDCLNAME,  1},
    {"log",      CMDCLLOG,   1},
    {"mod",      CMDCACCESS, 1},
    {"name",     CMDCNAME,   1},
    {"outdest",  CMDCLODEST, 1},
    {"outsrc",   CMDCLOSRC,  1},
    {"pgm",      CMDCPGM,    1},
    {"poll",     CMDCPOLL,   1},
    {"que",      CMDCQUE,    1},
    {"scr",      CMDCSCRIPT, 1},
    {"show",     CMDCSHOW,   1},
    {"tbl",      CMDCTBL,    1},
    {"trn",      CMDCTRANS,  1},
    {"ttl",      CMDCTTL,    1},
    {"user",     CMDCUSER,   1},
    {"warntime", CMDCWARNTIM,1},
    {0,          0,          0}
};

#define CMDCHANENT ((sizeof(cmdchan)/sizeof(Cmd))-1)

#define CMDPREG     1
#define CMDPBAK     2
#define CMDPPSV     3
#define CMDPDID     4
#define CMDPIMM     5
#define CMDPPICK    6
#define CMDPSEND    7
#define CMDPNOOP    8
#define CMDPHOST    9
#ifdef HAVE_ESMTP
#  define CMDPESMTP 10
#endif

LOCVAR Cmd
	    parmchan[] =
{
    {"",         CMDPNOOP,   0},
    {"bak",      CMDPBAK,    0},
#ifdef HAVE_ESMTP
    {"esmtp",    CMDPESMTP,  0},
#endif
    {"host",     CMDPHOST,   0},
    {"imm",      CMDPIMM,    0},
    {"pick",     CMDPPICK,   0},
    {"psv",      CMDPPSV,    0},
    {"reg",      CMDPREG,    0},
    {"send",     CMDPSEND,   0},
    {0,          0,          0}
};

#define PARMENT ((sizeof(parmchan)/sizeof(Cmd))-1)


#define CMDASAME        1
#define CMDA733         2
#define CMDA822         3
#define CMDABIG         4
#define CMDANODOTS      5
#define CMDAJNT         6
#define CMDATRY         7
#ifdef HAVE_NOSRCROUTE
#  define CMDANOSRCRT   8
#  define CMDAREJSRCRT  9
#endif



LOCVAR Cmd
	parmap [] =
{
    {"733",      CMDA733,        0},
    {"822",      CMDA822,        0},
    {"big",      CMDABIG,        0},
    {"jnt",      CMDAJNT,        0},
    {"nodots",   CMDANODOTS,     0},
#ifdef HAVE_NOSRCROUTE
    {"nosrcrt",  CMDANOSRCRT,    0},
    {"rejsrcrt", CMDAREJSRCRT,   0},
#endif
    {"same",     CMDASAME,       0},
    {"try",	     CMDATRY,	0},
    {0,          0,              0}
};

#define PARMAPENT ((sizeof(parmap)/sizeof(Cmd))-1)

#define CMDTFREE        0
#define CMDTINLOG       1
#define CMDTINWARN      2
#define CMDTINBLOCK     3
#define CMDTOUTLOG      4
#define CMDTOUTWARN     5
#define CMDTOUTBLOCK    6
#define CMDTHAU         7
#define CMDTDHO         8

LOCVAR  Cmd
	authmap [] =
{
    {"dho",      CMDTDHO,        0},
    {"free",     CMDTFREE,       0},
    {"hau",      CMDTHAU,        0},
    {"inblock",  CMDTINBLOCK,    0},
    {"inlog",    CMDTINLOG,      0},
    {"inwarn",   CMDTINWARN,     0},
    {"outblock", CMDTOUTBLOCK,   0},
    {"outlog",   CMDTOUTLOG,     0},
    {"outwarn",  CMDTOUTWARN,    0},
    {0,          0,              0}
};

#define AUTHMAPENT	((sizeof(authmap)/sizeof(Cmd))-1)

int ch_tai (argc, argv)
    int argc;
    char *argv[];
{
    int ind;
    int tbind,
	showind;
    char *tbname;
    register Chan *chptr;

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "ch_tai (%s: %d)", argv[0], argc);
#endif

    if (ch_numchans >= ch_maxchans)
    {
	tai_error ("exceed ch table size", argv[0], argc, argv);
	return (NOTOK);
    }
#ifdef DEBUG
    if ( logptr -> ll_level == LLOGFTR ) {
	ll_log (logptr, LLOGFTR, "old channels (%d)", ch_numchans);
	for (ind = 0; ch_tbsrch[ind] != (Chan *) 0; ind++)
	    ll_log (logptr, LLOGFTR, "ch(%d) '%s'",
		ind, ch_tbsrch[ind] -> ch_name);
    }
#endif

    /*NOSTRICT*/
    ch_tbsrch[ch_numchans++] = chptr =
			(Chan *) malloc ((unsigned) (sizeof (Chan)));
    ch_tbsrch[ch_numchans] = (Chan *) 0;

    chptr -> ch_name   =  "nochanname";
    chptr -> ch_show   =  (char *) 0;
    chptr -> ch_table  =  (Table *) 0;
    chptr -> ch_queue  =  (char *) 0;
    chptr -> ch_access =  0;                    /* == DLVRREG */
    chptr -> ch_ppath  =  (char *) 0;
    chptr -> ch_lname  =  locname;
    chptr -> ch_ldomain = locdomain;
    chptr -> ch_host   =  (char *) NORELAY;
    chptr -> ch_login  =  (char *) NOLOGIN;
    chptr -> ch_poltime = (char) 8; /* every two hours */
    chptr -> ch_script =  (char *) 0;
    chptr -> ch_trans  =  (char *) DEFTRANS;
    chptr -> ch_insource = (Table *) 0;
    chptr -> ch_outsource = (Table *) 0;
    chptr -> ch_indest  =  (Table *) 0;
    chptr -> ch_outdest =  (Table *) 0;
    chptr -> ch_confstr = (char *) 0;
    chptr -> ch_apout = AP_SAME;
    chptr -> ch_known = (Table  *) 0;
    chptr -> ch_auth = CH_FREE;
    chptr -> ch_dead = (Cache *) 0;
    chptr -> ch_ttl = (time_t)7200;  /* dead cache TimeToLive, two hours */
    chptr -> ch_logfile = chanlog.ll_file;
    chptr -> ch_loglevel = chanlog.ll_level;
    chptr -> ch_warntime = -1;
    chptr -> ch_failtime = -1;
    tbind = -1;
    showind = -1;

    for (ind = 0; ind < argc; ind++)
    {
	if (!lexequ (argv[ind], "="))
	{                       /* not an 'assignment' parameter        */
	    if (ind == 0)
	    {
#ifdef DEBUG
		ll_log (logptr, LLOGBTR, "defaulting base '%s'", argv[ind]);
#endif
		goto dobase;    /* default first field only */
	    }
	    else
	    {
		tai_error ("unassignable, bare channel field", argv[ind], argc, argv);
		continue;
	    }
	}
	else
	{
	    ind += 2;
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "ch '%s'(%d)", argv[ind - 1], argc - ind);
#endif
	    switch (cmdbsrch (argv[ind - 1], argc - ind, cmdchan, CMDCHANENT))
	    {
		case CMDCBASE:
	dobase:
		    chptr -> ch_name = argv[ind];
		    chptr -> ch_queue = argv[ind];
		    chptr -> ch_ppath = argv[ind];
		    break;

		case CMDCNAME:
		    chptr -> ch_name =  argv[ind];
		    break;

		case CMDCSHOW:
		    showind = ind;
		    break;

		case CMDCQUE:
		    chptr -> ch_queue =  argv[ind];
		    break;

		case CMDCTBL:
		    tbind = ind;
		    break;

		case CMDCPGM:
		    chptr -> ch_ppath =  argv[ind];
		    break;

		case CMDCHOST:
		    chptr -> ch_host =  argv[ind];
		    break;

		case CMDCUSER:
		    chptr -> ch_login =  argv[ind];
		    break;

		case CMDCSCRIPT:
		    chptr -> ch_script =  argv[ind];
		    break;

		case CMDCTRANS:
		    chptr -> ch_trans = argv[ind];
		    break;

		case CMDCPOLL:
		    chptr -> ch_poltime =  atoi (argv[ind]);
		    break;

		case CMDCTTL:
		    /*NOSTRICT*/
		    chptr -> ch_ttl = (time_t) atoi (argv[ind]) * 60L;
		    break;

		case CMDCLLOG:
		    chptr -> ch_logfile = argv[ind];
		    break;

		case CMDCLLEV:
		    if ((chptr -> ch_loglevel = tai_llev (argc, &argv[ind])) < 0)
			chptr -> ch_loglevel = chanlog.ll_level;
		    break;

		case CMDCACCESS:
#ifdef DEBUG
		    ll_log (logptr, LLOGFTR, "ch parm '%s'", argv[ind]);
#endif
		    switch (cmdbsrch (argv[ind], 0, parmchan, PARMENT))
		    {
			case CMDPREG:
			    chptr -> ch_access |= DLVRREG;
			    break;

			case CMDPBAK:
			    chptr -> ch_access |= DLVRBAK;
			    break;

			case CMDPPSV:
			    chptr -> ch_access |= DLVRPSV;
			    break;

			case CMDPIMM:
			    chptr -> ch_access |= DLVRIMM;
			    break;

			case CMDPPICK:
			    chptr -> ch_access |= CH_PICK;
			    break;

			case CMDPSEND:
			    chptr -> ch_access |= CH_SEND;
			    break;

			case CMDPHOST:
			    chptr -> ch_access |= DLVRHST;
			    break;

			case CMDPNOOP:
			    break;      /* noop */

#ifdef HAVE_ESMTP
			case CMDPESMTP:
			    chptr -> ch_access |= CH_ESMTP;
			    break;
#endif
			default:
			    tai_error ("unknown access parm", argv[ind],
					argc, argv);
			    continue;
		    }
		    break;

		case CMDCNOOP:
		    break;      /* noop */

		case CMDCLNAME:
		    chptr -> ch_lname =  argv[ind];
		    break;

		case CMDCLDOMN:
		    chptr -> ch_ldomain =  argv[ind];
		    break;

		case CMDCLISRC:
		    if ((chptr -> ch_insource = tb_nm2struct (argv[ind]))
							== (Table *) NOTOK)
		    {
			tai_error ("unknown source table", argv[ind], argc, argv);
			continue;
		    }
		    break;

		case CMDCLOSRC:
		    if ((chptr -> ch_outsource = tb_nm2struct (argv[ind]))
							== (Table *) NOTOK)
		    {
			tai_error ("unknown source table", argv[ind], argc, argv);
			continue;
		    }
		    break;

		case CMDCLIDEST:
		    if ((chptr -> ch_indest = tb_nm2struct (argv[ind]))
							== (Table *) NOTOK)
		    {
			tai_error ("unknown dest table", argv[ind], argc, argv);
			continue;
		    }
		    break;

		case CMDCLODEST:
		    if ((chptr -> ch_outdest = tb_nm2struct (argv[ind]))
							== (Table *) NOTOK)
		    {
			tai_error ("unknown dest table", argv[ind], argc, argv);
			continue;
		    }
		    break;

		case CMDCKNOWN:
		    if ((chptr -> ch_known = tb_nm2struct (argv[ind]))
							== (Table *) NOTOK)
		    {
			tai_error ("unknown known table", argv[ind], argc, argv);
			continue;
		    }
		    break;

		case CMDCCONFSTR:
		    chptr -> ch_confstr = argv [ind];
		    break;

		case CMDCAP:
#ifdef DEBUG
		    ll_log (logptr, LLOGFTR, "Ap style '%s'", argv[ind]);
#endif
		    switch (cmdbsrch (argv[ind], 0, parmap, PARMAPENT))
		    {
			case CMDASAME:
			    chptr -> ch_apout = (chptr -> ch_apout&0700)|AP_SAME;
			    break;
			case CMDA733:
			    chptr -> ch_apout = (chptr -> ch_apout&0700)|AP_733;
			    break;
			case CMDA822:
			    chptr -> ch_apout = (chptr -> ch_apout&0700)|AP_822;
			    break;
			case CMDABIG:
			    chptr -> ch_apout |= AP_BIG;
			    break;
			case CMDANODOTS:
			    chptr -> ch_apout |= AP_NODOTS;
			    break;
			case CMDATRY:
			    chptr -> ch_apout |= AP_TRY;
			    break;
			case CMDAJNT:
			     chptr -> ch_apout = (AP_733 | AP_BIG);
			     break;
#ifdef HAVE_NOSRCROUTE
			case CMDANOSRCRT:
			    chptr -> ch_apout |= AP_NOSRCRT;
			    break;
			case CMDAREJSRCRT:
			    chptr -> ch_apout |= AP_REJSRCRT;
			    break;
#endif
			default:
			    tai_error ("unknown address style", argv[ind], argc, argv);
			    continue;
		    }
#ifdef DEBUG
		    ll_log (logptr, LLOGFTR, "Ap style %s '%o'",
			    chptr -> ch_name, chptr -> ch_apout);
#endif
		    break;

		case CMDCAUTH:
#ifdef DEBUG
		    ll_log (logptr, LLOGFTR, "Authorisation '%s'", argv[ind]);
#endif
		    switch (cmdbsrch (argv[ind], 0, authmap, AUTHMAPENT))
		    {
			case CMDTFREE:
			    chptr -> ch_auth |= CH_FREE;
			    break;
			case CMDTINLOG:
			    chptr -> ch_auth |= CH_IN_LOG;
			    break;
			case CMDTINWARN:
			    chptr -> ch_auth |= CH_IN_WARN;
			    break;
			case CMDTINBLOCK:
			    chptr -> ch_auth |= CH_IN_BLOCK;
			    break;
			case CMDTOUTLOG:
			    chptr -> ch_auth |= CH_OUT_LOG;
			    break;
			case CMDTOUTWARN:
			    chptr -> ch_auth |= CH_OUT_WARN;
			    break;
			case CMDTOUTBLOCK:
			    chptr -> ch_auth |= CH_OUT_BLOCK;
			    break;
			case CMDTHAU:
			    chptr -> ch_auth |= CH_HAU;
			    break;
			case CMDTDHO:
			    chptr -> ch_auth |= CH_DHO;
			    break;

			default:
			    tai_error ("unknown auth type", argv[ind], argc, argv);
			    continue;
		    }
		    break;

		case CMDCFAILTIM:
		    chptr -> ch_failtime =  atoi (argv[ind]);
		    break;

		case CMDCWARNTIM:
		    chptr -> ch_warntime =  atoi (argv[ind]);
		    break;

		default:
		    tai_error ("unknown channel parm", argv[ind], argc, argv);
		    break;
	    }
	}
    }
    tbname = (tbind >= 0) ? argv[tbind] : chptr -> ch_name;
    if ((chptr -> ch_table = tb_nm2struct (tbname)) == (Table *) NOTOK)
    {                   /* table does not already exist */
#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "'%s' unknown table", tbname);
#endif
	if (tb_numtables >= tb_maxtables)
	{
	    tai_error ("exceed tb table size", tbname, argc, argv);
	    return (NOTOK);
	}
	else
	{
	    /*NOSTRICT*/
	    tb_list[tb_numtables++] = chptr -> ch_table =
		(Table *) malloc ((unsigned) (sizeof (Table)));
	    tb_list[tb_numtables] = (Table *) 0;
	    chptr -> ch_table -> tb_name =  tbname;
	    chptr -> ch_table -> tb_file =  tbname;
	    chptr -> ch_table -> tb_fp   =  (FILE *) NULL;
	    chptr -> ch_table -> tb_pos  =  0L;
	    chptr -> ch_table -> tb_show =
		(showind >= 0 ? argv[showind] : tbname);
	}
    }
    chptr -> ch_show = (showind >= 0 ? argv[showind] : chptr -> ch_name);

    for (ind = 0; ch_tbsrch[ind] != (Chan *) 0; ind++) {
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "ch(%d) '%s'",
	    ind, ch_tbsrch[ind] -> ch_name);
#endif
	if (ch_tbsrch[ind] == chptr)
	   continue;
	if (lexequ(ch_tbsrch[ind] -> ch_name, chptr -> ch_name)) {
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "Replacing channel");
#endif
		ch_tbsrch[ind] = chptr;
		ch_tbsrch[--ch_numchans] = (Chan *) 0;
	}
    }

    /*
     * Get a default list of channels which aren't passive.
     * This is for deliver so that people can run with just
     * "deliver -b" and have it automatically handle all channels.
     * Yours for no-mess-no-fuss computing. -- DSH
     *	(Suggested-by: Hans van Staveren <sater@cs.vu.nl>)
     */
    if ((chptr -> ch_access & DLVRPSV) == 0) {
	extern Chan **ch_exsrch;

	for (ind = 0; ch_exsrch[ind] != (Chan *) 0; ind++)
	    if (lexequ(ch_exsrch[ind] -> ch_name, chptr -> ch_name))
		break;
	if (ch_exsrch[ind] == (Chan *) 0) {
	    ch_exsrch[ind] = chptr;
	}
    }

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ch_tai (ret=YES)");
    ll_log (logptr, LLOGFTR, "nam=%s, tbl=%s, que=%s, acc=0%o, path=%s",
	    chptr -> ch_name, chptr -> ch_show, chptr -> ch_queue,
	    chptr -> ch_access, chptr -> ch_ppath);
    ll_log (logptr, LLOGFTR, "lnam=%s, ldomain=%s, host=%s",
	    chptr -> ch_lname, chptr -> ch_ldomain, chptr -> ch_host);
    ll_log (logptr, LLOGFTR, "login=%s, pol=%d, script=%s",
	    chptr -> ch_login, (int) (chptr -> ch_poltime), chptr -> ch_script);
    ll_log (logptr, LLOGFTR, "ipcs=%s, ap=%d",
	chptr -> ch_confstr, chptr -> ch_apout);
#endif

    return (YES);
}

/**/

/*      The following is one of those ugly buggers that has to play
 *      hairy games, just to do something simple.  It uses the full
 *      log-tailoring call, just to convert a logging-level specification
 *      into a number.  Yuch.  (Dave Crocker)
 */

int tai_llev (argc, argv)            /* return logging level value */
     int argc;
     char *argv[];
{
    LLog faklog;
    char *fakargv[4];

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tai_llev (%s : %d)", argv[0], argc);
#endif

    fakargv[0] =  "=";
    fakargv[1] =  "level";
    fakargv[2] =  (char *) 0;
    fakargv[3] =  (char *) 0;

    if (argc <= 0)
	return (NOTOK);

    fakargv[2] = argv[0];       /* assign the user's string as the 'value' */
    faklog.ll_level = LLOGFST;  /* something reasonable? */
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tai_llev before (%d)", faklog.ll_level);
#endif
    if (tai_log (3, fakargv, &faklog) < 0)
    {
	tai_error ("bad log level", argv[0], argc, argv);
	return (NOTOK);
    }
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tai_llev after (%d)", faklog.ll_level);
#endif
    return (faklog.ll_level);
}

extern char *mmtailor;
void *read_variable(varname, value, type)
char *varname;
void **value;
int *type;
{
  void *ret = NULL;
  *type = VARTYPE_CHAR;
  
  if(varname != NULL)
    switch (cmdbsrch (varname, 1, cmdtab, CMDTABENT))
    {
        case NO:
        case MMNOOP:
        default:        /* command not known to us      */
          if(lexequ(varname, "mmdftailor")) {
            ret = (void *)mmtailor;
          }
          else
            *type = VARTYPE_NIL;
          break;

        case ALIAS:
        case MMDOMAIN:
        case MMCHAN:
        case MMTBL:
        case AUTHLOG:
        case MMCHANLOG:
        case MMMSGLOG:
          *type = VARTYPE_NIL;
          break;

        case MMLOCMACHINE:
          ret = (void *)locmachine;
	    break;

        case MMLOCHOST:
          ret = (void *)locname;
	    break;

        case MMLOCDOMAIN:
          ret = (void *)locdomain;
          break;

	case MMSIGN:
	    ret = (void *)sitesignature;
	    break;

	case MMLOGIN:
	    ret = (void *)mmdflogin;
	    break;

	case MMSUPPORT:
	    ret = (void *)supportaddr;
	    break;

	case MMLOGDIR:
	    ret = (void *)logdfldir;
	    break;

	case MMPHSDIR:
	    ret = (void *)phsdfldir;
	    break;

	case MMTBLDIR:
	    ret = (void *)tbldfldir;
	    break;

	case MMDBM:
	    ret = (void *)tbldbm;
	    break;

	case MMQUEDIR:
	    ret = (void *)quedfldir;
	    break;

	case MMCMDDIR:
	    ret = (void *)cmddfldir;
	    break;

	case MMCHANDIR:
	    ret = (void *)chndfldir;
	    break;

	case MMDLVRDIR:
	    ret = (void *)mldfldir;
	    break;

	case MMLCKDIR:
	    ret = (void *)lckdfldir;
	    break;

	case MMTEMPT:
	    ret = (void *)tquedir;
	    break;

	case MMADDRQ:
	    ret = (void *)aquedir;
	    break;

	case MMMSGQ:
	    ret = (void *)mquedir;
	    break;

	case MMQUEPROT:
	    ret = (void *) &queprot;
          *type = VARTYPE_OCT;
	    break;

	case AUTHREQUEST:
	    ret = (void *)authrequest;
	    break;

	case MMWARNTIME:
	    ret = (void *)&warntime;
    	    *type = VARTYPE_INT;
	    break;

	case MMFAILTIME:
	    ret = (void *)&failtime;
    	    *type = VARTYPE_INT;
	    break;

	case MMMAXSORT:
	    ret = (void *)&maxqueue;
    	    *type = VARTYPE_INT;
	    break;

	case MMSLEEP:
	    ret = (void *)&mailsleep;
    	    *type = VARTYPE_INT;
	    break;

	case MMSUBMIT:
	    ret = (void *)namsubmit;
        ret = (void *)pathsubmit;
	    break;

	case MMDELIVER:
	    ret = (void *)namdeliver;
        ret = (void *)pathdeliver;
	    break;

	case MMPICKUP:
	    ret = (void *)nampkup;
        ret = (void *)pathpkup;
	    break;

	case MMV6MAIL:
      ret = (void *)nammail;
      ret = (void *)pathmail;
      break;

        case MMMBXPROT:
          ret = (void *)&sentprotect;
          *type = VARTYPE_OCT;
          break;

	case MMMBXNAME:
	    ret = (void *)mldflfil;
	    break;

	case MMMBXPREF:
	    ret = (void *)delim1;
	    break;

	case MMMBXSUFF:
	    ret = (void *)delim2;
	    break;

	case MMDLVFILE:
	    ret = (void *)dlvfile;
	    break;

    	case MMAXHOPS:
    	    ret = (void *)&maxhops;
    	    *type = VARTYPE_INT;
    	    break;

    	case MADDID:
    	    ret = (void *)&mgt_addid;
    	    *type = VARTYPE_INT;
    	    break;

        case MADDIPADDR:
            ret = (void *)&mgt_addipaddr;
    	    *type = VARTYPE_INT;
            break;

        case MADDIPNAME:
            ret = (void *)&mgt_addipname;
    	    *type = VARTYPE_INT;
            break;

    	case MLISTSIZE:
	    ret = (void *)&lnk_listsize;
    	    *type = VARTYPE_INT;
            break;

	case UUname:
	    ret = (void *)Uuname;
	    break;

	case UUxstr:
	    ret = (void *)Uuxstr;
	    break;

	case MMDFLCHAN:
	    ret = (void *)ch_dflnam;
	    break;

	case MMMAILIDS:         /* enable/disable use of Mail-Ids */
	    ret = (void *)&mid_enable;
        *type = VARTYPE_INT;
	    break;

	case NIQUEDIR:
	    ret = (void *)pn_quedir;
	    break;

#ifdef HAVE_ESMTP
        case MMSGSIZELIMIT:
          ret = (void *)&message_size_limit;
          *type = VARTYPE_LONG;
          break;
#   ifdef HAVE_ESMTP_8BITMIME
        case M8BITMIME:
          ret = (void *)&accept_8bitmime;
          *type = VARTYPE_INT;
          break;
#   endif /* HAVE_ESMTP_8BITMIME */
#   ifdef HAVE_ESMTP_DSN
        case MDSN:
          ret = (void *)&dsn;
          *type = VARTYPE_INT;
          break;
#   endif /* HAVE_ESMTP_DSN */
#endif /* HAVE_ESMTP */
    }
  else
    *type = VARTYPE_NIL;

  *value = ret;
}
