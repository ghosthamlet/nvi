/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_subst.c,v 8.28 1993/12/23 11:30:34 bostic Exp $ (Berkeley) $Date: 1993/12/23 11:30:34 $";
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "vi.h"
#include "excmd.h"

#define	SUB_FIRST	0x01		/* The 'r' flag isn't reasonable. */
#define	SUB_MUSTSETR	0x02		/* The 'r' flag is required. */

static int		checkmatchsize __P((SCR *, regex_t *));
static inline int	regsub __P((SCR *,
			    char *, char **, size_t *, size_t *));
static int		substitute __P((SCR *, EXF *,
			    EXCMDARG *, char *, regex_t *, u_int));
/*
 * ex_substitute --
 *	[line [,line]] s[ubstitute] [[/;]pat[/;]/repl[/;] [cgr] [count] [#lp]]
 *
 *	Substitute on lines matching a pattern.
 */
int
ex_substitute(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	regex_t *re, lre;
	size_t blen, len;
	u_int flags;
	int delim, eval, reflags, replaced;
	char *bp, *ptrn, *rep, *p, *t;

	/*
	 * Skip leading white space.
	 *
	 * !!!
	 * Historic vi allowed any non-alphanumeric to serve as the
	 * substitution command delimiter.
	 *
	 * !!!
	 * If the arguments are empty, it's the same as &, i.e. we
	 * repeat the last substitution.
	 */
	for (p = cmdp->argv[0]->bp,
	    len = cmdp->argv[0]->len; len > 0; --len, ++p) {
		if (!isblank(*p))
			break;
	}
	if (len == 0)
		return (ex_subagain(sp, ep, cmdp));
	delim = *p++;
	if (isalnum(delim))
		return (substitute(sp, ep,
		    cmdp, p, &sp->subre, SUB_MUSTSETR));

	/*
	 * Get the pattern string, toss escaped characters.
	 *
	 * !!!
	 * Historic vi accepted any of the following forms:
	 *
	 *	:s/abc/def/		change "abc" to "def"
	 *	:s/abc/def		change "abc" to "def"
	 *	:s/abc/			delete "abc"
	 *	:s/abc			delete "abc"
	 *
	 * QUOTING NOTE:
	 *
	 * Only toss an escape character if it escapes a delimiter.
	 * This means that "s/A/\\\\f" replaces "A" with "\\f".  It
	 * would be nice to be more regular, i.e. for each layer of
	 * escaping a single escape character is removed, but that's
	 * not how the historic vi worked.
	 */
	for (ptrn = t = p;;) {
		if (p[0] == '\0' || p[0] == delim) {
			if (p[0] == delim)
				++p;
			/*
			 * !!!
			 * Nul terminate the pattern string -- it's passed
			 * to regcomp which doesn't understand anything else.
			 */
			*t = '\0';
			break;
		}
		if (p[0] == '\\' && p[1] == delim)
			++p;
		*t++ = *p++;
	}

	/* If the pattern string is empty, use the last one. */
	if (*ptrn == NULL) {
		if (!F_ISSET(sp, S_SUBRE_SET)) {
			msgq(sp, M_ERR,
			    "No previous regular expression.");
			return (1);
		}
		re = &sp->subre;
		flags = 0;
	} else {
		/* Set RE flags. */
		reflags = 0;
		if (O_ISSET(sp, O_EXTENDED))
			reflags |= REG_EXTENDED;
		if (O_ISSET(sp, O_IGNORECASE))
			reflags |= REG_ICASE;

		/* Convert vi-style RE's to POSIX 1003.2 RE's. */
		if (re_conv(sp, &ptrn, &replaced))
			return (1);

		/* Compile the RE. */
		eval = regcomp(&lre, (char *)ptrn, reflags);

		/* Free up any allocated memory. */
		if (replaced)
			FREE_SPACE(sp, ptrn, 0);

		if (eval) {
			re_error(sp, eval, &lre);
			return (1);
		}

		/*
		 * Set saved RE.
		 *
		 * !!!
		 * Historic practice is that substitutes set the search
		 * direction as well as both substitute and search RE's.
		 */
		sp->searchdir = FORWARD;
		sp->sre = lre;
		F_SET(sp, S_SRE_SET);
		sp->subre = lre;
		F_SET(sp, S_SUBRE_SET);

		re = &lre;
		flags = SUB_FIRST;
	}

	/*
	 * Get the replacement string.
	 *
	 * The special character ~ (\~ if O_MAGIC not set) inserts the
	 * previous replacement string into this replacement string.
	 *
	 * The special character & (\& if O_MAGIC not set) matches the
	 * entire RE.  No handling of & is required here, it's done by
	 * regsub().
	 *
	 * QUOTING NOTE:
	 *
	 * Only toss an escape character if it escapes a delimiter or
	 * an escape character, or if O_MAGIC is set and it escapes a
	 * tilde.
	 */
	if (*p == '\0') {
		if (sp->repl != NULL)
			FREE(sp->repl, sp->repl_len);
		sp->repl = NULL;
		sp->repl_len = 0;
	} else {
		/*
		 * Count ~'s to figure out how much space we need.  We could
		 * special case nonexistent last patterns or whether or not
		 * O_MAGIC is set, but it's probably not worth the effort.
		 */
		for (rep = p, len = 0;
		    p[0] != '\0' && p[0] != delim; ++p, ++len)
			if (p[0] == '~')
				len += sp->repl_len;
		GET_SPACE_RET(sp, bp, blen, len);
		for (t = bp, len = 0, p = rep;;) {
			if (p[0] == '\0' || p[0] == delim) {
				if (p[0] == delim)
					++p;
				break;
			}
			if (p[0] == '\\') {
				if (p[1] == '\\' || p[1] == delim)
					++p;
				else if (p[1] == '~') {
					++p;
					if (!O_ISSET(sp, O_MAGIC))
						goto tilde;
				}
			} else if (p[0] == '~' && O_ISSET(sp, O_MAGIC)) {
tilde:				++p;
				memmove(t, sp->repl, sp->repl_len);
				t += sp->repl_len;
				len += sp->repl_len;
				continue;
			}
			*t++ = *p++;
			++len;
		}
		if (sp->repl != NULL)
			FREE(sp->repl, sp->repl_len);
		if ((sp->repl = malloc(len)) == NULL) {
			msgq(sp, M_SYSERR, NULL);
			FREE_SPACE(sp, bp, blen);
			return (1);
		}
		memmove(sp->repl, bp, len);
		sp->repl_len = len;
		FREE_SPACE(sp, bp, blen);
	}

	if (checkmatchsize(sp, &sp->subre))
		return (1);
	return (substitute(sp, ep, cmdp, p, re, flags));
}

/*
 * ex_subagain --
 *	[line [,line]] & [cgr] [count] [#lp]]
 *
 *	Substitute using the last substitute RE and replacement pattern.
 */
int
ex_subagain(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	if (!F_ISSET(sp, S_SUBRE_SET)) {
		msgq(sp, M_ERR, "No previous regular expression.");
		return (1);
	}
	return (substitute(sp, ep, cmdp, cmdp->argv[0]->bp, &sp->subre, 0));
}

/*
 * ex_subtilde --
 *	[line [,line]] ~ [cgr] [count] [#lp]]
 *
 *	Substitute using the last RE and last substitute replacement pattern.
 */
int
ex_subtilde(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	if (!F_ISSET(sp, S_SRE_SET)) {
		msgq(sp, M_ERR, "No previous regular expression.");
		return (1);
	}
	return (substitute(sp, ep, cmdp, cmdp->argv[0]->bp, &sp->sre, 0));
}

/* 
 * The nasty part of the substitution is what happens when the replacement
 * string contains newlines.  It's a bit tricky -- consider the information
 * that has to be retained for "s/f\(o\)o/^M\1^M\1/".  The solution here is
 * to build a set of newline offets which we use to break the line up later,
 * when the replacement is done.  Don't change it unless you're pretty damned
 * confident.
 */
#define	NEEDNEWLINE(sp) {						\
	if (sp->newl_len == sp->newl_cnt) {				\
		sp->newl_len += 25;					\
		REALLOC(sp, sp->newl, size_t *,				\
		    sp->newl_len * sizeof(size_t));			\
		if (sp->newl == NULL) {					\
			sp->newl_len = 0;				\
			return (1);					\
		}							\
	}								\
}

#define	BUILD(sp, l, len) {						\
	if (lbclen + (len) > lblen) {					\
		lblen += MAX(lbclen + (len), 256);			\
		REALLOC(sp, lb, char *, lblen);				\
		if (lb == NULL) {					\
			lbclen = 0;					\
			return (1);					\
		}							\
	}								\
	memmove(lb + lbclen, l, len);					\
	lbclen += len;							\
}

#define	NEEDSP(sp, len, pnt) {						\
	if (lbclen + (len) > lblen) {					\
		lblen += MAX(lbclen + (len), 256);			\
		REALLOC(sp, lb, char *, lblen);				\
		if (lb == NULL) {					\
			lbclen = 0;					\
			return (1);					\
		}							\
		pnt = lb + lbclen;					\
	}								\
}

/*
 * substitute --
 *	Do the substitution.  This stuff is *really* tricky.  There are
 *	lots of special cases, and general nastiness.  Don't mess with it
 * 	unless you're pretty confident.
 */
static int
substitute(sp, ep, cmdp, s, re, flags)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
	char *s;
	regex_t *re;
	u_int flags;
{
	MARK from, to;
	recno_t elno, lno, lastline;
	size_t blen, cnt, last, lbclen, lblen, len, offset;
	int do_eol_match, eflags, empty_ok, eval, linechanged, quit;
	int cflag, gflag, lflag, nflag, pflag, rflag;
	char *bp, *lb;

	/*
	 * Historic vi permitted the '#', 'l' and 'p' options in vi mode, but
	 * it only displayed the last change.  I'd disallow them, but they are
	 * useful in combination with the [v]global commands.  In the current
	 * model the problem is combining them with the 'c' flag -- the screen
	 * would have to flip back and forth between the confirm screen and the
	 * ex print screen, which would be pretty awful.  We do display all
	 * changes, though, for what that's worth.
	 *
	 * !!!
	 * Historic vi was fairly strict about the order of "options", the
	 * count, and "flags".  I'm somewhat fuzzy on the difference between
	 * options and flags, anyway, so this is a simpler approach, and we
	 * just take it them in whatever order the user gives them.  (The ex
	 * usage statement doesn't reflect this.)
	 */
	cflag = gflag = lflag = nflag = pflag = rflag = 0;
	for (lno = OOBLNO; *s != '\0'; ++s)
		switch (*s) {
		case ' ':
		case '\t':
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (lno != OOBLNO)
				goto usage;
			errno = 0;
			lno = strtoul(s, &s, 10);
			if (*s == '\0')		/* Loop increment correction. */
				--s;
			if (errno == ERANGE) {
				if (lno == LONG_MAX)
					msgq(sp, M_ERR, "Count overflow.");
				else if (lno == LONG_MIN)
					msgq(sp, M_ERR, "Count underflow.");
				else
					msgq(sp, M_SYSERR, NULL);
				return (1);
			}
			/*
			 * In historic vi, the count was inclusive from the
			 * second address.
			 */
			cmdp->addr1.lno = cmdp->addr2.lno;
			cmdp->addr2.lno += lno - 1;
			break;
		case '#':
			nflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'g':
			gflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'r':
			if (LF_ISSET(SUB_FIRST)) {
				msgq(sp, M_ERR,
		    "Regular expression specified; r flag meaningless.");
				return (1);
			}
			if (!F_ISSET(sp, S_SUBRE_SET)) {
				msgq(sp, M_ERR,
				    "No previous regular expression.");
				return (1);
			}
			rflag = 1;
			break;
		default:
			goto usage;
		}

	if (*s != '\0' || !rflag && LF_ISSET(SUB_MUSTSETR)) {
usage:		msgq(sp, M_ERR, "Usage: %s", cmdp->cmd->usage);
		return (1);
	}

	if (IN_VI_MODE(sp) && cflag && (lflag || nflag || pflag)) {
		msgq(sp, M_ERR,
	"The #, l and p flags may not be combined with the c flag in vi mode.");
		return (1);
	}

	/* Get some space. */
	GET_SPACE_RET(sp, bp, blen, 512);

	/*
	 * lb:		build buffer pointer.
	 * lbclen:	current length of built buffer.
	 * lblen;	length of build buffer.
	 */
	lb = NULL;
	lbclen = lblen = 0;

	/*
	 * Since multiple changes can happen in a line, we only increment
	 * the change count on the first change to a line.
	 */
	lastline = OOBLNO;

	/* For each line... */
	for (quit = 0, lno = cmdp->addr1.lno,
	    elno = cmdp->addr2.lno; !quit && lno <= elno; ++lno) {

		/* Get the line. */
		if ((s = file_gline(sp, ep, lno, &len)) == NULL) {
			GETLINE_ERR(sp, lno);
			return (1);
		}

		/*
		 * Make a local copy if doing confirmation -- when calling
		 * the confirm routine we're likely to lose our cached copy.
		 */
		if (cflag) {
			ADD_SPACE_RET(sp, bp, blen, len)
			memmove(bp, s, len);
			s = bp;
		}

		/* Reset the buffer pointer. */
		lbclen = 0;

		/* Reset empty match flag. */
		empty_ok = 1;

		/*
		 * We don't want to have to do a setline if the line didn't
		 * change -- keep track of whether or not this line changed.
		 */
		linechanged = 0;

		/* New line, do EOL match. */
		do_eol_match = 1;

		/* It's not nul terminated, but we pretend it is. */
		eflags = REG_STARTEND;

		/* The search area is from 's' to the end of the line. */
nextmatch:	sp->match[0].rm_so = 0;
		sp->match[0].rm_eo = len;

		/* Get the next match. */
skipmatch:	eval = regexec(re,
		    (char *)s, re->re_nsub + 1, sp->match, eflags);

		/*
		 * There wasn't a match -- if there was an error, deal with
		 * it.  If there was a previous match in this line, resolve
		 * the changes into the database.  Otherwise, just move on.
		 */
		if (eval == REG_NOMATCH) {
			if (linechanged)
				goto endmatch;
			continue;
		}
		if (eval != 0) {
			re_error(sp, eval, re);
			goto ret1;
		}

		/*
		 * !!!
		 * It's possible to match 0-length strings -- for example, the
		 * command s;a*;X;, when matched against the string "aabb" will
		 * result in "XbXbX", i.e. the matches are "aa", the space
		 * between the b's and the space between the b's and the end of
		 * the string.  There is a similar space between the beginning
		 * of the string and the a's.  The rule that we use (because vi
		 * historically used it) is that any 0-length match, occurring
		 * immediately after a match, is ignored.  Otherwise, the above
		 * example would have resulted in "XXbXbX".  Another example is
		 * incorrectly using " *" to replace groups of spaces with one
		 * space.
		 *
		 * The way we do this is that if we just had a successful match,
		 * the starting offset does not skip characters, and the match
		 * is empty, ignore the match and move forward.  If there's no
		 * more characters in the string, we were attempting to match
		 * after the last character, so quit.
		 */
		if (!empty_ok && sp->match[0].rm_so == sp->match[0].rm_eo) {
			empty_ok = 1;

			/*
			 * Can't get here if !gflag or !linechanged, so just
			 * test cflag.  Same logic also guarantees that offset
			 * has been initialized.
			 */
			if (cflag) {
				if (sp->match[0].rm_so == offset) {
					if (len == offset)
						goto endmatch;
					BUILD(sp, s, 1)
					++s;
					--len;
					sp->match[0].rm_eo = len;
					goto skipmatch;
				}
			} else
				if (sp->match[0].rm_so == 0) {
					if (!len)
						goto endmatch;
					BUILD(sp, s, 1)
					++s;
					--len;
					goto nextmatch;
				}
		}

		/* Confirm change. */
		if (cflag) {
			/*
			 * Set the cursor position for confirmation.  Note,
			 * if we matched on a '$', the cursor may be past
			 * the end of line.
			 *
			 * XXX
			 * May want to "fix" this in the confirm routine;
			 * the confirm routine may be able to display a
			 * cursor past EOL.
			 */
			from.lno = lno;
			from.cno = sp->match[0].rm_so;
			to.lno = lno;
			to.cno = sp->match[0].rm_eo;
			if (len != 0) {
				if (to.cno >= len)
					to.cno = len - 1;
				if (from.cno >= len)
					from.cno = len - 1;
			}

			switch (sp->s_confirm(sp, ep, &from, &to)) {
			case CONF_YES:
				break;
			case CONF_NO:
				/*
				 * Copy the bytes before the match and the
				 * bytes in the match into the build buffer.
				 */
				BUILD(sp, s, sp->match[0].rm_eo);
				goto skip;
			case CONF_QUIT:
				/* Set the quit flag. */
				quit = 1;

				/* If interruptible, pass the info back. */
				if (F_ISSET(sp, S_INTERRUPTIBLE))
					F_SET(sp, S_INTERRUPTED);
				
				/*
				 * If any changes, resolve them, otherwise
				 * return to the main loop.
				 */
				if (linechanged)
					goto endmatch;
				continue;
			}
		}

		/* Copy the bytes before the match into the build buffer. */
		BUILD(sp, s, sp->match[0].rm_so);

		/*
		 * Update the cursor to the start of the change.
		 *
		 * !!!
		 * Historic vi just put the cursor on the first non-blank
		 * of the last line changed.  This might be better.
		 */
		if (!cflag)
			sp->cno = sp->match[0].rm_so;

		/* Substitute the matching bytes. */
		if (regsub(sp, s, &lb, &lbclen, &lblen))
			goto ret1;

		/* Set the change flag so we know this line was modified. */
		linechanged = 1;

		/* Move the pointers past the matched bytes. */
skip:		s += sp->match[0].rm_eo;
		len -= sp->match[0].rm_eo;

		/* Got a match, turn off empty patterns. */
		empty_ok = 0;

		/* Only the first search matches anchored expression. */
		eflags |= REG_NOTBOL;

		/*
		 * If doing a global change with confirmation, we have to
		 * update the screen.  The basic idea is to store the line
		 * so the screen update routines can find it, but start at
		 * the old offset.
		 */
		if (linechanged && cflag && gflag) {
			/* Save offset. */
			offset = lbclen;
			
			/* Copy the suffix. */
			if (len)
				BUILD(sp, s, len)

			/* Store inserted lines, adjusting the build buffer. */
			last = 0;
			if (sp->newl_cnt) {
				for (cnt = 0; cnt < sp->newl_cnt;
				    ++cnt, ++lno, ++elno, ++lastline) {
					if (file_iline(sp, ep, lno,
					    lb + last, sp->newl[cnt] - last))
						goto ret1;
					last = sp->newl[cnt] + 1;
					++sp->rptlines[L_ADDED];
				}
				lbclen -= last;
				offset -= last;

				sp->newl_cnt = 0;
			}

			/* Store and retrieve the line. */
			if (file_sline(sp, ep, lno, lb + last, lbclen))
				goto ret1;
			if ((s = file_gline(sp, ep, lno, &len)) == NULL) {
				GETLINE_ERR(sp, lno);
				goto ret1;
			}
			ADD_SPACE_RET(sp, bp, blen, len)
			memmove(bp, s, len);
			s = bp;

			/* Restart the build. */
			lbclen = 0;

			/* Update changed line counter. */
			if (lastline != lno) {
				++sp->rptlines[L_CHANGED];
				lastline = lno;
			}

			/*
			 * Do a test for the after the string match.  Set
			 * REG_NOTEOL so the '$' pattern only matches once.
			 */
			if (!do_eol_match)
				goto endmatch;

			if (offset == len) {
				do_eol_match = 0;
				eflags |= REG_NOTEOL;
			}

			/* Start in the middle of the line. */
			sp->match[0].rm_so = offset;
			sp->match[0].rm_eo = len;

			goto skipmatch;
		}

		/*
		 * If it's a global:
		 * Do a test for the after the string match.  Set
		 * REG_NOTEOL so the '$' pattern only matches once.
		 */
		if (gflag && do_eol_match) {
			if (!len) {
				do_eol_match = 0;
				eflags |= REG_NOTEOL;
			}
			goto nextmatch;
			
		}

		/* Copy any remaining bytes into the build buffer. */
endmatch:	if (len)
			BUILD(sp, s, len)

		/* Store inserted lines, adjusting the build buffer. */
		last = 0;
		if (sp->newl_cnt) {
			for (cnt = 0; cnt < sp->newl_cnt;
			    ++cnt, ++lno, ++elno, ++lastline) {
				if (file_iline(sp, ep,
				    lno, lb + last, sp->newl[cnt] - last))
					goto ret1;
				last = sp->newl[cnt] + 1;
				++sp->rptlines[L_ADDED];
			}
			lbclen -= last;

			sp->newl_cnt = 0;
			linechanged = 1;
		}

		/* Store the changed line. */
		if (linechanged)
			if (file_sline(sp, ep, lno, lb + last, lbclen))
				goto ret1;

		/* Update changed line counter. */
		if (lastline != lno) {
			++sp->rptlines[L_CHANGED];
			lastline = lno;
		}

		/* Display as necessary. */
		if (lflag || nflag || pflag) {
			from.lno = to.lno = lno;
			from.cno = to.cno = 0;
			if (lflag)
				ex_print(sp, ep, &from, &to, E_F_LIST);
			if (nflag)
				ex_print(sp, ep, &from, &to, E_F_HASH);
			if (pflag)
				ex_print(sp, ep, &from, &to, E_F_PRINT);
		}
	}

	/*
	 * Cursor moves to last line changed, unless doing confirm,
	 * in which case don't move it.
	 */
	if (!cflag && lastline != OOBLNO)
		sp->lno = lastline;

	/*
	 * Note if nothing found.  Else, if nothing displayed to the
	 * screen, put something up.
	 */
	if (sp->rptlines[L_CHANGED] == 0 && !F_ISSET(sp, S_GLOBAL))
		msgq(sp, M_INFO, "No match found.");
	else if (!lflag && !nflag && !pflag)
		F_SET(sp, S_AUTOPRINT);

	FREE_SPACE(sp, bp, blen);
	return (0);

ret1:	FREE_SPACE(sp, bp, blen);
	return (1);
}

/*
 * regsub --
 * 	Do the substitution for a regular expression.
 */
static inline int
regsub(sp, ip, lbp, lbclenp, lblenp)
	SCR *sp;
	char *ip;			/* Input line. */
	char **lbp;
	size_t *lbclenp, *lblenp;
{
	enum { C_NOTSET, C_LOWER, C_ONELOWER, C_ONEUPPER, C_UPPER } conv;
	size_t lbclen, lblen;		/* Local copies. */
	size_t mlen;			/* Match length. */
	size_t rpl;			/* Remaining replacement length. */
	char *rp;			/* Replacement pointer. */
	int ch;
	int no;				/* Match replacement offset. */
	char *p, *t;			/* Buffer pointers. */
	char *lb;			/* Local copies. */

	lb = *lbp;			/* Get local copies. */
	lbclen = *lbclenp;
	lblen = *lblenp;

	/*
	 * QUOTING NOTE:
	 *
	 * There are some special sequences that vi provides in the
	 * replacement patterns.
	 *	 & string the RE matched (\& if nomagic set)
	 *	\# n-th regular subexpression	
	 *	\E end \U, \L conversion
	 *	\e end \U, \L conversion
	 *	\l convert the next character to lower-case
	 *	\L convert to lower-case, until \E, \e, or end of replacement
	 *	\u convert the next character to upper-case
	 *	\U convert to upper-case, until \E, \e, or end of replacement
	 *
	 * Otherwise, since this is the lowest level of replacement, discard
	 * all escape characters.  This (hopefully) follows historic practice.
	 */
#define	ADDCH(ch) {							\
	CHAR_T __ch = (ch);						\
	u_int __value = term_key_val(sp, __ch);				\
	if (__value == K_CR || __value == K_NL) {			\
		NEEDNEWLINE(sp);					\
		sp->newl[sp->newl_cnt++] = lbclen;			\
	} else if (conv != C_NOTSET) {					\
		switch (conv) {						\
		case C_ONELOWER:					\
			conv = C_NOTSET;				\
			/* FALLTHROUGH */				\
		case C_LOWER:						\
			if (isupper(__ch))				\
				__ch = tolower(__ch);			\
			break;						\
		case C_ONEUPPER:					\
			conv = C_NOTSET;				\
			/* FALLTHROUGH */				\
		case C_UPPER:						\
			if (islower(__ch))				\
				__ch = toupper(__ch);			\
			break;						\
		default:						\
			abort();					\
		}							\
	}								\
	NEEDSP(sp, 1, p);						\
	*p++ = __ch;							\
	++lbclen;							\
}
	conv = C_NOTSET;
	for (rp = sp->repl, rpl = sp->repl_len, p = lb + lbclen; rpl--;) {
		switch (ch = *rp++) {
		case '&':
			if (O_ISSET(sp, O_MAGIC)) {
				no = 0;
				goto subzero;
			}
			break;
		case '\\':
			if (rpl == 0)
				break;
			--rpl;
			switch (ch = *rp) {
			case '&':
				if (!O_ISSET(sp, O_MAGIC)) {
					++rp;
					no = 0;
					goto subzero;
				}
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				no = *rp++ - '0';
subzero:			if (sp->match[no].rm_so == -1 ||
			    	    sp->match[no].rm_eo == -1)
					continue;
				mlen =
				    sp->match[no].rm_eo - sp->match[no].rm_so;
				for (t = ip + sp->match[no].rm_so; mlen--; ++t)
					ADDCH(*t);
				continue;
			case 'e':
			case 'E':
				++rp;
				conv = C_NOTSET;
				continue;
			case 'l':
				++rp;
				conv = C_ONELOWER;
				continue;
			case 'L':
				++rp;
				conv = C_LOWER;
				continue;
			case 'u':
				++rp;
				conv = C_ONEUPPER;
				continue;
			case 'U':
				++rp;
				conv = C_UPPER;
				continue;
			default:
				++rp;
				break;
			}
		}
		ADDCH(ch);
	}

	*lbp = lb;			/* Update caller's information. */
	*lbclenp = lbclen;
	*lblenp = lblen;
	return (0);
}

static int
checkmatchsize(sp, re)
	SCR *sp;
	regex_t *re;
{
	/* Build nsub array as necessary. */
	if (sp->matchsize < re->re_nsub + 1) {
		sp->matchsize = re->re_nsub + 1;
		REALLOC(sp, sp->match,
		    regmatch_t *, sp->matchsize * sizeof(regmatch_t));
		if (sp->match == NULL) {
			sp->matchsize = 0;
			return (1);
		}
	}
	return (0);
}
