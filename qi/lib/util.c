/*
 * Copyright (c) 1985 Corporation for Research and Educational Networking
 * Copyright (c) 1988 University of Illinois Board of Trustees, Steven
 *		Dorner, and Paul Pomes
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Corporation for
 *	Research and Educational Networking (CREN), the University of
 *	Illinois at Urbana, and their contributors.
 * 4. Neither the name of CREN, the University nor the names of their
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE TRUSTEES AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TRUSTEES OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char  RcsId[] = "@(#)$Id: util.c,v 1.15 1994/09/09 20:15:13 p-pomes Exp $";
#endif

#include "protos.h"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

static char Delimiters[] = "  ,.:()&#;\n";

void 
free_ary(ary)
	INT32 **ary;
{
	register int i;

	for (i = 0; ary[i]; i++)
	{
		if (ary[i] != (void *) NIL)
			free(ary[i]);
		ary[i] = 0;
	}
}

int 
length(ary)
	INT32 *ary;
{
	int	len;

	if (!ary)
		return (0);
	for (len = 0; *ary++; len++) ;

	return (len);
}

int 
Plength(ary)
	INT32 **ary;
{
	int	len;

	if (!ary)
		return (0);
	for (len = 0; *ary++; len++) ;

	return (len);
}

char *
getword(str, buf)
	char *str, *buf;
{
	register i = 0;

	/* skip extraneous beginnings */
	while (*str && any(*str, Delimiters))
		str++;

	if (*str == '\"')
	{			/* this is a quoted string */
		do
		{		/* grab up to next quote */
			*buf++ = *str++;
			if (++i >= (WORDSIZE - 1))	/* limit word length */
				break;
			if (*str == '\"')
			{
				if (*++str != '\"')
				{	/* not quoted quote */
					*buf++ = '\"';
					break;
				}
			}
		}
		while (*str);

	} else
	{
		/* copy until uninteresting char found */
		while (*str && !any(*str, Delimiters))
		{
			*buf++ = *str++;
			if (++i >= (WORDSIZE - 1))	/* limit word length */
				break;
		}
	}

	*buf = '\0';
	return (str);
}


/*
 * if the string is quoted, strip the quotes in place and return 1 else 0
 */
int 
quoted(str)
	char *str;
{
	register char *ptr = str;

	if (*str != '\"')
		return 0;
	while (*ptr = *++str)
		ptr++;
	if (ptr[-1] == '\"')
		ptr[-1] = '\0';
	return 1;
}


/*
 * This function returns an index into the index file for string str limit is
 * the max integer - 1 allowable. returns 1 <= x <= limit-1
 */

#define SEED  341		/* it's something isn't it? */

INT32 
ihash(str, seed, limit)
	char *str;
	INT32 seed, limit;
{
	int	i;

	for (i = seed; *str; str++)
	{
		i = i * SEED + (isupper(*str) ? tolower(*str) : *str);
	}
	if (i < 0)
		i = -i;
	return (i % (limit - 1) + 1);	/* zero not allowed */
}

static int SysLog = 1;
int 
DoSysLog(yes)
	int yes;
{
	SysLog = yes;
}

#ifdef __STDC__
int 
IssueMessage(int severity, char *fmt,...)
#else /* !__STDC__ */
int 
IssueMessage(severity, fmt, va_alist)
	int severity;
	char *fmt;
va_dcl
#endif /* __STDC__ */
{
	va_list	args;

	if (OP_VALUE(NOLOG_OP))
		return (0);
#ifdef __STDC__
	va_start(args, fmt);
#else /* !__STDC__ */
	va_start(args);
#endif /* __STDC__ */
	if (SysLog)
	{
		char	buffer[4096];

		vsprintf(buffer, fmt, args);
		syslog(severity, "%s", buffer);
	} else
	{
		fprintf(stderr, "Severity %d: ", severity);
		vfprintf(stderr, fmt, args);
		fputs("\n", stderr);
	}
	va_end(args);
	return (0);
}
