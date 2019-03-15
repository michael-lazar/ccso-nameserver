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
static char  RcsId[] = "@(#)$Id: phoneme.c,v 1.13 1994/03/12 00:24:45 paul Exp $";
#endif

#include "protos.h"

/*
 *  English to Phoneme translation.
 *
 *  Rules are made up of four parts:
 *  
 *	The left context.
 *	The text to match.
 *	The right context.
 *	The phonemes to substitute for the matched text.
 *
 *  Procedure:
 *
 *	Seperate each block of letters (apostrophes included) 
 *	and add a space on each side.  For each unmatched 
 *	letter in the word, look through the rules where the 
 *	text to match starts with the letter in the word.  If 
 *	the text to match is found and the right and left 
 *	context patterns also match, output the phonemes for 
 *	that rule and skip to the next unmatched letter.
 *
 *
 *  Special Context Symbols:
 *
 *	#   One or more vowels
 *	:   Zero or more consonants
 *	^   One consonant.
 *	.   One of B, D, V, G, J, L, M, N, R, W or Z (voiced 
 *	    consonants)
 *	%   One of ER, E, ES, ED, ING, ELY (a suffix)
 *	    (Right context only)
 *	+   One of E, I or Y (a "front" vowel)
 */

#ifndef TRUE
#define FALSE (0)
#define TRUE (!0)
#endif

typedef char *Rule[4];		/* A rule is four character pointers */

extern Rule *rules[];		/* An array of pointers to rules */

static char *xlate_word __P((char *));
static int find_rule __P((char *, int, Rule *, char **));
static int leftmatch __P((char *, char *));
static int rightmatch __P((char *, char *));

#define isvowel(chr)  \
    (chr=='A' || chr=='E' || chr=='I' || chr=='Y' || chr=='O' || chr=='U')

#define isconsonant(chr)    (islower(chr) && !isvowel(chr))

char *
phonemify(word)
	char *word;
{
	char	scratch[80];
	register char *cp;

	*scratch = ' ';
	for (cp = scratch + 1; *word; word++)
	{
		if (isupper(*word))
			*word = tolower(*word);

		if (!isconsonant(*word) || *word != *(cp - 1))
			*cp++ = *word;
	}
	*cp++ = ' ';
	*cp = '\0';
	return (xlate_word(scratch));
}

static char *
xlate_word(word)
	char word[];
{
	int	indx;		/* Current position in word */
	int	type;		/* First letter of match part */
	static char phonetics[1024];
	char	*phoneme;

	phonetics[0] = 0;
	indx = 1;		/* Skip the initial blank */
	do
	{
		if (islower(word[indx]))
			type = word[indx] - 'a' + 1;
		else
			type = 0;

		indx = find_rule(word, indx, rules[type], &phoneme);
		if (phoneme)
			(void) strcat(phonetics, phoneme);
	}
	while (word[indx] != '\0');
	return (phonetics);
}

static int 
find_rule(word, indx, rules, phoneme)
	char word[];
	int indx;
	Rule *rules;
	char **phoneme;
{
	Rule	*rule;
	char	*left, *match, *right;
	int	remainder;

	*phoneme = NULL;

	for (;;)		/* Search for the rule */
	{
		rule = rules++;
		match = (*rule)[1];

		if (match == 0) /* bad symbol! */
		{
			/* fprintf(stderr, */
			/*
			 * "Error: Can't find rule for: '%c' in \"%s\"\n", word[indx],
			 * word);
			 */
			return indx + 1;	/* Skip it! */
		}
		for (remainder = indx; *match != '\0'; match++, remainder++)
		{
			if (*match != word[remainder])
				break;
		}

		if (*match != '\0')	/* found missmatch */
			continue;
		/*
		 * printf("\nWord: \"%s\", Index:%4d, Trying: \"%s/%s/%s\" =
		 * \"%s\"\n",
		 */
		/* word, indx, (*rule)[0], (*rule)[1], (*rule)[2], (*rule)[3]); */
		left = (*rule)[0];
		right = (*rule)[2];

		if (!leftmatch(left, &word[indx - 1]))
			continue;
		/*
		 * printf("leftmatch(\"%s\",\"...%c\") succeded!\n", left,
		 * word[indx-1]);
		 */
		if (!rightmatch(right, &word[remainder]))
			continue;
		/*
		 * printf("rightmatch(\"%s\",\"%s\") succeded!\n", right,
		 * &word[remainder]);
		 */
		*phoneme = (*rule)[3];
		/*
		 * printf("Success: ");
		 */
		/* outstring(output); */
		return remainder;
	}
}


static int 
leftmatch(pattern, context)
	char *pattern, *context;
{
	char	*pat;
	char	*text;
	int	count;

	if (*pattern == '\0')	/* null string matches any context */
	{
		return TRUE;
	}
	/* point to last character in pattern string */
	count = strlen(pattern);
	pat = pattern + (count - 1);

	text = context;

	for (; count > 0; pat--, count--)
	{
		/* First check for simple text or space */
		if (isalpha(*pat) || *pat == '\'' || *pat == ' ')
			if (*pat != *text)
				return FALSE;
			else
			{
				text--;
				continue;
			}
		switch (*pat)
		{
		case '#':	/* One or more vowels */
			if (!isvowel(*text))
				return FALSE;

			text--;

			while (isvowel(*text))
				text--;
			break;

		case ':':	/* Zero or more consonants */
			while (isconsonant(*text))
				text--;
			break;

		case '^':	/* One consonant */
			if (!isconsonant(*text))
				return FALSE;
			text--;
			break;

		case '.':	/* B, D, V, G, J, L, M, N, R, W, Z */
			if (*text != 'B' && *text != 'D' && *text != 'V'
			    && *text != 'G' && *text != 'J' && *text != 'L'
			    && *text != 'M' && *text != 'N' && *text != 'R'
			    && *text != 'W' && *text != 'Z')
				return FALSE;
			text--;
			break;

		case '+':	/* E, I or Y (front vowel) */
			if (*text != 'E' && *text != 'I' && *text != 'Y')
				return FALSE;
			text--;
			break;

		case '%':
		default:
			/* fprintf(stderr, "Bad char in left rule: '%c'\n", *pat); */
			return FALSE;
		}
	}

	return TRUE;
}


static int 
rightmatch(pattern, context)
	char *pattern, *context;
{
	char	*pat;
	char	*text;

	if (*pattern == '\0')	/* null string matches any context */
		return TRUE;

	pat = pattern;
	text = context;

	for (pat = pattern; *pat != '\0'; pat++)
	{
		/* First check for simple text or space */
		if (isalpha(*pat) || *pat == '\'' || *pat == ' ')
			if (*pat != *text)
				return FALSE;
			else
			{
				text++;
				continue;
			}
		switch (*pat)
		{
		case '#':	/* One or more vowels */
			if (!isvowel(*text))
				return FALSE;

			text++;

			while (isvowel(*text))
				text++;
			break;

		case ':':	/* Zero or more consonants */
			while (isconsonant(*text))
				text++;
			break;

		case '^':	/* One consonant */
			if (!isconsonant(*text))
				return FALSE;
			text++;
			break;

		case '.':	/* B, D, V, G, J, L, M, N, R, W, Z */
			if (*text != 'B' && *text != 'D' && *text != 'V'
			    && *text != 'G' && *text != 'J' && *text != 'L'
			    && *text != 'M' && *text != 'N' && *text != 'R'
			    && *text != 'W' && *text != 'Z')
				return FALSE;
			text++;
			break;

		case '+':	/* E, I or Y (front vowel) */
			if (*text != 'E' && *text != 'I' && *text != 'Y')
				return FALSE;
			text++;
			break;

		case '%':	/* ER, E, ES, ED, ING, ELY (a suffix) */
			if (*text == 'E')
			{
				text++;
				if (*text == 'L')
				{
					text++;
					if (*text == 'Y')
					{
						text++;
						break;
					} else
					{
						text--; /* Don't gobble L */
						break;
					}
				} else if (*text == 'R' || *text == 'S'
					   || *text == 'D')
					text++;
				break;
			} else if (*text == 'I')
			{
				text++;
				if (*text == 'N')
				{
					text++;
					if (*text == 'G')
					{
						text++;
						break;
					}
				}
				return FALSE;
			} else
				return FALSE;

		default:
			/* fprintf(stderr, "Bad char in right rule:'%c'\n", *pat); */
			return FALSE;
		}
	}

	return TRUE;
}
