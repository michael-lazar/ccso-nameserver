/*
 * Portable vfprintf  by Robert A. Larson <blarson@skat.usc.edu>
 *
 * Copyright 1989 Robert A. Larson.
 * Distribution in any form is allowed as long as the author
 * retains credit, changes are noted by their author and the
 * copyright message remains intact.  This program comes as-is
 * with no warentee of fitness for any purpouse.
 *
 * Thanks to Doug Gwen, Chris Torek, and others who helped clarify
 * the ansi printf specs.
 *
 * Please send any bug fixes and improvments to blarson@skat.usc.edu .
 * The use of goto is NOT a bug.
 *
 * Feb	7, 1989		blarson		First usenet release
 *
 * Changes for use with the IDA sendmail made by Paul Pomes, University
 * of Illinois, Computing Services Office.  The original version is checked
 * in to the RCS directory as number 1.1.
 */

/* This code implements the vsprintf function, without relying on
 * the existance of _doprint or other system specific code.
 *
 * Define NOVOID if void * is not a supported type.
 *
 * Two compile options are available for efficency:
 *	INTSPRINTF	should be defined if sprintf is int and returns
 *			the number of chacters formated.
 *	LONGINT		should be defined if sizeof(INT32) == sizeof(int)
 *
 *	They only make the code smaller and faster, they need not be
 *	defined.
 *
 * UNSIGNEDSPECIAL should be defined if unsigned is treated differently
 * than int in argument passing.  If this is definded, and LONGINT is not,
 * the compiler must support the type unsigned INT32.
 *
 * Most quirks and bugs of the available sprintf fuction are duplicated,
 * however * in the width and precision fields will work correctly
 * even if sprintf does not support this, as will the n format.
 *
 * Bad format strings, or those with very long width and precision
 * fields (including expanded * fields) will cause undesired results.
 */
#include "conf.h"

#ifdef NO_VFPRINTF
#include <stdio.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

# if !defined(lint)
static char sccsid[] = "@(#)vprint.c	1.1 (USC) 2/7/89";
static char rcsid[] = "@(#)$Id: vprintf.c,v 1.3 1994/09/09 20:15:13 p-pomes Exp $";
# endif /* not lint */

# ifndef __STDC__
#  define NOVOID			/* a possibly reasonable assumption */
# endif /* !__STDC__ */

# ifdef OSK			/* os9/68k can take advantage of both */
#  define LONGINT
#  define INTSPRINTF
# endif /* OSK */

/* This must be a typedef not a #define! */
# ifdef NOVOID
typedef char *pointer;
# else /* !NOVOID */
typedef void *pointer;
# endif /* NOVOID */

# ifdef INTSPRINTF
#  define Sprintf(string,format,arg)	(sprintf((string),(format),(arg)))
# else /* !INTSPRINTF */
# define Sprintf(string,format,arg)	(\
	sprintf((string),(format),(arg)),\
	strlen(string)\
)
# endif /* INTSPRINTF */

typedef int *intp;

int
# ifdef __STDC__
vsprintf (char *dest, const char *format, va_list args)
# else /* !__STDC__ */
vsprintf (dest, format, args)
	char    *dest;
	register char *format;
	va_list  args;
# endif /* __STDC__ */
{
	register char *dp = dest;
	register char c;
	register char *tp;
	char     tempfmt[64];

# ifndef LONGINT
	int      longflag;

# endif /* !LONGINT */

	tempfmt[0] = '%';
	while (c = *format++) {
		if (c == '%') {
			tp = &tempfmt[1];
# ifndef LONGINT
			longflag = 0;
# endif /* !LONGINT */
	continue_format:
			switch (c = *format++) {
			    case 's':
				*tp++ = c;
				*tp = '\0';
				dp += Sprintf (dp, tempfmt, va_arg (args, char *));
				break;
			    case 'u':
			    case 'x':
			    case 'o':
			    case 'X':
# ifdef UNSIGNEDSPECIAL
				*tp++ = c;
				*tp = '\0';
#  ifndef LONGINT
				if (longflag)
					dp += Sprintf (dp, tempfmt, va_arg (args, unsigned INT32));
				else
#  endif /* !LONGINT */
					dp += Sprintf (dp, tempfmt, va_arg (args, unsigned));
				break;
# endif /* UNSIGNEDSPECIAL */
			    case 'd':
			    case 'c':
			    case 'i':
				*tp++ = c;
				*tp = '\0';
# ifndef LONGINT
				if (longflag)
					dp += Sprintf (dp, tempfmt, va_arg (args, INT32));
				else
# endif /* !LONGINT */
					dp += Sprintf (dp, tempfmt, va_arg (args, int));
				break;
			    case 'f':
			    case 'e':
			    case 'E':
			    case 'g':
			    case 'G':
				*tp++ = c;
				*tp = '\0';
				dp += Sprintf (dp, tempfmt, va_arg (args, double));
				break;
			    case 'p':
				*tp++ = c;
				*tp = '\0';
				dp += Sprintf (dp, tempfmt, va_arg (args, pointer));
				break;
			    case '-':
			    case '+':
			    case '0':
			    case '1':
			    case '2':
			    case '3':
			    case '4':
			    case '5':
			    case '6':
			    case '7':
			    case '8':
			    case '9':
			    case '.':
			    case ' ':
			    case '#':
			    case 'h':
				*tp++ = c;
				goto continue_format;
			    case 'l':
# ifndef LONGINT
				longflag = 1;
				*tp++ = c;
# endif /* !LONGINT */
				goto continue_format;
			    case '*':
				tp += Sprintf (tp, "%d", va_arg (args, int));
				goto continue_format;
			    case 'n':
				*va_arg (args, intp) = dp - dest;
				break;
			    case '%':
			    default:
				*dp++ = c;
				break;
			}
		} else
			*dp++ = c;
	}
	*dp = '\0';
	return dp - dest;
}


int 
# ifdef __STDC__
vfprintf (FILE *dest, const char *format, va_list args)
# else /* !__STDC__ */
vfprintf (dest, format, args)
	FILE    *dest;
	register char *format;
	va_list  args;
# endif /* __STDC__ */
{
	register char c;
	register char *tp;
	register int count = 0;
	char     tempfmt[64];

# ifndef LONGINT
	int      longflag;

# endif /* !LONGINT */

	tempfmt[0] = '%';
	while (c = *format++) {
		if (c == '%') {
			tp = &tempfmt[1];
# ifndef LONGINT
			longflag = 0;
# endif /* !LONGINT */
	continue_format:
			switch (c = *format++) {
			    case 's':
				*tp++ = c;
				*tp = '\0';
				count += fprintf (dest, tempfmt, va_arg (args, char *));
				break;
			    case 'u':
			    case 'x':
			    case 'o':
			    case 'X':
# ifdef UNSIGNEDSPECIAL
				*tp++ = c;
				*tp = '\0';
#  ifndef LONGINT
				if (longflag)
					count += fprintf (dest, tempfmt, va_arg (args, unsigned INT32));
				else
#  endif /* !LONGINT */
					count += fprintf (dest, tempfmt, va_arg (args, unsigned));
				break;
# endif /* UNSIGNEDSPECIAL */
			    case 'd':
			    case 'c':
			    case 'i':
				*tp++ = c;
				*tp = '\0';
# ifndef LONGINT
				if (longflag)
					count += fprintf (dest, tempfmt, va_arg (args, INT32));
				else
# endif /* !LONGINT */
					count += fprintf (dest, tempfmt, va_arg (args, int));
				break;
			    case 'f':
			    case 'e':
			    case 'E':
			    case 'g':
			    case 'G':
				*tp++ = c;
				*tp = '\0';
				count += fprintf (dest, tempfmt, va_arg (args, double));
				break;
			    case 'p':
				*tp++ = c;
				*tp = '\0';
				count += fprintf (dest, tempfmt, va_arg (args, pointer));
				break;
			    case '-':
			    case '+':
			    case '0':
			    case '1':
			    case '2':
			    case '3':
			    case '4':
			    case '5':
			    case '6':
			    case '7':
			    case '8':
			    case '9':
			    case '.':
			    case ' ':
			    case '#':
			    case 'h':
				*tp++ = c;
				goto continue_format;
			    case 'l':
# ifndef LONGINT
				longflag = 1;
				*tp++ = c;
# endif /* !LONGINT */
				goto continue_format;
			    case '*':
				tp += Sprintf (tp, "%d", va_arg (args, int));
				goto continue_format;
			    case 'n':
				*va_arg (args, intp) = count;
				break;
			    case '%':
			    default:
				putc (c, dest);
				count++;
				break;
			}
		} else {
			putc (c, dest);
			count++;
		}
	}
	return count;
}

int
# ifdef __STDC__
vprintf (const char *format, va_list args)
# else /* !__STDC__ */
vprintf (format, args)
	register char *format;
	va_list  args;
# endif /* __STDC__ */
{
	register int nchars;
	nchars = vfprintf (stdout, format, args);
	return nchars;
}

#endif /* NO_VFPRINTF */
