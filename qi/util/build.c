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
static char  RcsId[] = "@(#)$Id: build.c,v 1.14 1994/09/09 20:17:46 p-pomes Exp $";
#endif

#include <fcntl.h>
#include <sys/file.h>
#include "protos.h"

#define ESC   '\033'

extern QHEADER header;
extern IDX last_node;
extern NODE *node_buf;
extern LEAF_DES *leaf_des_buf;
extern int Quiet;		/* qi/qi.c */
static char *Me;		/* the name of this program */


main(argc, argv)
	int	argc;
	char	*argv[];

{
	char	bdx_file[100];
	char	idx_file[100];
	char	seq_file[100];
	char	line[512];
	struct iindex buf;
	int	fd;
	int	pipe_fd1[2], pipe_fd2[2];
	int	seqflag = 0;
	IDX	hash_index;
	FILE	*to_sort, *from_sort;
	int	count = 0;
	char	*cp;

	/* when you're strange, no one remembers your name */
	Me = *argv;

	OP_VALUE(NOLOG_OP) = strdup("");
	while (--argc > 0 && **(++argv) == '-')
	{
		char *equal, **opt;

		(*argv)++;
		if (**argv == 's')
			seqflag++;
		else if (**argv == 'q')
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
		fprintf(stderr, "%s: building sequence file for database %s\n",
			Me, Database);
	sleep(5);
	sprintf(idx_file, "%s.idx", Database);
	sprintf(bdx_file, "%s.bdx", Database);
	sprintf(seq_file, "%s.seq", Database);

	DoSysLog(0);		/* report errors to stderr */

	if (seqflag)
	{			/* build a new .seq file */
		new_file(seq_file);
		chmod(seq_file, 0660);

		if ((fd = open(idx_file, O_RDONLY)) == -1)
		{
			perror(idx_file);
			exit(1);
		}
		/* set up the neccessary mechanism to talk to sort */
		pipe(pipe_fd1);
		pipe(pipe_fd2);

		if (fork() == 0)
		{
			dup2(pipe_fd1[0], 0);
			dup2(pipe_fd2[1], 1);

			close(pipe_fd1[1]);
			close(pipe_fd2[0]);

			execlp("sort", "sort", "-r", "+1", 0);
			perror("Execl in build");
			exit(1);
		}
		close(pipe_fd1[0]);
		close(pipe_fd2[1]);

		to_sort = fdopen(pipe_fd1[1], "w");
		from_sort = fdopen(pipe_fd2[0], "r");


		if (fork() == 0)
		{		/* read from .idx file and write to sort */
			fclose(from_sort);
			hash_index = 0;
			while (read(fd, (char *) &buf, sizeof (buf)) > 0)
			{
				if (buf.i_string[0] && buf.i_string[0] != EMPTY)
				{	/* a hit */
					if (buf.i_string[0] != ESC)
					{
						fprintf(to_sort, "%d %s\n", hash_index, buf.i_string);
						/* fprintf(stderr,"%d %s\n",hash_index,buf.i_string); */
					}
				}
				hash_index++;
				count++;
				if (!Quiet && count % 1000 == 1)
					fprintf(stderr, "%d (%s) to sort.\n", count,
						Visible(buf.i_string, strlen(buf.i_string)));
			}
			if (!Quiet)
				fprintf(stderr, "%d to sort\n", count);
			exit(0);
		} else
		{		/* read from sort and insert into .seq file */
			fclose(to_sort);

			while (fgets(line, sizeof line, from_sort))
			{

				count++;
				if (!Quiet && count % 1000 == 1)
					fprintf(stderr, "%d from sort.\n", count);
				/* Get rid of the ending newline */
				if (cp = strchr(line, '\n'))
					*cp = '\0';

				/* Get the hash table index */
				hash_index = atoi(line);

				/* Skip to the key and inset it into the seq set */
				if (cp = strchr(line, ' '))
				{
					strcpy(buf.i_string, cp + 1);
					/*
					 * fprintf( stderr, "%d %s\n", hash_index, buf.i_string );
					 */
					insert(buf.i_string, hash_index);
				}
			}
			if (!Quiet)
				fprintf(stderr, "%d from sort\n", count);
		}

		close(fd);
		put_tree_head();
	}
	bintree_init(Database);
	build_leaf_descriptors();
	header.index_root = build_tree((INT32) 1, header.last_leaf);
	printf("Tree built, %ld nodes\n", last_node);

	write_index(bdx_file);

	put_tree_head();
	exit(0);
}
