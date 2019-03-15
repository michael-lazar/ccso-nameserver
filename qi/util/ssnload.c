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
static char  RcsId[] = "@(#)$Id: ssnload.c,v 1.6 1994/03/12 00:59:25 paul Exp $";
#endif

/*
 * replaces ssn fields with id fields, using id database
 */
#include "conf.h"
#include <stdio.h>
#include <ndbm.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>

#ifdef __STDC__
# include <stdlib.h>
# include <string.h>
#endif /* __STDC__ */

#define BUF_SIZE    1024

main(argc, argv)
	int argc;
	char **argv;
{
	char	buffer[BUF_SIZE];
	int	err;
	DBM	*db;
	datum	first, second;


	if (argc != 2)
	{
		fprintf(stderr, "Usage: ssnload ssndb\n");
		exit(1);
	}
	if ((db = dbm_open(argv[1], O_RDWR | O_CREAT, 0660)) == NULL)
	{
		perror(argv[1]);
		exit(3);
	}
	first.dptr = buffer;
	while (gets(buffer) != NULL)
	{
		second.dptr = strchr(buffer, ':');	/* find end of ssn */
		if (!second.dptr)
			continue;	/* skip */
		first.dsize = second.dptr - first.dptr;
		*second.dptr++ = '\0';
		first.dsize = second.dptr - first.dptr;
		second.dsize = strlen(second.dptr) + 1;
		if (err = dbm_store(db, first, second, DBM_INSERT))
			fprintf(stderr, "Dbm err %d on <%s:%s>\n", first.dptr, second.dptr);
#ifdef DEBUG
		else
			printf("%d %s:%d %s\n", first.dsize, first.dptr, second.dsize, second.dptr);
#endif
	}

	dbm_close(db);
	exit(0);
}
