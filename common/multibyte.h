#ifndef MULTIBYTE_H
#define MULTIBYTE_H

#include <wchar.h>
#include <wctype.h>

#ifdef USE_WIDECHAR
typedef	int		RCHAR_T;
#define RCHAR_T_MAX	((1 << 24)-1)
typedef	wchar_t		CHAR_T;
#define	MAX_CHAR_T	0xffffff    /* XXXX */
typedef	u_int		UCHAR_T;

#define STRLEN		wcslen
#define STRTOL		wcstol
#define SPRINTF		swprintf
#define STRCMP		wcscmp

#else
typedef	char		RCHAR_T;
#define RCHAR_T_MAX	CHAR_MAX
typedef	u_char		CHAR_T;
#define	MAX_CHAR_T	0xff
typedef	u_char		UCHAR_T;

#define STRLEN		strlen
#define STRTOL		strtol
#define SPRINTF		snprintf
#define STRCMP		strcmp

#endif

#define MEMCMP(to, from, n) 						    \
	memcmp(to, from, (n) * sizeof(*(to)))
#define	MEMMOVE(p, t, len)	memmove(p, t, (len) * sizeof(*(p)))
#define	MEMCPY(p, t, len)	memcpy(p, t, (len) * sizeof(*(p)))
#define STRSET(s,c,n)							    \
	sizeof(char) == sizeof(CHAR_T) ? memset(s,c,n) : v_strset(s,c,n)
#define SIZE(w)		(sizeof(w)/sizeof(*w))

#endif
