/*
 *  L O C A L . H 
 *
 *  EE/CIS Computer Lab
 *  Department of Computer and Information Sciences
 *  Department of Electrical Engineering
 *  University of Delaware
 *
 *  REVISION HISTORY:
 *
 *  $Revision: 1.4 $
 *
 *  $Log: local.h,v $
 *  Revision 1.4  1998/10/07 13:13:44  krueger
 *  Added changes from v44a8 to v44a9
 *
 *  Revision 1.3.2.1  1998/10/06 14:21:08  krueger
 *  first cleanup, is now compiling and running under linux
 *
 *  Revision 1.3  1985/11/16 15:54:31  galvin
 *  Added include of v7.local.h if 4_2BSD is defined.
 *
 * Revision 1.3  85/11/16  15:54:31  galvin
 * Added include of v7.local.h if 4_2BSD is defined.
 * 
 * Revision 1.2  85/11/16  15:53:32  galvin
 * Added comment header for revision history.
 * 
 *
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)local.h	5.1 (Berkeley) 6/6/85
 */

#include "./v7.local.h"

#ifdef V4_1BSD
#include "./v7.local.h"
#endif

#ifdef V7
#include "./v7.local.h"
#endif

#ifdef CORY
#include "./c.local.h"
#endif

#ifdef INGRES
#include "./ing.local.h"
#endif

#ifdef V6
#include "./v6.local.h"
#endif

#ifdef CC
#include "./cc.local.h"
#endif

#ifdef V40
#include "./40.local.h"
#endif
