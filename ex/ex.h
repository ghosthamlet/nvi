/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: ex.h,v 5.42 1993/05/10 11:34:14 bostic Exp $ (Berkeley) $Date: 1993/05/10 11:34:14 $
 */

struct _excmdarg;

/* Ex command structure. */
typedef struct _excmdlist {
	char	*name;			/* Command name. */
					/* Underlying function. */
	int (*fn) __P((SCR *, EXF *, struct _excmdarg *));

#define	E_ADDR1		0x00001		/* One address. */
#define	E_ADDR2		0x00002		/* Two address. */
#define	E_ADDR2_ALL	0x00004		/* Zero/two addresses; zero == all. */
#define	E_ADDR2_NONE	0x00008		/* Zero/two addresses; zero == none. */
#define	E_FORCE		0x00010		/*  ! */

#define	E_F_CARAT	0x00020		/*  ^ flag. */
#define	E_F_DASH	0x00040		/*  - flag. */
#define	E_F_DOT		0x00080		/*  . flag. */
#define	E_F_HASH	0x00100		/*  # flag. */
#define	E_F_LIST	0x00200		/*  l flag. */
#define	E_F_PLUS	0x00400		/*  + flag. */
#define	E_F_PRINT	0x00800		/*  p flag. */
#define	E_F_MASK	0x00fe0		/* Flag mask. */
#define	E_F_PRCLEAR	0x01000		/* Clear the print (#, l, p) flags. */

#define	E_NOGLOBAL	0x02000		/* Not in a global. */
#define	E_NOPERM	0x04000		/* Permission denied for now. */
#define	E_NORC		0x08000		/* Not from a .exrc or EXINIT. */
#define	E_SETLAST	0x10000		/* Reset last command. */
#define	E_ZERO		0x20000		/* 0 is a legal addr1. */
#define	E_ZERODEF	0x40000		/* 0 is default addr1 of empty files. */
	u_int	 flags;
	char	*syntax;		/* Syntax script. */
	char	*usage;			/* Usage line. */
} EXCMDLIST;
extern EXCMDLIST cmds[];		/* List of ex commands. */

/* Structure passed around to functions implementing ex commands. */
typedef struct _excmdarg {
	EXCMDLIST *cmd;		/* Command entry in command table. */
	int addrcnt;		/* Number of addresses (0, 1 or 2). */
	MARK addr1;		/* 1st address. */
	MARK addr2;		/* 2nd address. */
	recno_t lineno;		/* Line number. */
	u_int flags;		/* Selected flags from EXCMDLIST. */
	int argc;		/* Count of file/word arguments. */
	char **argv;		/* List of file/word arguments. */
	char *command;		/* Command line, if parse locally. */
	char *plus;		/* '+' command word. */
	char *string;		/* String. */
	int buffer;		/* Named buffer. */
} EXCMDARG;

extern char *defcmdarg[2];	/* Default array. */

/* Macro to set up the structure. */
#define	SETCMDARG(s, _cmd, _addrcnt, _lno1, _lno2, _force, _arg) {	\
	memset(&s, 0, sizeof(EXCMDARG));				\
	s.cmd = &cmds[_cmd];						\
	s.addrcnt = (_addrcnt);						\
	s.addr1.lno = (_lno1);						\
	s.addr2.lno = (_lno2);						\
	s.addr1.cno = s.addr2.cno = 1;					\
	if (_force)							\
		s.flags |= E_FORCE;					\
	s.argc = _arg ? 1 : 0;						\
	s.argv = defcmdarg;						\
	s.string = "";							\
	defcmdarg[0] = _arg;						\
}

/* Control character. */
#define	ctrl(ch)	((ch) & 0x1f)

/* Ex function prototypes. */
int	buildargv __P((SCR *, EXF *, char *, int, int *, char ***));
int	esystem __P((SCR *, const char *, const char *));

int	ex __P((struct _scr *, struct _exf *));
int	ex_cfile __P((SCR *, EXF *, char *, int));
int	ex_cmd __P((SCR *, EXF *, char *));
int	ex_cstring __P((SCR *, EXF *, char *, int));
int	ex_end __P((SCR *));
int	ex_gb __P((SCR *, EXF *, HDR *, int, u_int));
int	ex_getline __P((SCR *, FILE *, size_t *));
int	ex_init __P((SCR *, EXF *));
int	ex_print __P((SCR *, EXF *, MARK *, MARK *, int));
int	ex_readfp __P((SCR *, EXF *, char *, FILE *, MARK *, recno_t *));
int	ex_suspend __P((SCR *));
int	ex_writefp __P((SCR *, EXF *, char *, FILE *, MARK *, MARK *, int));
void	ex_refresh __P((SCR *, EXF *));

#define	EXPROTO(type, name)						\
	type	name __P((SCR *, EXF *, EXCMDARG *));

EXPROTO(int, ex_abbr);
EXPROTO(int, ex_append);
EXPROTO(int, ex_args);
EXPROTO(int, ex_at);
EXPROTO(int, ex_bang);
EXPROTO(int, ex_bdisplay);
EXPROTO(int, ex_cc);
EXPROTO(int, ex_cd);
EXPROTO(int, ex_change);
EXPROTO(int, ex_color);
EXPROTO(int, ex_copy);
EXPROTO(int, ex_debug);
EXPROTO(int, ex_delete);
EXPROTO(int, ex_digraph);
EXPROTO(int, ex_edit);
EXPROTO(int, ex_equal);
EXPROTO(int, ex_errlist);
EXPROTO(int, ex_file);
EXPROTO(int, ex_global);
EXPROTO(int, ex_join);
EXPROTO(int, ex_list);
EXPROTO(int, ex_make);
EXPROTO(int, ex_map);
EXPROTO(int, ex_mark);
EXPROTO(int, ex_mkexrc);
EXPROTO(int, ex_move);
EXPROTO(int, ex_next);
EXPROTO(int, ex_number);
EXPROTO(int, ex_pr);
EXPROTO(int, ex_prev);
EXPROTO(int, ex_put);
EXPROTO(int, ex_quit);
EXPROTO(int, ex_read);
EXPROTO(int, ex_rew);
EXPROTO(int, ex_set);
EXPROTO(int, ex_shell);
EXPROTO(int, ex_shiftl);
EXPROTO(int, ex_shiftr);
EXPROTO(int, ex_source);
EXPROTO(int, ex_split);
EXPROTO(int, ex_stop);
EXPROTO(int, ex_subagain);
EXPROTO(int, ex_substitute);
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
