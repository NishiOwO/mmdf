/*
 * $Id: funcs.h,v 1.2 1998/05/25 11:43:46 mmdf Exp $
 */

#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#endif

/* addr/adrparse.c */
extern void parsadr();

/* addr/ap_1adr.c */
extern int ap_1adr ();
/* LOCFUN void ap_7to8 (); */

/* addr/ap_dmflip.c */
extern char *ap_dmflip();

/* addr/ap_lex.c */
extern int ap_lex();
extern int ap_char();

/* addr/ap_lxtable.c */

/* addr/ap_normali.c */
/* extern AP_ptr ap_normalize(); */
/* LOCFUN void	ap_ptinit(); */
#ifndef	JNTMAIL
extern void ap_locnormalize();
#endif
extern int ap_dmnormalize();
/* LOCFUN void logtree(); */

/* functions from addr/ap_p2s.c */
extern char *ap_p2s();

/* functions from addr/ap_util.c */
extern void ap_free();
extern void ap_fllnode();
extern void ap_ninit();
extern void ap_insert();
extern void ap_delete();
extern void ap_sqtfix ();
extern void ap_iinit ();
extern void ap_clear ();
extern int  ap_ppush ();
extern void ap_ppop ();
extern int  ap_flget ();
extern int  ap_fpush ();
extern void ap_fpop ();
extern void ap_palloc ();
extern void ap_pfill ();
extern void ap_pnsrt ();
extern void ap_pappend ();
extern void ap_padd ();

/**************************************************/
/* mmdf/alias.c */
extern void aliasinit();
extern void aliasmap();
extern int get_key();
extern int m_next_address();

/* mmdf/aliasfetch.c */
extern int aliasfetch();

/* mmdf/amp_hdr.c */
extern long amp_hdr();
extern int getaitm ();
extern void pretty ();
extern void out_init();
extern int out_adr();

/* mmdf/ch_llinit.c */
extern void ch_llinit();

/* mmdf/ca_add.c */
extern int ca_add ();
extern int ca_find ();

/* mmdf/mm_tai.c */
extern int mm_tai();
extern int tb_tai();
extern int dm_tai();
extern int al_tai ();
extern int ch_tai();
extern int tai_llev();

/* mmdf/post_tai.c */
extern int post_tai ();

/* mmdf/uip_tai.c */
extern int uip_tai ();

/* mmdf/mmdf_init.c */
void tai_error ();
void mmdf_init ();

/* lexequ.c */
extern int lexequ();

/* tb_chdbm.c */
extern int tb_k2val();

/* util/cstr2arg.c */
extern int cstr2arg ();

/* util/strindex.c */
extern int strindex ();

/* util/ll_err.c */
extern int ll_err();

/* util/ll_log.c */
extern int ll_open ();
extern int ll_close();
extern int ll_hdinit();
extern int ll_init();
extern int ll_log();

/* err_abrt.c */
/* extern void err_abrt (); */
/* cmdbsrch.c */
extern int cmdbsrch ();
/* util/tai_packages.c */
extern int tai_pgm ();
extern int tai_log ();
