/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1991, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "$Id: ex_version.c,v 10.25 1996/04/12 10:48:11 bostic Exp $ (Berkeley) $Date: 1996/04/12 10:48:11 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <bitstring.h>
#include <limits.h>
#include <stdio.h>

#include "../common/common.h"
#include "version.h"

/*
 * ex_version -- :version
 *	Display the program version.
 *
 * PUBLIC: int ex_version __P((SCR *, EXCMD *));
 */
int
ex_version(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	(void)ex_printf(sp, "%s\n", VI_VERSION);
	return (0);
}
