/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_quit.c,v 5.23 1993/05/27 19:53:30 bostic Exp $ (Berkeley) $Date: 1993/05/27 19:53:30 $";
#endif /* not lint */

#include <sys/types.h>

#include "vi.h"
#include "excmd.h"

int
ex_quit(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	int force;

	force = F_ISSET(cmdp, E_FORCE);

	/* Historic practice: quit! doesn't do autowrite. */
	if (!force)
		MODIFY_CHECK(sp, ep, 0);

	/*
	 * Historic practice: quit! doesn't check for other files.
	 * Also check for related screens; if they exist, quit.
	 */
	if (!force && ep->refcnt <= 1 && file_next(sp, ep, 0)) {
		msgq(sp, M_ERR,
	"More files; use \":n\" to go to the next file, \":q!\" to quit.");
		return (1);
	}

	F_SET(sp, force ? S_EXIT_FORCE : S_EXIT);
	return (0);
}
