/*
 * $Id: mm_io.h,v 1.1 1998/05/25 20:11:09 krueger Exp $
 */

extern int mm_init ();
extern int mm_end ();
extern int mm_sbinit ();
extern int mm_sbend();
extern int mm_pkinit ();
extern int mm_pkend ();
extern int mm_rrply ();
extern int mm_wrply ();
extern int mm_rrec ();
extern int mm_rstm ();
extern int mm_wrec ();
extern int mm_wstm ();

extern int mm_winit ();
extern int mm_wadr ();
extern int mm_waend ();
extern int mm_wtxt ();
extern int mm_wtend ();
