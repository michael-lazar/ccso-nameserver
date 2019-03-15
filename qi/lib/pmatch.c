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
static char  RcsId[] = "@(#)$Id: pmatch.c,v 1.10 1994/03/12 00:06:47 paul Exp $";
#endif

#include "protos.h"

/*LINTLIBRARY*/

static int meta_found = 0;
static int brkt_err = 0;
static int star_match __P((char *, char *));

int
pmatch(str, pat)
	char *str, *pat;
{
	register int range_cc, str_cc, in_range;
	int	c, low_lim;
	int	answer;

	str_cc = isupper(*str) ? tolower(*str) : *str;
	low_lim = 077777;
	switch (c = isupper(*pat) ? tolower(*pat) : *pat)
	{
	    case '[':
		in_range = 0;
		meta_found = 1;
		while (range_cc = *++pat)
		{
			if (range_cc == ']')
			{
				if (in_range)
					answer = pmatch(++str, ++pat);
				else
					answer = NO_MATCH;
				break;
			} else if (range_cc == '-')
			{
				if (low_lim <= str_cc & str_cc <= pat[1])
					in_range++;
				if (pat[1] != ']')
					range_cc = pat[1];
			}
			if (str_cc == (low_lim = range_cc))
				in_range++;
		}
		if (range_cc != ']')
		{
			brkt_err = 1;
			answer = NO_MATCH;
		}
		break;

	    case '?':
		meta_found = 1;
		if (str_cc)
			answer = pmatch(++str, ++pat);
		else
			answer = CONTINUE;
		break;

	    case '*':
		meta_found = 1;
		answer = star_match(str, ++pat);
		break;

	    case 0:
		if (!str_cc)
			answer = MATCH;
		else
			answer = NO_MATCH;
		break;
	default:
		if (str_cc == c)
			answer = pmatch(++str, ++pat);
		else if (str_cc < c)
			answer = CONTINUE;
		else
			answer = NO_MATCH;
		break;
	}

	if (answer != MATCH)
		answer = meta_found ? CONTINUE : answer;
	meta_found = 0;
	brkt_err = 0;
	return answer;
}

static int
star_match(str, pat)
	char *str, *pat;
{
	if (*pat == 0)
		return (MATCH);
	while (*str)
		if (pmatch(str++, pat) == MATCH)
			return (MATCH);
	return brkt_err ? NO_MATCH : CONTINUE;
}
