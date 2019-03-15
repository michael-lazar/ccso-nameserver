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
static char  RcsId[] = "@(#)$Id: str.c,v 1.11 1994/03/12 00:06:47 paul Exp $";
#endif

#include "protos.h"

/*LINTLIBRARY*/

#define to_lower(a) (((a)>='A'&&(a)<='Z')?((a)|040):(a))

/*
 * Just like strcmp but case independent.
 */
int 
stricmp(s1, s2)
	char *s1, *s2;
{
	register char c1, c2;

	for (c1 = *s1, c2 = *s2; to_lower(c1) == to_lower(c2); c1 = *s1, c2 = *s2)
	{
		s2++;
		if (*s1++ == '\0')
			return (0);
	}
	return (to_lower(c1) - to_lower(c2));
}

/*
 * Just like strncmp, but case independent.
 */
int 
strincmp(s1, s2, n)
	char *s1, *s2;
	int n;
{

	while (--n >= 0 && to_lower(*s1) == to_lower(*s2))
	{
		s2++;
		if (*s1++ == '\0')
			return (0);
	}
	return (n < 0 ? 0 : to_lower(*s1) - to_lower(*s2));
}

/*
 * Return true iff the given string is a blank line.
 */
int 
blankline(str)
	char *str;
{
	while (isspace(*str))
		str++;
	return ((*str == '\0') ? 1 : 0);
}

/*
 * Get rid of trailing spaces (and newlines and tabs ) as well as
 * beginning ones.  This routine just truncates the line in place.
 */
char *
ltrunc(str)
	char *str;
{
	register char *ptr;

	ptr = str;
	while (*ptr)
		ptr++;
	while (isspace(*--ptr))
		if (ptr < str)
			break;
	ptr[1] = '\0';
	while (isspace(*str))
		str++;
	return (str);
}

/*
 * Return true iff character (c) occurs in the string (list).
 */
int 
any(c, list)
	char c, *list;
{
	while (*list)
	{
		if (c == *list)
			return 1;
		list++;
	}
	return 0;
}

void 
mkargv(argc, argv, line)
	int *argc;
	char **argv, *line;
{
	int	count, instring;
	char	*ptr;

	count = 0;
	instring = 0;
	for (ptr = line; *ptr; ptr++)
	{
		if (isspace(*ptr))
		{
			instring = 0;
			*ptr = '\0';
		} else if (!instring)
		{
			argv[count++] = ptr;
			instring = 1;
		}
	}
	argv[count] = 0;
	*argc = count;
}

/*
 * Just like strcat, except return a ptr to the null byte at the end of
 * cat'ed string.
 */
char *
strecat(s1, s2)
	char *s1, *s2;
{
	while (*s1)
		s1++;
	while (*s1++ = *s2++)
		;
	return (--s1);
}

/*
 * Just like strcpy, except return a ptr to the null byte at the end of
 * cpy'ed string.
 */
char *
strecpy(s1, s2)
	char *s1, *s2;
{
	while (*s1++ = *s2++)
		;
	return (--s1);
}

/*
 * is one string a subset of another?
 */
int 
issub(string, sub)
	char *string, *sub;
{
	int	len;

	len = strlen(sub);
	for (; *string; string++)
		if (!strncmp(string, sub, len))
			return (1);
	return (0);
}

/*
 * copy a string, return the # of chars copied
 */
int 
strcpc(to, from)
	char *to, *from;
{
	char	*old;

	old = to;
	while (*to++ = *from++) ;

	return (to - old - 1);
}

/*
 * count the occurrences of a character in a string
 */
int 
countc(string, c)
	char *string, c;
{
	register int count;

	count = 0;

	while (*string)
		if (*string++ == c)
			count++;

	return (count);
}

/*
 * concatenate strings, handling escaped newlines and tabs
 */
int 
strcate(to, from)
	char *to, *from;
{
	int	escaped;
	char	*orig;

	while (*to)
		to++;
	orig = to;

	for (escaped = 0; *from; from++)
	{
		if (escaped)
		{
			switch (*from)
			{
			case 'n':
				*to++ = '\n';
				break;
			case 't':
				*to++ = '\t';
				break;
			default:
				*to++ = *from;
				break;
			}
			escaped = 0;
		} else if (!(escaped = (*from == '\\')))
			*to++ = *from;
	}
	*to = '\0';
	return (to - orig);
}

/*
 * Make a string of control chars legible
 */
char *
Visible(s, n)
	char *s;
	int n;
{
	static char scratch[4096];
	char	*Spot;
	unsigned char c;
	static char hexDigit[] =
	{'0', '1', '2', '3', '4', '5', '6', '7',
	 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	int	wasHex = 0;

	for (Spot = scratch; n--; s++)
	{
		c = *((unsigned char *) s);
		if (c > 127)
		{
			if (!wasHex)
				*Spot++ = '<';
			*Spot++ = hexDigit[(c >> 4) & 0xf];
			*Spot++ = hexDigit[c & 0xf];
			wasHex = 1;
		} else
		{
			if (wasHex)
				*Spot++ = '>';
			wasHex = 0;
			if (c == '^')
			{
				*Spot++ = '\\';
				*Spot++ = c;
			} else if (' ' <= c && c <= '~')
				*Spot++ = c;
			else if (c == 127)
			{
				*Spot++ = '^';
				*Spot++ = '?';
			} else
			{
				*Spot++ = '^';
				*Spot++ = c + '@';
			}
		}
	}
	if (wasHex)
		*Spot++ = '>';
	*Spot = 0;
	return (scratch);
}

/*
 * is a string all metacharacters?
 */
int 
AllMeta(s)
	char *s;
{
	for (; *s; s++)
		if (!any(*s, "[]?*"))
			return (0);
	return (1);
}

/*
 * copy a string, converting to lower case
 */
char *
strlcpy(to, from)
	char *to, *from;
{
	char	*save = to;

	while (*to++ = isupper(*from) ? tolower(*from) : *from)
		from++;

	return (save);
}


/*
 * Remove special characters from a string
 */
int 
RemoveSpecials(str)
	char *str;
{
	for (; *str; str++)
		if (strchr("()$&<>|;/`", *str))
			*str = ' ';
}
