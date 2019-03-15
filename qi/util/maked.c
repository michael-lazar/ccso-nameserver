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
static char  RcsId[] = "@(#)$Id: maked.c,v 1.15 1994/03/12 22:31:18 paul Exp $";
#endif

#include "protos.h"

/*
 * maked -- make a dir file for the nameserver
 */

extern int Quiet;		/* qi/qi.c */
extern int IndicateAlways;	/* qi/query.c */

#define BUF_SIZE    8192
#define DIR_MAX	    80
static char *Me;		/* the name of this program */
int	lcount;

main(argc, argv)
	int	argc;
	char	**argv;

{
	char	buffer[BUF_SIZE];
	QDIR	dirp;

	/* when you're strange, no one remembers your name */
	Me = *argv;

	OP_VALUE(NOLOG_OP) = strdup("");
	IndicateAlways = 0;	/* somebody wants this */
	dirp = (char **) malloc(DIR_MAX * sizeof (char *));

	while (--argc > 0 && **(++argv) == '-')
	{
		char *equal, **opt;

		(*argv)++;
		if (**argv == 'q')
			Quiet++;
		else if (equal = strchr(*argv, '='))
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
	Database = (argc > 0) ? *argv : DATABASE;
	if (!Quiet)
		fprintf(stderr, "%s: building database %s\n", Me, Database);
	sleep(5);
	DoSysLog(0);		/* report errors to stderr */

	if (!dbd_init(Database))
	{
		fprintf(stderr, "Couldn't init %s.\n", Database);
		exit(2);
	}
	get_dir_head();

	lcount = 0;
	while (GetLine(buffer) != 0)
	{
		TurnIntoDir(buffer, dirp);

		if (!new_ent())
		{
			perror("new_ent failed");
			exit(1);
		}
		if (!putdata(dirp))
		{
			perror("Putdata");
			abort();
		}
		set_date(0);
		store_ent();
		if (!Quiet && ++lcount % 500 == 0)
			fprintf(stderr, "%d\r", lcount, dirp[0]);
	}
	if (!Quiet)
		putc('\n', stderr);
	put_dir_head();
	exit(0);
}

/*
 * get a line of input
 */
GetLine(line)
	char *line;
{
	register int ccount = 0;
	register int c;
	register int backslash = 0;
	char *sline = line;

	for (c = getchar(); c != EOF && c != '\n'; c = getchar())
	{
		if (backslash)
		{
			if (c == 'n')
				*line++ = '\n';
			else if (c == 't')
				*line++ = ' ';	/* cheating, I know... */
			else
				*line++ = c;
			ccount++;
			backslash = 0;
		} else if (!(backslash = (c == '\\')))
		{
			*line++ = c;
			ccount++;
		}
	}
	if (ccount > BUF_SIZE)
	{
		fprintf(stderr, "\nOh no--overflow!\n");
		fprintf(stderr, "line %d len %d\n", lcount+1, ccount);
		fprintf(stderr, "%s\n", sline);
		exit(4);
	}
	if (c == '\n')
		ccount++;
	*line = 0;
	return (ccount);
}

/*
 * turn an input line into a dir
 */
TurnIntoDir(line, dirp)
	char	*line;
	char   **dirp;
{
	char	*token;
	char   **origDir = dirp;

	for (token = strtok(line, "\t"); token; token = strtok(0, "\t"))
	{
		if (strchr(token, ':') == strrchr(token, ':') &&
		    token[strlen(token) - 1] == ':')
			continue;
		*dirp++ = token;
	}

	*dirp = 0;
	if (dirp - origDir > DIR_MAX)
	{
		fprintf(stderr, "Oh no--Dir overflow!\n");
		fprintf(stderr, "line %d\n", lcount+1);
		fprintf(stderr, "%s\n", line);
		exit(5);
	}
}
