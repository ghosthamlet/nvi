/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: ex.h,v 8.39 1993/12/19 12:36:45 bostic Exp $ (Berkeley) $Date: 1993/12/19 12:36:45 $
 */

/* Ex command structure. */
typedef struct _excmdlist {
	char	*name;			/* Command name. */
					/* Underlying function. */
	int (*fn) __P((SCR *, EXF *, EXCMDARG *));

#define	E_ADDR1		0x0000001	/* One address. */
#define	E_ADDR2		0x0000002	/* Two address. */
#define	E_ADDR2_ALL	0x0000004	/* Zero/two addresses; zero == all. */
#define	E_ADDR2_NONE	0x0000008	/* Zero/two addresses; zero == none. */
#define	E_ADDRDEF	0x0000010	/* Default addresses used. */
#define	E_BUFFER	0x0000020	/* Buffer name supplied. */
#define	E_COUNT		0x0000040	/* Count supplied. */
#define	E_FORCE		0x0000080	/*  ! */

#define	E_F_CARAT	0x0000100	/*  ^ flag. */
#define	E_F_DASH	0x0000200	/*  - flag. */
#define	E_F_DOT		0x0000400	/*  . flag. */
#define	E_F_EQUAL	0x0000800	/*  = flag. */
#define	E_F_HASH	0x0001000	/*  # flag. */
#define	E_F_LIST	0x0002000	/*  l flag. */
#define	E_F_PLUS	0x0004000	/*  + flag. */
#define	E_F_PRINT	0x0008000	/*  p flag. */

#define	E_F_PRCLEAR	0x0010000	/* Clear the print (#, l, p) flags. */
#define	E_MODIFY	0x0020000	/* File name expansion modified arg. */
#define	E_NOGLOBAL	0x0040000	/* Not in a global. */
#define	E_NOPERM	0x0080000	/* Permission denied for now. */
#define	E_NORC		0x0100000	/* Not from a .exrc or EXINIT. */
#define	E_SETLAST	0x0200000	/* Reset last command. */
#define	E_ZERO		0x0400000	/* 0 is a legal addr1. */
#define	E_ZERODEF	0x0800000	/* 0 is default addr1 of empty files. */
	u_long	 flags;
	char	*syntax;		/* Syntax script. */
	char	*usage;			/* Usage line. */
	char	*help;			/* Help line. */
} EXCMDLIST;
#define	MAXCMDNAMELEN	12		/* Longest command name. */
extern EXCMDLIST const cmds[];		/* List of ex commands. */

/* Structure passed around to functions implementing ex commands. */
struct _excmdarg {
	EXCMDLIST const *cmd;	/* Command entry in command table. */
	CHAR_T	buffer;		/* Named buffer. */
	recno_t	lineno;		/* Line number. */
	long	count;		/* Signed, specified count. */
	int	addrcnt;	/* Number of addresses (0, 1 or 2). */
	MARK	addr1;		/* 1st address. */
	MARK	addr2;		/* 2nd address. */
	ARGS  **argv;		/* Array of arguments. */
	int	argc;		/* Count of arguments. */
	u_int	flags;		/* Selected flags from EXCMDLIST. */
};

/* Ex private, per-screen memory. */
typedef struct _ex_private {
	ARGS   **args;			/* Arguments. */
	int	 argscnt;		/* Argument count. */
	int	 argsoff;		/* Offset into arguments. */

	CHAR_T	 at_lbuf;		/* Last executed at buffer's name. */
	int	 at_lbuf_set;		/* If at_lbuf is set. */

	char	*ibp;			/* Line input buffer. */
	size_t	 ibp_len;		/* Line input buffer length. */

	EXCMDLIST const *lastcmd;	/* Last command. */

	CHAR_T	*lastbcomm;		/* Last bang command. */

	TAILQ_HEAD(_tagh, _tag) tagq;	/* Tag stack. */
	TAILQ_HEAD(_tagfh, _tagf) tagfq;/* Tag stack. */
	char	*tlast;			/* Saved last tag. */
} EX_PRIVATE;
#define	EXP(sp)	((EX_PRIVATE *)((sp)->ex_private))
	
/* Macro to set up a command structure. */
#define	SETCMDARG(s, cmd_id, naddr, lno1, lno2, force, arg) {		\
	ARGS *__ap[2], __a;						\
	memset(&s, 0, sizeof(EXCMDARG));				\
	s.cmd = &cmds[cmd_id];						\
	s.addrcnt = (naddr);						\
	s.addr1.lno = (lno1);						\
	s.addr2.lno = (lno2);						\
	s.addr1.cno = s.addr2.cno = 1;					\
	if (force)							\
		s.flags |= E_FORCE;					\
	if ((__a.bp = arg) == NULL) {					\
		s.argc = 0;						\
		__a.len = 0;						\
	} else {							\
		s.argc = 1;						\
		__a.len = strlen(arg);					\
	}								\
	__ap[0] = &__a;							\
	__ap[1] = NULL;							\
	s.argv = __ap;							\
}

/*
 * :next, :prev, :rewind, :tag, :tagpush, :tagpop modifications check.
 * If force is set, the autowrite is skipped.
 */
#define	MODIFY_CHECK(sp, ep, force) {					\
	if (F_ISSET((ep), F_MODIFIED))					\
		if (O_ISSET((sp), O_AUTOWRITE)) {			\
			if (!(force) &&					\
			    file_write((sp), (ep), NULL, NULL, NULL,	\
			    FS_ALL | FS_POSSIBLE))			\
				return (1);				\
		} else if (ep->refcnt <= 1 && !(force)) {		\
			msgq(sp, M_ERR,					\
	"Modified since last write; write or use ! to override.");	\
			return (1);					\
		}							\
}

/*
 * Macros to set and restore the terminal values.
 *
 * The old terminal values almost certainly turn on VINTR, VQUIT and VSUSP.
 * We don't want to interrupt the parent(s), so we ignore VINTR.  VQUIT is
 * ignored by main() because nvi never wants to catch it.  A VSUSP handler
 * have been installed by the screen code.
 */
#define	EX_RESTORE_TERM(sp, isig, act, oact, term) {			\
	(act).sa_handler = SIG_IGN;					\
	sigemptyset(&(act).sa_mask);					\
	(act).sa_flags = 0;						\
	if ((isig) = !sigaction(SIGINT, &(act), &(oact))) {		\
		if (tcgetattr(STDIN_FILENO, &(term))) {			\
			msgq(sp, M_SYSERR, "tcgetattr");		\
			goto err;					\
		}							\
		if (tcsetattr(STDIN_FILENO,				\
		    TCSANOW | TCSASOFT, &sp->gp->original_termios)) {	\
			msgq(sp, M_SYSERR, "tcsetattr");		\
			goto err;					\
		}							\
	}								\
}

#define	EX_SET_TERM(sp, oact, term) {					\
	if (sigaction(SIGINT, &(oact), NULL)) {				\
		msgq(sp, M_SYSERR, "signal");				\
		rval = 1;						\
	}								\
	if (tcsetattr(STDIN_FILENO, TCSANOW | TCSASOFT, &(term))) {	\
		msgq(sp, M_SYSERR, "tcsetattr");			\
		rval = 1;						\
	}								\
}
/*
 * Filter actions:
 *
 *	FILTER		Filter text through the utility.
 *	FILTER_READ	Read from the utility into the file.
 *	FILTER_WRITE	Write to the utility, display its output.
 */
enum filtertype { FILTER, FILTER_READ, FILTER_WRITE };
int	filtercmd __P((SCR *, EXF *,
	    MARK *, MARK *, MARK *, char *, enum filtertype));

/* Argument expansion routines. */
int	argv_init __P((SCR *, EXF *, EXCMDARG *));
int	argv_exp0 __P((SCR *, EXF *, EXCMDARG *, char *, size_t));
int	argv_exp1 __P((SCR *, EXF *, EXCMDARG *, char *, size_t, int));
int	argv_exp2 __P((SCR *, EXF *, EXCMDARG *, char *, size_t, int));
int	argv_exp3 __P((SCR *, EXF *, EXCMDARG *, char *, size_t));
int	argv_free __P((SCR *));

/* Ex function prototypes. */
int	ex __P((SCR *, EXF *));
int	ex_cfile __P((SCR *, EXF *, char *));
int	ex_cmd __P((SCR *, EXF *, char *, size_t));
int	ex_end __P((SCR *));
int	ex_exec_proc __P((SCR *, char *, char *, char *));
int	ex_gb __P((SCR *, EXF *, TEXTH *, int, u_int));
int	ex_getline __P((SCR *, FILE *, size_t *));
int	ex_icmd __P((SCR *, EXF *, char *, size_t));
int	ex_init __P((SCR *, EXF *));
int	ex_optchange __P((SCR *, int));
int	ex_print __P((SCR *, EXF *, MARK *, MARK *, int));
int	ex_readfp __P((SCR *, EXF *, char *, FILE *, MARK *, recno_t *, int));
void	ex_refresh __P((SCR *, EXF *));
int	ex_screen_copy __P((SCR *, SCR *));
int	ex_screen_end __P((SCR *));
int	ex_sdisplay __P((SCR *, EXF *));
int	ex_suspend __P((SCR *));
int	ex_tdisplay __P((SCR *, EXF *));
int	ex_writefp __P((SCR *, EXF *,
	    char *, FILE *, MARK *, MARK *, u_long *, u_long *));
int	proc_wait __P((SCR *, long, const char *, int));
int	sscr_end __P((SCR *));
int	sscr_exec __P((SCR *, EXF *, recno_t));
int	sscr_input __P((SCR *));

#define	EXPROTO(type, name)						\
	type name __P((SCR *, EXF *, EXCMDARG *))

EXPROTO(int, ex_abbr);
EXPROTO(int, ex_append);
EXPROTO(int, ex_args);
EXPROTO(int, ex_at);
EXPROTO(int, ex_bang);
EXPROTO(int, ex_bg);
EXPROTO(int, ex_cd);
EXPROTO(int, ex_change);
EXPROTO(int, ex_color);
EXPROTO(int, ex_copy);
EXPROTO(int, ex_debug);
EXPROTO(int, ex_delete);
EXPROTO(int, ex_digraph);
EXPROTO(int, ex_display);
EXPROTO(int, ex_edit);
EXPROTO(int, ex_equal);
EXPROTO(int, ex_fg);
EXPROTO(int, ex_file);
EXPROTO(int, ex_global);
EXPROTO(int, ex_help);
EXPROTO(int, ex_insert);
EXPROTO(int, ex_join);
EXPROTO(int, ex_list);
EXPROTO(int, ex_map);
EXPROTO(int, ex_mark);
EXPROTO(int, ex_mkexrc);
EXPROTO(int, ex_move);
EXPROTO(int, ex_next);
EXPROTO(int, ex_number);
EXPROTO(int, ex_open);
EXPROTO(int, ex_pr);
EXPROTO(int, ex_preserve);
EXPROTO(int, ex_prev);
EXPROTO(int, ex_put);
EXPROTO(int, ex_quit);
EXPROTO(int, ex_read);
EXPROTO(int, ex_resize);
EXPROTO(int, ex_rew);
EXPROTO(int, ex_script);
EXPROTO(int, ex_set);
EXPROTO(int, ex_shell);
EXPROTO(int, ex_shiftl);
EXPROTO(int, ex_shiftr);
EXPROTO(int, ex_source);
EXPROTO(int, ex_split);
EXPROTO(int, ex_stop);
EXPROTO(int, ex_subagain);
EXPROTO(int, ex_substitute);
EXPROTO(int, ex_subtilde);
EXPROTO(int, ex_tagpop);
EXPROTO(int, ex_tagpush);
EXPROTO(int, ex_tagtop);
EXPROTO(int, ex_unabbr);
EXPROTO(int, ex_undo);
EXPROTO(int, ex_undol);
EXPROTO(int, ex_unmap);
EXPROTO(int, ex_usage);
EXPROTO(int, ex_validate);
EXPROTO(int, ex_version);
EXPROTO(int, ex_vglobal);
EXPROTO(int, ex_visual);
EXPROTO(int, ex_viusage);
EXPROTO(int, ex_wq);
EXPROTO(int, ex_write);
EXPROTO(int, ex_xit);
EXPROTO(int, ex_yank);
EXPROTO(int, ex_z);
