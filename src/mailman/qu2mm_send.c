/*
 *     MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *     Copyright (C) 1979,1980,1981  University of Delaware
 *
 *     Department of Electrical Engineering
 *     University of Delaware
 *     Newark, Delaware  19711
 *
 *     Phone:  (302) 738-1163
 *
 *     This program module was developed as part of the University
 *     of Delaware's Multi-Channel Memo Distribution Facility (MMDF).
 *
 *     Acquisition, use, and distribution of this module and its listings
 *     are subject restricted to the terms of a license agreement.
 *     Documents describing systems using this module must cite its source.
 *
 *     The above statements must be retained with all copies of this
 *     program and may not be removed without the consent of the
 *     University of Delaware.
 *
 *
 *     version  -1    David H. Crocker    March   1979
 *     version   0    David H. Crocker    April   1980
 *     version  v7    David H. Crocker    May     1981
 *     version   1    David H. Crocker    October 1981
 *
 */
/*                SEND FROM DELIVER TO SUBMIT
 *
 *  Feb 83  Doug Kingston       Initial version of list processor
 *  Apr 86  Craig Partridge     Minor efficiency tweaking
 *  Dec 87  Dan Long            Converted to address blocking function
 */

#include "util.h"
#include "mmdf.h"
#include "phs.h"
#include "ch.h"
#include "ap.h"
#include <pwd.h>
#include <grp.h>
#include "ml_send.h"
#include "mm_io.h"

extern struct ll_struct   *logptr;
extern Chan *chanptr;
extern long qu_msglen;
extern char *supportaddr;
extern char *mmdfgroup;

extern char *multcat();
extern char *blt();

LOCFUN qu2mm_each ();            /* send one copy of text per message */
LOCFUN mm_verify();
#ifdef __STDC__
LOCFUN qu2mm_txtcpy (RP_Buf *rp); /* copy the text of the message       */
LOCFUN mm_master(char *adrp);    /* pass adr to submit */
#else /* __STDC__ */
LOCFUN qu2mm_txtcpy ();         /* copy the text of the message       */
LOCFUN mm_master();    /* pass adr to submit */
#endif /* __STDC__ */

extern int ml_state;              /* to fake ml_send(3) out */

char adr[ADDRSIZE];
char lo_info[2 * LINESIZE];
char lo_sender[2 * LINESIZE];
char	lo_adr[2 * LINESIZE];
char lo_replyto[2 * LINESIZE]; /* don't forget to change strncpy() cmd in lo_wtmail.c */
char	lo_size[16];
char	*lo_parmv[15];
int     lo_parmc;
struct passwd *lo_pw;


  LOCVAR struct rp_construct
#if 0
  rp_bdrem =
{
    RP_BHST, 'B', 'a', 'd', ' ', 'r', 'e', 's', 'p', 'o', 'n', 's', 'e',
    ' ', 'f', 'r', 'o', 'm', ' ', 's', 'u', 'b', 'm', 'i', 't', '\0'
},
	rp_adr =
{
    RP_AOK, 'a', 'd', 'd', 'r', 'e', 's', 's', ' ', 'o', 'k', '\0'
},
	rp_gdtxt =
{
    RP_MOK, 't', 'e', 'x', 't', ' ', 's', 'e', 'n', 't', ' ', 'o', 'k', '\0'
},
	rp_noop =
{
    RP_NOOP, 's', 'u', 'b', '-', 'l', 'i', 's', 't', ' ', 'n', 'o', 't', ' ',
    's', 'p', 'e', 'c', 'i', 'a', 'l', '\0'
},
	rp_vhost =
{
    RP_USER, 'H', 'o', 's', 't', 'n', 'a', 'm', 'e', ' ', 'n', 'o', ' ', 'l',
    'o', 'n', 'g', 'e', 'r', ' ', 'v', 'a', 'l', 'i', 'd', '\0'
},
	rp_vuser =
{
    RP_USER, 'U', 's', 'e', 'r', 'n', 'a', 'm', 'e', ' ', 'n', 'o', ' ', 'l',
    'o', 'n', 'g', 'e', 'r', ' ', 'v', 'a', 'l', 'i', 'd', '\0'
},
#endif
  rp_aend  =
{
	RP_OK, 'l', 'o', 'c', ' ', 'e', 'n', 'd', ' ', 'o', 'f', ' ', 'a',
	'd', 'd', 'r', ' ', 'l', 'i', 's', 't', '\0'
},
  rp_hend  =
{
	RP_NOOP, 'e', 'n', 'd', ' ', 'o', 'f', ' ', 'h', 'o', 's', 't', ' ',
	'i', 'g', 'n', 'o', 'r', 'e', 'd', '\0'
};

/**/

qu2mm_send ()             /* overall mngmt for batch of msgs    */
{
  short       result;

#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "qu2mm_send ()");
#endif

  if (rp_isbad (result = qu_pkinit ()))
	return (result);
    
  /*
   *  While there are messages to process ...
   */
  for(;;){                /* get initial info for new message   */
    result = qu_rinit (lo_info, lo_sender, chanptr -> ch_apout);
    if(rp_gval(result) == RP_NS){
      qu_rend();
      continue;
    }
    else {
      if(rp_gval(result) != RP_OK)
        break;
    }
    phs_note (chanptr, PHS_WRSTRT);

    snprintf (lo_size, sizeof(lo_size), "%ld", qu_msglen);
    result = qu2mm_each ();
    qu_rend();
    if (rp_isbad (result))
      return (result);
  }

  if (rp_gval (result) != RP_DONE)
  {
    ll_log (logptr, LLOGTMP, "not DONE (%s)", rp_valstr (result));
    return (RP_RPLY);         /* catch protocol errors      */
  }

  qu_pkend ();                  /* done getting messages          */
  phs_note (chanptr, PHS_WREND);

  return (result);
}
/**/

LOCFUN
	qu2mm_each ()            /* send one copy of text per message */
{
  struct rp_bufstruct thereply;
  RP_Buf  replyval;
  short   result;
  int     len;
  char    host[ADDRSIZE];
  char    info[LINESIZE];
  char    *adrp;
  char    *colonp,*commap,*atp;

#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "qu2mm_each()");
#endif

  /*
   *  For every address in the message ...
   */
  FOREVER
  {
    result = qu_radr (host, adr);
	if (rp_isbad (result))
      return (result);      /* get address from Deliver           */

    if (rp_gval (result) == RP_HOK)
    {                       /* no-op the sub-list indication */
      qu_wrply ((RP_Buf *) &rp_hend, rp_conlen (rp_hend));
      continue;
    }
 
    if (rp_gval (result) == RP_DONE)
    {
      qu_wrply ((RP_Buf *) &rp_aend, rp_conlen (rp_aend));
      return (RP_OK);       /* end of address list */
    }

    
    /* Build notification string for sender */
    if ((colonp = strchr(adr, ':')) == (char *) 0) {
      /* username[@host] */
      adrp = adr;
      if ((atp = strrchr (adrp, '@')) != (char *) 0)
        *atp = '\0';   /* strip hostname */         
      if ((tb_k2val(chanptr->ch_table, 1, adrp, lo_adr) != OK) ||
          (strlen(lo_adr) == 0))    
        fprintf(stderr, "'%s', not Found - 1\n", adrp);
      else
        fprintf(stderr, "'%s' Found - 1 (%s)\n", adrp, lo_adr);
      /* avoid mail loops */
      adrp = multcat("~", adr, (char *) 0);
    } else {
      /* @channel[,route[,route]]:username[@host] */
      if ((commap=strchr(adr, ',')) == (char *) 0) {
        adrp=colonp+1;
        /* username[@host] */
        if (((atp = strrchr (adrp, '@')) == (char *) 0) ||
            (tb_k2val(chanptr->ch_table, 1, atp+1, lo_adr) != OK) ||
            (strlen(lo_adr) == 0))
          fprintf(stderr, "'%s', not Found - 2\n", adrp);
        else
          fprintf(stderr, "'%s' Found - 2 (%s)\n", adrp, lo_adr);
      } else {
        adrp = commap+1;
    		        /* @route[,@route]:username[@host] */
        if (((atp = strrchr (colonp+1, '@')) == (char *) 0) ||
            (tb_k2val(chanptr->ch_table, 1, atp+1, lo_adr) != OK) ||
            (strlen(lo_adr) == 0)) {
          fprintf(stderr, "'%s', not Found - 3\n", adrp);
          *colonp = '\0';
          atp = strrchr(adrp,'@'); /* start of last route */
          *colonp = ':';
        } else
          fprintf(stderr, "'%s' Found - 3 (%s)\n", adrp, lo_adr);
      }

    }
    ll_log(logptr, LLOGFST, "%s to %s for %s",
           "Warning", lo_sender, adrp);

    replyval.rp_val = mm_verify ();
    if (rp_isbad (replyval.rp_val)) {
      qu_wrply (&replyval, (sizeof replyval.rp_val));
      continue;                       /* go for more! */
    }

    replyval.rp_val = mm_master(adrp);
    switch (rp_gval (replyval.rp_val)) {
        case RP_LIO:
        case RP_TIME:
          /* RP_AGN prevents deliver from calling us dead */
          replyval.rp_val = RP_AGN;
        case RP_LOCK:
        case RP_USER:         /* typicial valid responses */
        case RP_BHST:
        case RP_NOOP:
        case RP_NO:
        case RP_MOK:
        case RP_AOK:
        case RP_OK:
          break;
        default:              /* not expected so may abort          */
          if (rp_isgood (replyval.rp_val))
          {
            ll_log (logptr, LLOGFAT,
                    "lo_wadr = %s", rp_valstr (replyval.rp_val));
            replyval.rp_val = RP_RPLY;  /* Conservative */
          }
    }
    qu_wrply (&replyval, (sizeof replyval.rp_val));
  }
    /* NOTREACHED */
}

/**/
const char *VALID_COMMANDS[] = {
        "post", 
        "mailcmd",
        "mailowner",
        NULL                                 /* Sentinel, don't remove */
};

LOCFUN int check_command(command)
char *command;
{
  int i = 0;

  while (VALID_COMMANDS[i] != NULL) {
    if (!strcmp(command, VALID_COMMANDS[i]))
      return 1;
    i++;
  }
  return 0;
}

LOCFUN mm_verify () /* send one address spec to local     */
{
  int	argc;
  char    mailid[MAILIDSIZ];      /* mailid of recipient */
  register char   *p;

#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "mm_verify()");
#endif

  lo_parmc = sstr2arg(lo_adr, 15, lo_parmv, " \t");	/* split it */
  if( lo_parmc < 2 ) { /* not good enough! */
    ll_log (logptr, LLOGFST, "to few parameters");
    return RP_PARM;
  }
    
  if (!check_command(lo_parmv[0])) {
    ll_log (logptr, LLOGFST, "Illegal command: %s", lo_parmv[0]);
    return RP_PARM;
  }
    
  return (RP_OK);          /* Invalid addresss or real problems */
}


LOCFUN mm_master(adrp)           /* pass adr to submit */
char *adrp;
{
  int   childid;                /* child does the real work           */
  struct group *grp;

#ifdef DEBUG
  ll_log(logptr, LLOGFST, "mm_master()\n");
#endif
  
        lo_pw = getpwnam("mailman");
  switch (childid = fork ()) {
      case NOTOK:
		return (RP_LIO);        /* problem with system */

      case OK:
		ll_close (logptr);      /* since the process is splitting */

        /* set to mmdf's group-id to be able write to the mail-box directory */
        if( (grp = getgrnam(mmdfgroup)) == NULL) {
          ll_err (logptr, LLOGTMP, "Cannot find mmdfgroup (%s)", mmdfgroup);
          err_abrt(RP_BHST);
        }
        
        /* we want to run with lo_pw user-id and mmdf's group id */
        if(
#ifdef HAVE_INITGROUPS
          initgroups (lo_pw->pw_name, lo_pw->pw_gid) == NOTOK
            || setgid (lo_pw->pw_gid) == NOTOK
#else /* HAVE_INITGROUPS */
          setgid (grp->gr_gid) == NOTOK
#endif /* HAVE_INITGROUPS */
          || setuid (lo_pw->pw_uid) == NOTOK) {
          ll_err (logptr, LLOGTMP, "can't set id's (%d,%d)",
                  lo_pw->pw_uid, grp->gr_gid);
          exit (RP_BHST);
        }

        if (chdir (lo_pw->pw_dir) == NOTOK) {
          /* move out of MMDF queue space       */
          ll_err (logptr, LLOGTMP, "can't chdir to '%s'",
                  lo_pw->pw_dir);
          exit (RP_LIO);
        }

        exit (mm_slave());

        default:                  /* parent just waits  */
          return (pgmwait (childid));
  }
  /* NOTREACHED */
}

