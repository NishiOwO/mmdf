/*
 * $Id: funcs.h,v 1.4 2000/08/08 20:38:02 krueger Exp $
 */

#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#endif


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

/* util/ll_log.c * /
extern int ll_open ();
extern int ll_close();
extern int ll_hdinit();
extern int ll_init();
extern int ll_log(); */

/* err_abrt.c */
/* extern void err_abrt (); */
/* cmdbsrch.c */
extern int cmdbsrch ();
/* util/tai_packages.c */
extern int tai_pgm ();
extern int tai_log ();

/* util/gcread.c */
extern int gcread ();
extern int qread ();

