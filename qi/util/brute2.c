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
static char  RcsId[] = "@(#)$Id: brute2.c,v 1.9 1994/03/12 00:59:25 paul Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include "protos.h"


int	pstrcmp __P((char *, char *));

#define SRT_SIZE 16384

main()
{
	int	size = 0;
	int	iRead;
	char	*buffer = malloc(SRT_SIZE);
	char	*spot;
	int	nLines = 0;
	char   **ptrs, **this;


	/*
	 * pass 1; read the file
	 */
	if (!buffer)
		perror("malloc");

	while ((iRead = read(0, buffer + size, SRT_SIZE)) > 0)
	{
		size += iRead;
		buffer = realloc(buffer, size + SRT_SIZE);
		if (!buffer)
			perror("realloc");
	}

	buffer = realloc(buffer, size + 1);
	if (!buffer)
		perror("realloc");
	buffer[size] = '\n';

	/*
	 * count the lines, and replace newlines with nulls
	 */
	for (spot = buffer; spot <= buffer + size; spot++)
		if (*spot == '\n')
		{
			nLines++;
			if (!(nLines % 500))
				fprintf(stderr, "reading %d\r", nLines);
			*spot = '\0';
		}
	nLines--;
	putc('\n', stderr);

	/*
	 * build an array of pointers to the lines
	 */
	if (nLines == 0)	/* nothing to do and malloc(0) often dies */
		exit(0);
	ptrs = (char **) malloc(nLines * sizeof (char *));

	if (!ptrs)
		perror("malloc");
	spot = buffer;
	for (this = ptrs; this < ptrs + nLines; this++)
	{
		*this = spot;
		while (*spot++) ;
	}

	/*
	 * sort them
	 */
	qsort(ptrs, nLines, sizeof (char *), pstrcmp);

	/*
	 * output them
	 */
	while (nLines--)
		puts(*ptrs++);
	exit(0);
}

int 
pstrcmp(p1, p2)
	char *p1, *p2;
{
	return (strcmp(*(char **) p1, *(char **) p2));
}
