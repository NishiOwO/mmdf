/***********************************************************************/
/** 
 *
 * $Id: mmdf_vtinit.c,v 1.2 2002/10/12 17:54:01 krueger Exp $
 *
 * (C) 2001 Kai Krueger
 * Email: krueger@mathematik.uni-kl.de
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#ifndef lint
  static char rcsid[]="@(#) $Revision: 1.2 $ $Date: 2002/10/12 17:54:01 $";
  static char copyright[] =
"@(#)Copyright (c) 2001 Kai Krueger\n";
#endif /* lint */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * HISTORY
 * $Log: mmdf_vtinit.c,v $
 * Revision 1.2  2002/10/12 17:54:01  krueger
 * *** empty log message ***
 *
 * Revision 1.1  2001/04/26 22:02:13  krueger
 * Added support for virtual hosts
 *
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Features test switches
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * system headers
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#include "util.h"
#include "mmdf.h"
#include "cmd.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * local headers
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#include "pathnames.h"
#ifdef HAVE_VIRTUAL_DOMAINS

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Macros
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#define MAXARG 100

#define VIRTUALENVNAME "MMDFVIRTUALNAME"
#define MMNOOP         100

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * File scope variables
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
char *mmvirtual_tailor = MMDFCFGDIR"/mmdfvirtual";


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * External variables
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
extern LLog *logptr;
extern char *mmtailor;         /* where the tailoring file is          */
extern char *logdfldir;


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * External functions
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
char *mmdf_parse_args(argc, argv)
int argc;
char *argv[];

{
  int c;
  int this_option_optind = optind ? optind : 1;
  int option_index = 0;
  static struct option long_options[] = {
    {"virual", 1, 0, 0},
    {0, 0, 0, 0}
  };
  return argv[0];
  printf("start c=%d, argc=%d, idx=%d\n", c, argc, option_index);
  while (1) {
    c = getopt_long(argc, argv, "abc:d:0123456789",
                    long_options, &option_index);
    printf("c=%d, argc=%d, idx=%d\n", c, argc, option_index);
    
    if (c == EOF)
      break;
    switch (c) {
        case 0:
          printf("option %s", long_options[option_index].name);
          if (optarg)
            printf(" with arg %s", optarg);
          printf("\n");
          break;
        case '?':
          break;

        default:
          printf("?? getopt returned character code 0%o %c ??\n", c, c);
    }
  }

  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }
  printf("Ende of mmdf_parse_args()\n");
  
  return argv[0];
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Structures and unions
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
struct mm_virt {
  char *vt_name;              /* name of virtual domain */
  char *vt_mmtailor;          /* path to mmdftailor     */
  char vt_restricted;         /* run smtpsrvr restricted */
  int  vt_max_connections;    /* maximum connections allowed */
  char *vt_if_name;           /* interface valid for that virtualdomain */
  char *vt_if_ip;           /* interface valid for that virtualdomain */

//  struct sockaddr_in vt_addr; /* socket to bind on (addr of vt_if_name) */
};
typedef struct mm_virt MMvirt;
char *vt_name;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Internal functions (declarations)
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int mmdf_init_virtual();
LOCFUN int virt_tai ();
MMvirt *vt_nm2struct ();


LOCVAR MMvirt *vt_doms[NUMDOMAINS];
LOCVAR MMvirt **vt_list = vt_doms;
LOCVAR int vt_nument;
LOCVAR char mmdfvirtual_loaded = 0;
LOCVAR char *virtual = 0;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * 
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
mmdf_init_mmtailor(virtual_name)
char *virtual_name;
{
  MMvirt *vtptr;
  char buf[LINESIZE];

  virtual = getenv(VIRTUALENVNAME);
#ifdef DEBUG
  ll_log (logptr, LLOGPTR, "mmdf_init_mmtailor (%s, %s)\n", virtual_name, virtual);
#endif
  mmdf_init_virtual();

  if(virtual_name == NULL) {
    if(virtual == NULL) /* use default */
      return OK;
  } else {
    if(!strcmp(virtual_name, "")) {
      if(virtual != NULL) unsetenv(VIRTUALENVNAME);
      virtual = (char *)0;
      return OK;
    }
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%s=%s", VIRTUALENVNAME, virtual_name);
    putenv(buf);
    virtual = virtual_name;
  }
  
  ll_log (logptr, LLOGPTR, "mmdf_init_mmtailor search %s\n", virtual);

  vtptr = vt_nm2struct(virtual);
  if(vtptr != (MMvirt *)NOTOK) mmtailor = vtptr->vt_mmtailor;
  
  return OK;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * mmdf_init_virtual()
 * read the tailor file 'mmdfvirtual' and search for the tailorfile
 * for a given virtual domain.
 * If the parameter virtual_name is not given search for a given
 * environement with the name of the virtual domain.
 * use that variable to store the virtual domainname for all other
 * programs so we will not need to add a new parameter all programs at
 * starttime.
 * the parameter virtual_name overwrites MMDFVIRTUALNAME
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int mmdf_init_virtual()
{
  char *argv[MAXARG];
  int  argc;

#ifdef DEBUG
  ll_log (logptr, LLOGPTR, "mmdf_vtinit ()\n");
#endif
  
  if(mmdfvirtual_loaded) return OK;
  /* if don't have the virtual-configuration, don't care about it */
  if(tai_init(mmvirtual_tailor) != OK) {
    return NOTOK;
  }

  vt_nument = 0;
  while ((argc = tai_get (MAXARG, argv)) > 0) {
	if (argv[0] == 0 || argv[0][0] == '\0') continue;  /* noop  */

    if (lexequ ("mmvirt", argv[0])) {
      switch(virt_tai(argc-1, argv+1)) { /* virutal domain tailoring */
          case YES:
          case NOTOK:
            continue;
      }
    }
    
    /* fake garbage collection of unused external tailor info  */
    post_tai (argc, argv);
  }
  
  if (argc == NOTOK)
	err_abrt (RP_MECH, "Error processing tailoring file '%s'",
              mmvirtual_tailor);

  mmdfvirtual_loaded = 1;
  tai_end (0); 
  return OK;
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * tailoring for virtual domains/hosts
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#define CMDNAME    1
#define CMDFILE    2
#define CMDCONN    3
#define CMDIF      4
#define CMDFLAGS   5
#define CMDNOOP    6

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",         CMDNOOP,   0},
    {"conn",     CMDCONN,   1},  /* max connection         */
    {"file",	 CMDFILE,   1},  /* tailorfile             */
    {"flags",	 CMDFLAGS,  1},  /* restricted ? ( r-flag) */
    {"if",       CMDIF,     1},  /* interface to bind      */
    {"name",     CMDNAME,   1},  /* name of virtual domain */
    {0,          0,         0}
};

#define CMDBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)


LOCFUN int virt_tai (argc, argv)
int argc;
char *argv[];
{
  int ind;
  int type;
  struct mm_virt *tbptr;
  
#ifdef DEBUG
  ll_log (logptr, LLOGFTR, "virt_tai (%s: %d)", argv[0], argc);
#endif
  //printf("virt_tai (%s: %d)\n", argv[0], argc);
 
  if (vt_nument >= NUMDOMAINS) {
	tai_error ("exceed table size", argv[0], argc, argv);
	return (NOTOK);
  }
  vt_doms[vt_nument++]=tbptr = (MMvirt *)malloc ((unsigned) (sizeof (MMvirt)));
  vt_doms[vt_nument] = (MMvirt *)0;
  memset(tbptr, 0, (unsigned) (sizeof (MMvirt)));
  tbptr->vt_name = NULL;
  tbptr->vt_mmtailor = NULL;

  /* parse now the other arguments */
  for (ind = 0; ind < argc; ind++)
  {
	if (!lexequ (argv[ind], "=")) {
      if (ind == 0) {
        /* ok to default first field only */
#ifdef DEBUG
		ll_log (logptr, LLOGBTR, "defaulting base '%s'", argv[ind]);
#endif
		//printf("defaulting base '%s'\n", argv[ind]);
      } else {
		tai_error ("unassignable, bare data field",
                   argv[ind], argc, argv);
        //printf("unassignable, bare data field '%s', %d\n",argv[ind], argc, argv);
		continue;
      }
	} else {
      ind += 2;
#ifdef DEBUG
      ll_log (logptr, LLOGFTR, "tb '%s' = '%s' (%d)", argv[ind - 1],
              argv[ind], argc - ind);
#endif
      switch (cmdbsrch (argv[ind - 1], argc - ind, cmdtbl, CMDBENT))
      {
          case CMDNAME:  /* name of virtual domain */
            tbptr -> vt_name = argv[ind];
            break;

          case CMDFILE:  /* path to mmdftailor file */
            tbptr -> vt_mmtailor = argv[ind];
            break;

          case CMDIF:    /* interface to bind */
            tbptr -> vt_if_name = argv[ind];
            break;

          case CMDCONN:  /* max number of connections */
            tbptr -> vt_max_connections =  atoi(argv[ind]);
            break;

          case CMDFLAGS: /* restricted flag */
            tbptr -> vt_restricted =  atoi(argv[ind]);
            break;
            
          case CMDNOOP:
            break;       /* noop */

          default:
            tai_error ("unknown table parm", argv[ind], argc, argv);
            break;
      }
	}
  }

#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "virt_tai (ret=YES)");
  ll_log (logptr, LLOGFTR, "name=%s, chn=%s ",
          tbptr -> vt_name, tbptr -> vt_mmtailor);
#endif

    return (YES);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * give pointer to structure for virtual-domain
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
MMvirt  *vt_nm2struct (name)
register char  *name;		  /* string name of virtual  */
{
  register MMvirt **vtptr;

  for (vtptr = vt_list; *vtptr != (MMvirt *) 0; vtptr++) {
	if (lexequ (name, (*vtptr) -> vt_name))
      return (*vtptr);    /* return addr of virtual struct entry   */
  }
  
    return ((MMvirt *) NOTOK);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * give pointer to structure for virtual-domain
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
char *vt_hst2vtn (name, ip, stricked)
register char  *name;		  /* string name of virtual  */
register char *ip;
int *stricked;
{
  register MMvirt **vtptr;

  for (vtptr = vt_list; *vtptr != (MMvirt *) 0; vtptr++) {
	if ( (lexequ (name, (*vtptr) -> vt_if_name)) ||
          (lexequ (ip, (*vtptr) -> vt_if_ip))) {
      *stricked = (*vtptr)->vt_restricted;
      return ((*vtptr)->vt_name); /* return name of virtual struct entry */
	}
  }
  
  return (char*)0;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * give pointer to structure for virtual-domain
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int vt_localip(addr)
struct sockaddr_in *addr;
{
  MMvirt  *vtptr;
  struct hostent *hp;
 
  if(virtual==(char *)0) return 1;
  vtptr = vt_nm2struct (virtual);
  
  hp = gethostbyname(vtptr->vt_if_name);
  if(hp == NULL) return 1;

  memcpy(&(addr->sin_addr), hp->h_addr, sizeof(struct in_addr));
  return 0;
}

#else /* HAVE_VIRTUAL_DOMAINS */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * External functions
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
char *mmdf_parse_args(argc, argv)
int argc;
char *argv[];

{
  return argv;
}

#endif /* HAVE_VIRTUAL_DOMAINS */
