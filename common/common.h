/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: common.h,v 8.14 1993/10/02 16:15:00 bostic Exp $ (Berkeley) $Date: 1993/10/02 16:15:00 $
 */

/* System includes. */
#include <sys/time.h>		/* Required by screen.h. */

#include <limits.h>		/* Required by screen.h. */
#include <stdio.h>		/* Required by screen.h. */
#include <termios.h>		/* Required by gs.h. */

/*
 * Required by screen.h.  This is the first include that can pull
 * in "compat.h".  Should be after every other system include.
 */
#include <regex.h>

/*
 * Forward structure declarations.  Not pretty, but the include files
 * are far too interrelated for a clean solution.
 */
struct _cb;
struct _excmdarg;
struct _excmdlist;
struct _exf;
struct _fref;
struct _gs;
struct _hdr;
struct _ibuf;
struct _mark;
struct _msg;
struct _option;
struct _scr;
struct _seq;
struct _tag;
struct _tagf;
struct _text;

/*
 * XXX
 * Nobody's signal return value is the same as anyone else's.
 * Don't even try.
 */
				/* Required by screen.h. */
typedef void (*sig_ret_t) __P((int));

/*
 * Integral type that can hold a single character, and the type of that
 * type when passed as an argument using traditional promotion rules.
 * If no integral type can hold a character, forget it, you're done.
 */
typedef	u_char	CHAR_T;
typedef	u_int	ARG_CHAR_T;

/*
 * Local includes.
 *
 * Required by everybody; include before any local includes.
 */
#include "link.h"

#include "gs.h"				

#include <db.h>			/* Required by exf.h; includes compat.h. */

#include "mark.h"		/* Required by cut.h, exf.h. */
#include "cut.h"

#include "search.h"		/* Required by screen.h. */
#include "options.h"		/* Required by screen.h. */
#include "term.h"		/* Required by screen.h. */
#include "screen.h"		/* Required by exf.h. */

#include "exf.h"
#include "log.h"
#include "msg.h"
#include "seq.h"

/* Macros to set/clear/test flags. */
#define	F_SET(p, f)	(p)->flags |= (f)
#define	F_CLR(p, f)	(p)->flags &= ~(f)
#define	F_ISSET(p, f)	((p)->flags & (f))

#define	LF_INIT(f)	flags = (f)
#define	LF_SET(f)	flags |= (f)
#define	LF_CLR(f)	flags &= ~(f)
#define	LF_ISSET(f)	(flags & (f))

/* Memory allocation macros. */
#define	BINC(sp, lp, llen, nlen) {					\
	if ((nlen) > llen && binc(sp, &(lp), &(llen), nlen))		\
		return (1);						\
}
int	binc __P((SCR *, void *, size_t *, size_t));

#define	GET_SPACE(sp, bp, blen, nlen) {					\
	GS *__gp = (sp)->gp;						\
	if (F_ISSET(__gp, G_TMP_INUSE)) {				\
		bp = NULL;						\
		blen = 0;						\
		BINC(sp, bp, blen, nlen); 				\
	} else {							\
		BINC(sp, __gp->tmp_bp, __gp->tmp_blen, nlen);		\
		bp = __gp->tmp_bp;					\
		blen = __gp->tmp_blen;					\
		F_SET(__gp, G_TMP_INUSE);				\
	}								\
}

#define	ADD_SPACE(sp, bp, blen, nlen) {					\
	GS *__gp = (sp)->gp;						\
	if (bp == __gp->tmp_bp) {					\
		F_CLR(__gp, G_TMP_INUSE);				\
		BINC(sp, __gp->tmp_bp, __gp->tmp_blen, nlen);		\
		bp = __gp->tmp_bp;					\
		blen = __gp->tmp_blen;					\
		F_SET(__gp, G_TMP_INUSE);				\
	} else								\
		BINC(sp, bp, blen, nlen);				\
}

#define	FREE_SPACE(sp, bp, blen) {					\
	if (bp == sp->gp->tmp_bp)					\
		F_CLR(sp->gp, G_TMP_INUSE);				\
	else								\
		FREE(bp, blen);						\
}

#ifdef DEBUG
#define	FREE(p, sz) {							\
	memset(p, 0xff, sz);						\
	free(p);							\
}
#else
#define	FREE(p, sz)	free(p);
#endif

/*
 * XXX
 * MIN/MAX have traditionally been in <sys/param.h>.  Don't
 * try to get them from there, it's just not worth the effort.
 */
#ifndef	MAX
#define	MAX(_a,_b)	((_a)<(_b)?(_b):(_a))
#endif
#ifndef	MIN
#define	MIN(_a,_b)	((_a)<(_b)?(_a):(_b))
#endif

/* Function prototypes that don't seem to belong anywhere else. */
char	*charname __P((SCR *, ARG_CHAR_T));
void	 busy_off __P((SCR *));
void	 busy_on __P((SCR *, int, char const *));
int	 nonblank __P((SCR *, EXF *, recno_t, size_t *));
void	 set_alt_fname __P((SCR *, char *));
int	 set_window_size __P((SCR *, u_int, int));
int	 status __P((SCR *, EXF *, recno_t, int));
char	*tail __P((char *));

#ifdef DEBUG
void	TRACE __P((SCR *, const char *, ...));
#endif

/* Digraphs (not currently real). */
int	digraph __P((SCR *, int, int));
int	digraph_init __P((SCR *));
void	digraph_save __P((SCR *, int));
