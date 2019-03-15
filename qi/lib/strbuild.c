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
static char  RcsId[] = "@(#)$Id: strbuild.c,v 1.7 1994/03/12 00:06:47 paul Exp $";
#endif

#include "protos.h"

/*
 * These routines provide a reasonably efficient way of building up a
 * string of unknown length.  The string is kept in a buffer, and the end
 * of the current string is remembered so that it can be easily added on
 * to.  If the buffer runs out of space it is reallocated at twice its 
 * needed size.  Once the string has been built it should be copied, since
 * it will be destroyed by the next call to init_string().
 */

#define INITIAL_SIZE 128

static char *StrBuf;
static unsigned StrBufSize;
static unsigned StrBufUsed;

/*
 * Initialize the buffer to start building a new string.
 */
void 
init_string()
{
	if (StrBuf)
		free(StrBuf);
	StrBufSize = INITIAL_SIZE;
	StrBuf = malloc(StrBufSize);
	StrBuf[0] = '\0';
	StrBufUsed = 1;
}

/*
 * Add a new string onto the current buffer.	A pointer to the beginning *
 * of the buffer is returned.
 */
char *
add_string(str)
	char *str;
{
	int	strlength;

	/* Nothing to do */
	if (!str || !str[0])
		return StrBuf;

	/* Just so we don't have to count it twice */
	strlength = strlen(str);

	/* Make sure we have enough space in the buffer */
	if (StrBufUsed + strlength > StrBufSize)
	{
		StrBufSize += strlength;
		free(StrBuf);
		StrBuf = realloc(StrBuf, StrBufSize *= 2);
	}

	/* Add on the new string */
	(void) strcat(&StrBuf[StrBufUsed - 1], str);
	StrBufUsed += strlength;

	/* Return a pointer to the whole thing */
	return StrBuf;
}
