/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994, 1995
 *	Keith Bostic.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_util.c,v 10.4 1995/06/09 12:52:36 bostic Exp $ (Berkeley) $Date: 1995/06/09 12:52:36 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "compat.h"
#include <db.h>
#include <regex.h>

#include "common.h"
#include "vi.h"

/*
 * v_eof --
 *	Vi end-of-file error.
 *
 * PUBLIC: void v_eof __P((SCR *, MARK *));
 */
void
v_eof(sp, mp)
	SCR *sp;
	MARK *mp;
{
	recno_t lno;

	if (mp == NULL)
		v_message(sp, NULL, VIM_EOF);
	else {
		if (file_lline(sp, &lno))
			return;
		if (mp->lno >= lno)
			v_message(sp, NULL, VIM_EOF);
		else
			msgq(sp, M_BERR, "195|Movement past the end-of-file");
	}
}

/*
 * v_eol --
 *	Vi end-of-line error.
 *
 * PUBLIC: void v_eol __P((SCR *, MARK *));
 */
void
v_eol(sp, mp)
	SCR *sp;
	MARK *mp;
{
	size_t len;

	if (mp == NULL)
		v_message(sp, NULL, VIM_EOL);
	else {
		if (file_gline(sp, mp->lno, &len) == NULL) {
			FILE_LERR(sp, mp->lno);
			return;
		}
		if (mp->cno == len - 1)
			v_message(sp, NULL, VIM_EOL);
		else
			msgq(sp, M_BERR, "196|Movement past the end-of-line");
	}
}

/*
 * v_nomove --
 *	Vi no cursor movement error.
 *
 * PUBLIC: void v_nomove __P((SCR *));
 */
void
v_nomove(sp)
	SCR *sp;
{
	msgq(sp, M_BERR, "197|No cursor movement made");
}

/*
 * v_sof --
 *	Vi start-of-file error.
 *
 * PUBLIC: void v_sof __P((SCR *, MARK *));
 */
void
v_sof(sp, mp)
	SCR *sp;
	MARK *mp;
{
	if (mp == NULL || mp->lno == 1)
		msgq(sp, M_BERR, "198|Already at the beginning of the file");
	else
		msgq(sp, M_BERR, "199|Movement past the beginning of the file");
}

/*
 * v_sol --
 *	Vi start-of-line error.
 *
 * PUBLIC: void v_sol __P((SCR *));
 */
void
v_sol(sp)
	SCR *sp;
{
	msgq(sp, M_BERR, "200|Already in the first column");
}

/*
 * v_isempty --
 *	Return if the line contains nothing but white-space characters.
 *
 * PUBLIC: int v_isempty __P((char *, size_t));
 */
int
v_isempty(p, len)
	char *p;
	size_t len;
{
	for (; len--; ++p)
		if (!isblank(*p))
			return (0);
	return (1);
}

/*
 * v_message --
 *	Display a few common messages.
 *
 * PUBLIC: void v_message __P((SCR *, char *, vim_t));
 */
void
v_message(sp, p, which)
	SCR *sp;
	char *p;
	vim_t which;
{
	switch (which) {
	case VIM_COMBUF:
		msgq(sp, M_ERR,
		    "201|Buffers should be specified before the command");
		break;
	case VIM_EOF:
		msgq(sp, M_BERR, "202|Already at end-of-file");
		break;
	case VIM_EOL:
		msgq(sp, M_BERR, "203|Already at end-of-line");
		break;
	case VIM_NOCOM:
	case VIM_NOCOM_B:
		msgq(sp,
		    which == VIM_NOCOM_B ? M_BERR : M_ERR,
		    "204|%s isn't a vi command", p);
		break;
	case VIM_USAGE:
		msgq(sp, M_ERR, "205|Usage: %s", p);
		break;
	}
}
