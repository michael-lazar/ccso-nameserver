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
static char  RcsId[] = "@(#)$Id: lookup.c,v 1.22 1995/04/11 16:48:26 p-pomes Exp $";
#endif

#include "protos.h"

/*
 * do_lookup takes an array of pointers to strings and an array of unsigned
 * INT32 and returns an array of long which represent indicies into
 * the directory of entries that contain all the strings given and is a
 * subset of the array given. No array (empty) means return all matches.
 *
 * It always returns an array.
 */

#define ARY_FREE(a)	if(a){free((char*)a);a=NULL;}

INT32 *
do_lookup(strings, ary)
	char **strings;
	INT32 *ary;
{
	INT32	*new_ary, *old_ary;

	while (*strings)
	{
#ifdef LIMIT_SEARCH
		if (all_metas(*strings))
		{		/*ignore all words with all metas */
			*strings++ = (char *) NIL;
			continue;
		}
#endif /*LIMIT_SEARCH*/

		if (!quoted(*strings) && anyof(*strings, METAS))
			new_ary = find_all(*strings++);
		else
			new_ary = findstr(*strings++);


		if (!new_ary)
		{		/*no entry for this string */
			if (!ary)
				ary = (INT32 *) malloc(WORDSIZE);
			ary[0] = 0;
			break;
		}
		if (!ary)
		{		/*first time through, with no array given */
			ary = new_ary;
			continue;
		}
		ary = intersect(old_ary = ary, new_ary);

		ARY_FREE(new_ary);
		ARY_FREE(old_ary);

		if (length(ary) == 0)	/*no matches */
			break;
	}
	return (ary);
}

/*
 * make_lookup puts all of the words in "str" into the index with a pointer
 * to the directory entry "ent"
 */

void 
make_lookup(str, ent)
	char *str;
	int ent;
{
	char	buf[MAX_LEN];
	char	*cp;

	(void) strncpy(buf, str, MAX_LEN-1);
	for (cp = strtok(buf, IDX_DELIM); cp; cp = strtok(0, IDX_DELIM))
	{
		if (!putstr(cp, ent))
			IssueMessage(LOG_ERR, "make_lookup: putstr failed (%s:%d).", cp, ent);
	}
}

/*
 * remove pointers to entries from the index for words in str
 */
void 
unmake_lookup(str, ent)
	char *str;
	int ent;
{
	char	buf[MAX_LEN];
	char	*cp;

	(void) strncpy(buf, str, MAX_LEN-1);
	for (cp = strtok(buf, IDX_DELIM); cp; cp = strtok(0, IDX_DELIM))
	{
		if (strlen(cp) > 1)
			if (deletestr(cp, ent) == 0)
				IssueMessage(LOG_ERR, "unmake_lookup: deletestr failed.");
	}
}

/*
 * make or unmake index entries for a directory entry
 */
MakeLookup(dirp, entry, func)
	QDIR	dirp;
	INT32	entry;
	int	(*func) ();

{
	FDESC	*fd;
	char	*value;
	char	*atSign;

	for (; *dirp; dirp++)
	{
		value = (*dirp);	/*value preceeded by number */
		fd = FindFDI(atoi(value));
		if (!fd)
			IssueMessage(LOG_DEBUG, "Unknown database field %d.\n", atoi(value));
		else
		{
			if (fd->fdIndexed)
			{
				if ((value = strchr(value, ':')))	/*strip number */
				{
					value++;
					if (fd->fdTurn && *value == '*') /* don't index the star */
					  if (*++value == '\0')
					    continue;
					switch (fd->fdId)
					{
					case F_EMAIL:
						(*func) (value, entry);
						if (atSign = strchr(value, '@'))
						{
							*atSign = 0;
							(*func) (value, entry);
							*atSign = '@';
						}
						break;
					default:
						(*func) (value, entry);
						break;
					}
				}
			}
		}
	}
}
