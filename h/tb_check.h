/*
 * $Id: tb_check.h,v 1.1 2000/08/08 16:36:39 krueger Exp $
 */

#ifndef TB_CHECK_H
#define TB_CHECK_H

/*  various verbosity levels  */
#define         PERROR             01   /*  perror() should be called */
#define         ANYLEVEL          070   /*  any bit in this field */
#define         LEVEL1            010   /*  fatal */
#define         LEVEL1P      LEVEL1|PERROR
#define         LEVEL3            020   /*  major section */
#define         LEVEL4            030   /*  sub-section */
#define         LEVEL5            040
#define         LEVEL6            050
#define         LEVEL6_5          060   /*  warning [..] messages */
#define         LEVEL7            070   /* nitty gritty junk */
#define         LEVEL0       LEVEL1|PERROR

#define         BACKGROUND      LEVEL6  /* default verbosity */
#define         FINAL           0
#define         PARTIAL         1

extern void qflush ();
extern void que();
extern void format();
extern int  chkfile ();
extern int  chkpath ();
extern char *xerrstr();

#endif /* TB_CHECK_H */
