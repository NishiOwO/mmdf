/* $Header: /tmp/cvsroot_mmdf/mmdf/devsrc/lib/addr/parse.c,v 1.6 2000/07/06 17:43:42 krueger Exp $ */
/* $Log: parse.c,v $
/* Revision 1.6  2000/07/06 17:43:42  krueger
/* some minor compiler fixes
/*
 * Revision 1.6  2000/07/06 17:28:00  krueger
 * some minor compiler fixes
 *
/* Revision 1.5  1999/08/12 13:15:37  krueger
/* Added patch 2.44b3 and 2.44b4
/*
/* Revision 1.4.2.1  1999/08/04 12:16:00  krueger
/*
/*  	Makefile.in addr/ap_1adr.c addr/ap_lxtable.c addr/parse.c
/*  	mmdf/ml_send.c mmdf/mm_tai.c mmdf/qu_io.c mmdf/rtn_proc.c
/*  	table/tb_ns.c util/cnvtdate.c util/ll_log.c
/* 	* Changes by Brad Allen <Ulmo@Q.Net>
/* 	* Bug fixes (Various)
/* 	* Debugging Code (Scattered hunting)
/* 	* Local Modifications (Must determine what this is)
/* 	* Architecture Modifications (Linux/GLIBC/FHS)
/* 	* General Modifications (Enhancements)
/* 	* Stupid Modifications (Bug creation)
/*
 * Revision 1.4  1998/03/17 19:35:20  krueger
 * Added changes to disable source-route for sender in smtpsrvr
 *
 * Revision 1.3  1985/01/17 23:07:11  dpk
 * Reworked, it now works
 *
 * Revision 1.3  85/01/17  23:07:11  dpk
 * Reworked, it now works
 * 
 * Revision 1.2  83/11/18  17:09:41  reilly
 * Steve Kille's version
 *
 *  < 1978  B. Borden       Wrote initial version of parser code
 *  78-80   D. Crocker      Reworked parser into current form
 */
#include "config.h"
#if DEBUG < 2
main() {
	fprintf(stderr, "parse is useless when compiled with -DDEBUG=x (x < 2)\n");
}
#else
#include "ap_lex.h"
#include "util.h"
#include "mmdf.h"
#include "ap.h"

extern char ap_llex;
extern char *locname;
AP_ptr ap_fullparse ();

extern LLog *logptr;


extern char *namtab[],
	   *typtab[];
extern int debug;

main (argc, argv)
{
    AP_ptr ap_fp;
    int	getach();

    mmdf_init ("PARSE");
    logptr -> ll_level = LLOGFTR;
    if (argc == 2)
	debug++;

    for (;;)
    {
	printf ("Parse: ");

	switch ((int)(ap_fp = ap_fullparse (getach))) {
	case NOTOK:
	    printf ("\nNOTOK: on %s\n", namtab[ap_llex]);
	    break;
	case DONE:
	    printf ("\nNOTOK: on %s\n", namtab[ap_llex]);
	    break;
	case OK:
	    printf ("\nOK?: on %s\n", namtab[ap_llex]);
	    break;
	default:
	    printf ("\n\nAccept\n");
	    pretty (ap_fp);
	    printf ("\n");
	    while (ap_sqdelete (ap_fp, (AP_ptr)0) != 0);
	    ap_free (ap_fp);  /* delete full string         */
	    ap_fp = (AP_ptr) 0;
	    continue;
	}
    	break;
    }
    exit (0);
}

getach ()
{
    int	c;

    c = getchar();
    if (c == '\n')
	return (0);
    if (c == EOF)
	exit(0);
    return (c);
}

void pretty (ap)
register AP_ptr ap;
{
    register int    depth = 1;

    do {
	switch (ap -> ap_obtype)
	{
	    case APV_EGRP: 
	    case APV_EPER: 
		depth -= 2;
	}

	printf ("%.*d%-9s", depth, "          ");
    	if (ap -> ap_obtype >= 0 && ap -> ap_obtype <= 13)
		printf("%s %s", typtab[(int)(ap -> ap_obtype)],
			ap -> ap_obvalue ? ap -> ap_obvalue : "NIL");
    	else
		printf("BOGUS!(%d) %s", ap -> ap_obtype,
			ap -> ap_obvalue ? ap -> ap_obvalue : "NIL");
    	switch (ap -> ap_ptrtype) {
	case APP_NIL:
		printf("\t(NIL)\n");
    		break;
	case APP_NXT:
		printf("\t(NXT)\n\n");
    		break;
	case APP_ETC:
		printf("\t(ETC)\n", ap -> ap_ptrtype);
    		break;
    	default:
		printf("\t(BOGUS!)\n", ap -> ap_ptrtype);
    	}
	switch (ap -> ap_obtype)
	{
	    case APV_NGRP: 
	    case APV_NPER: 
		depth += 2;
	}
    } while ((ap = ap -> ap_chain) != (AP_ptr)0);
    printf ("End on null pointer\n");
}
#endif
