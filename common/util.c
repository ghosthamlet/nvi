/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: util.c,v 5.27 1993/02/19 13:39:56 bostic Exp $ (Berkeley) $Date: 1993/02/19 13:39:56 $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vi.h"
#include "options.h"
#include "pathnames.h"

/*
 * __putchar --
 *	Functional version of putchar, for tputs.
 */
void
__putchar(ch)
	int ch;
{
	(void)putchar(ch);
}

/*
 * binc --
 *	Increase the size of a buffer.
 */
int
binc(ep, argp, bsizep, min)
	EXF *ep;
	void *argp;
	size_t *bsizep, min;
{
	void *bpp;
	size_t csize;

	/* If already larger than the minimum, just return. */
	csize = *bsizep;
	if (min && csize >= min)
		return (0);

	csize += MAX(min, 256);
	bpp = *(char **)argp;

	/* For non-ANSI C realloc implementations. */
	if (bpp == NULL)
		bpp = malloc(csize);
	else
		bpp = realloc(bpp, csize);
	if (bpp == NULL) {
		msg(ep, M_ERROR, "Error: %s.", strerror(errno));
		*bsizep = 0;
		return (1);
	}
	*(char **)argp = bpp;
	*bsizep = csize;
	return (0);
}

/*
 * nonblank --
 *	Return the column number of the first non-blank character of the
 *	line.
 */
int
nonblank(ep, lno, cnop)
	EXF *ep;
	recno_t lno;
	size_t *cnop;
{
	register int cnt;
	register u_char *p;
	size_t len;

	if ((p = file_gline(ep, lno, &len)) == NULL) {
		if (file_lline(ep) == 0) {
			*cnop = 0;
			return (0);
		}
		GETLINE_ERR(ep, lno);
		return (1);
	}
	for (cnt = 0; len-- && isspace(*p); ++cnt, ++p);
	*cnop = cnt;
	return (0);
}

/*
 * tail --
 *	Return tail of a path.
 */
char *
tail(path)
	char *path;
{
	char *p;

	if ((p = strrchr(path, '/')) == NULL)
		return (path);
	return (p + 1);
}

/*
 * onhup --
 *	Handle SIGHUP, restoring sanity to the terminal, preserving the file.
 */
/* ARGSUSED */
void
onhup(signo)
	int signo;
{
	/* Restore the terminal's sanity. */
	endwin();

#ifdef XXX_RIP_THIS_OUT
	/* If we had a temp file going, then preserve it. */
	if (tmpnum > 0 && tmpfd >= 0) {
		(void)close(tmpfd);
		(void)snprintf(buf, sizeof(buf),
		    "%s %s", _PATH_PRESERVE, tmpname);
		(void)esystem(PVAL(O_SHELL), tmpblk.c);
	}

	/* Delete any old temp files. */
	cutend();
#endif

	/* Exit with the proper exit status. */
	(void)signal(SIGHUP, SIG_DFL);
	(void)kill(0, SIGHUP);

	/* NOTREACHED */
	exit (1);
}
