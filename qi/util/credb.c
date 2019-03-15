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
static char  RcsId[] = "@(#)$Id: credb.c,v 1.8 1994/03/12 00:59:25 paul Exp $";
#endif

/*
 * This program creates the files associated with a database. if the name of
 * the database is given, then all files are made. If a specific file is
 * named then that file only is made. A database name can't contain a ".".
 * The size is mandatory and is used to build the .idx file.
 */

#include "protos.h"

#define DIR 1
#define INDEX 2

static char *Me;		/* the name of this program */

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	fd, fsize, ftype;
	char	zero = 0;
	char	*dotptr;
	char	tmp[100];
	struct dirhead Dirhead;

	/* when you're strange, no one remembers your name */
	Me = *argv;

	while (--argc > 0 && **(++argv) == '-')
	{
		char *equal, **opt;

		(*argv)++;
		if (equal = strchr(*argv, '='))
		{
			*equal++ = '\0';
			for (opt = Strings; *opt; opt += 2)
				if (!strcmp(opt[0], *argv))
				{
					opt[1] = equal;
					break;
				}
			if (*opt == '\0')
			{
				fprintf(stderr, "%s: %s: unknown string.\n",
					Me, *argv);
				exit(1);
			}
		} else
		{
			fprintf(stderr, "%s: %s: unknown option.\n", Me, *argv);
			exit(1);
		}
	}
	if (argc < 1 || (fsize = atoi(*argv)) == 0)
	{
		printf("usage: %s size [filename[.(idx|dir)]]\n", Me);
		exit(1);
	}
	fsize *= (sizeof (struct iindex));
	argc--; argv++;

	Database = (argc > 0) ? *argv : DATABASE;
	fprintf(stderr, "%s: creating database %s\n", Me, Database);
	sleep(5);
	if (!(dotptr = strrchr(Database, '.')))	/* create all */
		ftype = INDEX | DIR;
	else if (!strcmp(dotptr + 1, "dir"))
	{
		ftype = DIR;
		*dotptr = '\0';
	} else if (!strcmp(dotptr + 1, "idx"))
	{
		ftype = INDEX;
		*dotptr = '\0';
	}
	if (ftype & INDEX)
	{			/* make index file ,uses size */
		if (!strchr(Database, '.'))
			sprintf(tmp, "%s.idx", Database);
		else
			strcpy(tmp, Database);
		if (!(fd = creat(tmp, 0664)))
		{
			perror(tmp);
			exit(1);
		}

		if (lseek(fd, fsize - 1, 0) < 0)
		{
			perror(tmp);
			exit(1);
		}
		if (write(fd, &zero, 1) < 0)
		{
			perror(tmp);
			exit(1);
		}
		printf("%s : %d bytes\n", tmp, fsize);
		close(fd);

		/* create overflow file */
		sprintf(tmp, "%s.iov", Database);
		if (!(fd = creat(tmp, 0664)))
		{
			perror(tmp);
			exit(1);
		}
		close(fd);
	}
	if (ftype & DIR)
	{
		if (!strchr(Database, '.'))
			sprintf(tmp, "%s.dir", Database);
		else
			strcpy(tmp, Database);
		if (!(fd = creat(tmp, 0664)))
		{
			perror(tmp);
			exit(1);
		}
		Dirhead.nents = 0;
		Dirhead.next_id = 1;
		Dirhead.nfree = 0;
		if (lseek(fd, 0, 0) < 0)
		{
			perror(tmp);
			exit(1);
		}
		if (write(fd, &Dirhead, sizeof (Dirhead)) < 0)
		{
			perror(tmp);
			exit(1);
		}
		fsize = (sizeof (DREC));
		if (lseek(fd, fsize - 1, 0) < 0)
		{
			perror(tmp);
			exit(1);
		}
		if (write(fd, &zero, 1) < 0)
		{
			perror(tmp);
			exit(1);
		}
		printf("%s : %d bytes\n", tmp, fsize);
		close(fd);

		/* create overflow file */
		sprintf(tmp, "%s.dov", Database);
		if (!(fd = creat(tmp, 0664)))
		{
			perror(tmp);
			exit(1);
		}
		close(fd);
	}
	exit(0);
}
