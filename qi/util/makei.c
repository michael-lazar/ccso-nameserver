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
static char  RcsId[] = "@(#)$Id: makei.c,v 1.15 1994/09/09 20:17:46 p-pomes Exp $";
#endif

#include "protos.h"

/*
 * these thing keep ld happy
 */
int	InputType;
int	Daemon;
FILE	*Input, *Output;
char	*DBState;

/*
 * end of ld pacification
 */
extern int Quiet;		/* qi/qi.c */
extern int ReadOnly;
extern int DoTree;
int	CheckMeta;
static char *Me;		/* the name of this program */
extern int DoTree;
static int pipe_fd1[2], pipe_fd2[2];
static FILE *to_sort, *from_sort;
static void flush_key __P((char *,PTRTYPE *,PTRTYPE));
static int debug = 0;
extern void printarry __P((INT32 *));
extern int getient __P((register char *, struct iindex *));

main(argc, argv)
	int	argc;
	char   **argv;
{
	char inbuf[MAX_KEY_LEN+50];
	char curkey[MAX_KEY_LEN+1];
	PTRTYPE curmaxlen = NIPTRS, recidx = 0;
	PTRTYPE *reclist = (PTRTYPE *) calloc(curmaxlen,PTRSIZE);
	int count = 0, keys = 0;
#ifdef SA_FULLDUMP
	/* AIX does not provide the data section in a core dump by default. */
	struct sigaction handlr;

	handlr.sa_handler = NULL;
	handlr.sa_flags = SA_FULLDUMP;
	sigaction(SIGQUIT, &handlr, NULL);
	sigaction(SIGILL, &handlr, NULL);
	sigaction(SIGBUS, &handlr, NULL);
	sigaction(SIGSEGV, &handlr, NULL);
	sigaction(SIGIOT, &handlr, NULL);
#endif /* SA_FULLDUMP */

	/* when you're strange, no one remembers your name */
	Me = *argv;
	*curkey = '\0';

	OP_VALUE(NOLOG_OP) = strdup("");
	ReadOnly = 0;
	DoTree = 0;
	while (--argc > 0 && **(++argv) == '-')
	  {
		char *equal, **opt;

		(*argv)++;
		if (**argv == 'q')
			Quiet++;
		else if (equal = (char *)strchr(*argv, '='))
		{
			*equal++ = 0;
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
		printf("%s: indexing database %s\n", Me, Database);
	sleep(5);
	setbuf(stdout, NULL);

	DoSysLog(0);		/* report errors to stderr */

	dbd_init(Database);
	get_dir_head();
	if (!GetFieldConfig())
		exit(1);
	/* bintree_init(Database); *//* forget bintree here */
	DoTree = 0;
	print_head();

	/* set up the neccessary mechanism to talk to sort */
	pipe(pipe_fd1);
	pipe(pipe_fd2);

	if (fork() == 0)
	  {
		  dup2(pipe_fd1[0], 0);
		  dup2(pipe_fd2[1], 1);
		  
		  close(pipe_fd1[1]);
		  close(pipe_fd2[0]);
		  
		  execlp("sort", "sort", "-t\t", "+0", "-1", "+1n", 0);
		  perror("Execl in makei");
		  exit(1);
	  }
	close(pipe_fd1[0]);
	close(pipe_fd2[1]);
	
	to_sort = fdopen(pipe_fd1[1], "w");
	from_sort = fdopen(pipe_fd2[0], "r");
	
	
	if (fork() == 0)
	  {		/* read from .dir file and write to sort */
		fclose(from_sort);
		printf("sent indicies for %d dir entries to sort.\n", make_index());
		exit(0);
	  } else
	    {		/* read from sort and insert into .idx file */
		    fclose(to_sort);
		    
		    if (!dbi_init(Database)) {
			    fprintf(stderr,"%s: couldn't init\n",Database);
			    exit(1);
		    }
		    while (fgets(inbuf,sizeof(inbuf),from_sort)) {
			    char *rp = (char *)strchr(inbuf,'\t');
			    if (rp == NULL) {
				fprintf(stderr,"%s: no tab in =%s=\n",
				    Database, inbuf);
				exit(1);
			    }
			    *rp++ = '\0';
			    count++;
			    if (!Quiet && count % 1000 == 1)
			      printf("%d from sort.\n", count);
			    if (strcmp(inbuf,curkey)) {	/* new key */
				    flush_key(curkey,reclist,recidx); /* flush the old one */
				    recidx = 0;
				    reclist[0] = 0;
				    strncpy(curkey,inbuf,sizeof(curkey));
				    keys++;
			    }
			    if (recidx >= curmaxlen-1)
			      reclist = (PTRTYPE *) realloc(reclist, (curmaxlen += NOPTRS)*PTRSIZE);
			    reclist[recidx++] = atoi(rp);
		    }
		    flush_key(curkey,reclist,recidx); /* flush the final one */
		    if (!Quiet)
		      printf("%d from sort\n", count);
	    }
	printf("indexed %d unique strings out of %d total.\n",keys,count);
	
	exit(0);
}

extern int TrySum;
extern int TryTimes;
extern int WordsIndexed;
extern int MaxIdx;

make_index()
{
	QDIR	dirp;
	INT32	ent;
	extern struct dirhead DirHead;
	int	entries_done;
	void sort_lookup();


	entries_done = 0;

	for (ent = 1; ent < DirHead.nents; ent++)
	{
		if (!next_ent(ent))
		{
			/* printf("didn't do %d\n",ent); */
			continue;
		}
		getdata(&dirp);	/* setup entry */

		/* for all make the index entries */

		MakeLookup(dirp, ent, sort_lookup);
		if ((entries_done++ % 1000) == 0)
			printf("%d to sort\n", entries_done);
		FreeDir(&dirp);
	}
	return (ent);
}

print_head()
{
	extern struct dirhead DirHead;

	printf("nents = %d\n", DirHead.nents);
	printf("next_id = %d\n", DirHead.next_id);
}

void
sort_lookup(str,ent)
char *str;
int ent;
{
  char	buf[MAX_LEN];
  char	*cp;
  char *strlncpy();
#ifdef DEBUG
  static FILE *out = NULL;
  if (out == NULL) {
    if ((out = fopen("./debug.index","w")) == NULL) {
      perror("./debug.index");
      exit(1);
    }
  }
#endif  
  (void) strlncpy(buf, str, MAX_LEN);
  for (cp = strtok(buf, IDX_DELIM); cp; cp = strtok(0, IDX_DELIM)) {
    if (cp[1] != '\0') {	/* has to be at least 2 letters! */
      fprintf(to_sort,"%.*s\t%d\n", MAX_KEY_LEN, cp,ent);
#ifdef DEBUG
      fprintf(out,"%.*s\t%d\n",MAX_KEY_LEN,cp,ent);
#endif
    }
  }
}

char *
strlncpy(to, from, max)
char *to, *from;
int max;
{
  char *save = to;

  while ((max-- > 0) && (*to++ = isupper(*from) ? tolower(*from) : *from))
    from++;
  
  return (save);
}

/*
 * Flush the key and the entire list of records it is in.
 */

static void
flush_key(key,reclist,nelem)
char *key;
PTRTYPE *reclist, nelem;
{
  struct iindex x;
  INT32 i, j, iloc;

  if (!*key)
    return;
  reclist[nelem] = 0;		/* make sure it's zero-terminated */
  key[MAX_KEY_LEN] = '\0';	/* just in case it isn't */

  if (!putstrarry(key, reclist)) {
    fprintf(stderr,"putstrarry failed for key %s (%d elements)\n",key,nelem);
    return;
  }
}
