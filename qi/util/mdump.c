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
static char  RcsId[] = "@(#)$Id: mdump.c,v 1.29 1995/06/10 17:36:51 p-pomes Exp $";
#endif

#include "protos.h"

#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pwd.h>

int	Progress = 0;
static char *Me;		/* the name of this program */
extern int Quiet;		/* qi/qi.c */
extern char *Kdomain;		/* qi/qi.c */

#define LENGTH	80

struct dumptype
{
	char	*name;
	int	(*select) ();
	int	(*dump) ();
};

struct dumptype * FindDump __P((char *));
char * escapeChars __P((char *));

int     sel_student(), sel_other(), sel_rejects(), sel_pubpeople(),
	sel_units(), dump_extra(), dump_all(), dump_4merge(), sel_people(),
	sel_countpw(), dump_nothing(), sel_email(), sel_all(), sel_weather(),
	sel_outsider(), dump_kerb(), sel_kerb(), dump_netware(),
	dump_email(), dump_aiss(), dump_size(), dump_ruchi(), dump_emdir(),
	dump_food(), sel_food(), sel_time(), sel_maggie(), dump_maggie(),
	sel_proxy(), dump_proxy(), sel_files(), dump_files(),
	dump_date(), dump_generics(), dump_aliases();

struct dumptype Dumps[] =
{
	"people", sel_people, dump_4merge,
	"netware", sel_people, dump_netware,
	"other", sel_other, dump_all,
	"outsider", sel_outsider, dump_all,
	"student", sel_student, dump_email,
	"time", sel_time, dump_all,
	"rejects", sel_rejects, dump_all,
	"units", sel_units, dump_all,
	"count_pw", sel_countpw, dump_nothing,
	"count_email", sel_email, dump_nothing,
	"all", sel_all, dump_all,
	"email", sel_email, dump_email,
	"weather", sel_weather, dump_all,
	"generics", sel_email, dump_generics,
	"aiss", sel_email, dump_aiss,
	"ruchi", sel_pubpeople, dump_ruchi,
	"entry_sizes", sel_all, dump_size,
	"food", sel_food, dump_food,
	"proxy", sel_proxy, dump_proxy,
	"files", sel_files, dump_files,
	"date", sel_all, dump_date,
	"aliases", sel_all, dump_aliases,
	"kerberos", sel_kerb, dump_kerb,
	"email_dir", sel_email, dump_emdir,
	"maggie", sel_maggie, dump_maggie,
	0, 0, 0
};

main(argc, argv)
	int	argc;
	char   **argv;
{
	INT32	entry;
	QDIR	dirp;
	int	count;
	int	selected;
	extern struct dirhead DirHead;
	struct dumptype *dt;

	/* when you're strange, no one remembers your name */
	Me = *argv;

	while (--argc > 0 && **(++argv) == '-')
	{
		char *equal, **opt;

		(*argv)++;
		if (**argv == 'p')
			Progress = 1;
		else if (**argv == 'q')
			Quiet = 1;
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

	if ((dt = FindDump(*argv)) == NULL)
	{
		fprintf(stderr, "Usage: mdump [-p] [-q] <dump-type> [<database>]\n");
		fprintf(stderr, "available dump types are:\n");
		for (dt = Dumps; dt -> name; dt++)
			fprintf(stderr, " %s", dt -> name);
		putc('\n', stderr);
		exit(1);
	}
	argc--;
	argv++;
	Database = (argc > 0) ? *argv : DATABASE;
	if (!Quiet)
		fprintf(stderr, "%s: dumping database %s\n", Me, Database);
	DoSysLog(0);
	if (!GetFieldConfig())
		exit(1);
	if (!dbd_init(Database))
		exit(1);
	get_dir_head();
	selected = count = 0;
	for (entry = 1; entry < DirHead.nents; entry++)
	{
		if (Progress && !(entry % 100))
			fprintf(stderr, "%d/%d/%d\r", selected, entry, DirHead.nents);
		if (dnext_ent(entry) && !ent_dead())
		{
			count++;
			(void) getdata(&dirp);
			if ((*dt -> select) (dirp))
			{
				selected++;
				(*dt -> dump) (dirp);
			}
			MdumpFreeDir(&dirp);
		}
#ifdef DEBUG
		if (entry % 500 == 0)
			fprintf(stderr, " %d/%d\r", entry, DirHead.nents);
#endif
	}
	fflush(stdout);
	if (!Quiet)
		fprintf(stderr, "Database %s: %d of %d selected, %d errors.\n",
			Database, selected, count, entry - 1 - count);
	exit(0);
}

/*
 * figure out if a named dump exists
 */
struct dumptype *
FindDump(name)
	char	*name;
{
	struct dumptype *dt;

	for (dt = Dumps; dt -> name; dt++)
		if (!strcmp(dt -> name, name))
			return (dt);
	return (NULL);
}

/*
 * anybody with an id
 */
sel_id(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_UNIVID));
}

/*
 * anybody with a proxy
 */
sel_proxy(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_PROXY));
}

/*
 *
 */
dump_proxy(dirp)
	char   **dirp;
{
	if (*FINDVALUE(dirp, F_ALIAS))
		printf("change alias=%s make proxy=\"%s\"\n",
		       FINDVALUE(dirp, F_ALIAS), escapeChars(FINDVALUE(dirp, F_PROXY)));
	else
		printf("change %s make proxy=\"%s\"\n",
		       FINDVALUE(dirp, F_NAME), escapeChars(FINDVALUE(dirp, F_PROXY)));
}

/*
 * units (campus unit listing)
 */
sel_units(dirp)
	char   **dirp;
{
	return (strstr(FINDVALUE(dirp, F_TYPE), "unit") != NULL);
}

/*
 * timetable
 */
sel_time(dirp)
	char   **dirp;
{
	return (strstr(FINDVALUE(dirp, F_TYPE), "timetable") != NULL);
}

/*
 * people - anyone with type "person"
 */
sel_people(dirp)
	char   **dirp;
{
	return (strstr(FINDVALUE(dirp, F_TYPE), "person") != NULL);
}

/*
 * outsiders
 */
sel_outsider(dirp)
	char   **dirp;
{
	return (strstr(FINDVALUE(dirp, F_TYPE), "outsider") != NULL);
}

/*
 * students
 */
sel_student(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_CURRICULUM)) ;
}

/*
 * Kerberos (people and outsiders)
 */
sel_kerb(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_ALIAS) && *FINDVALUE(dirp, F_PASSWORD));
}

sel_pubpeople(dirp)
	char   **dirp;
{
	return (sel_people(dirp) && !*FINDVALUE(dirp, F_SUPPRESS));
}

sel_other(dirp)
	char   **dirp;
{
	return (!sel_people(dirp) && !sel_time(dirp));
}

sel_rejects(dirp)
	char   **dirp;
{
	char	*type;

	if (sel_people(dirp))
		return (0);
	if (sel_other(dirp))
		return (0);
	type = FINDVALUE(dirp, F_TYPE);
	if (strstr(type, "food") != NULL)
		return (0);
	if (strstr(type, "timetable") != NULL)
		return (0);
	return (1);
}

/*
 *
 */
sel_email(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_UNIVID) && *FINDVALUE(dirp, F_ALIAS));
}

/*
 *
 */
sel_food(dirp)
	char   **dirp;
{
	return (strstr(FINDVALUE(dirp, F_TYPE), "food") != NULL);
}

/*
 *
 */
sel_weather(dirp)
	char   **dirp;
{
	return (strstr(FINDVALUE(dirp, F_TYPE), "weather") != NULL);
}

dump_netware(dirp)
	char   **dirp;
{
	printf("\"%s\",\"%s\",\"%s\"\r\n", FINDVALUE(dirp, F_ALIAS),
		FINDVALUE(dirp, F_NAME), FINDVALUE(dirp, F_CURRICULUM));
}

dump_food(dirp)
	char   **dirp;
{
	printf("delete %s\n", FINDVALUE(dirp, F_NAME));
}

/*
 *
 */
sel_countpw(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_PASSWORD));
}

/*
 *
 */
sel_idpw(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, F_UNIVID) || *FINDVALUE(dirp, F_PASSWORD));
}

/*
 *
 */
sel_maggie(dirp)
	char   **dirp;
{
	return (sel_people(dirp) && *FINDVALUE(dirp, F_EMAIL));
}

dump_maggie(dirp)
	char   **dirp;
{
	char	scratch[300];
	char	*space;
	char	*paper = FINDVALUE(dirp, F_PAPER);
	char	*id = FINDVALUE(dirp, F_UNIVID);
	char	**opt;
	static char *maild = NULL;

	/*
	 * if paper field is "no", we omit the user
	 */
	if (!strcmp(paper, "no"))
	{
		printf("%d:%s\t%d:!omit!\n", F_UNIVID, id, F_EMAIL);
		return;
	}
	/*
	 * if paper field is specific, we grab the first specified address
	 * if it's <50 characters long, we print it.  Otherwise, we fall
	 * through and print the alias-based address, since > 50 chars is
	 * unprintable
	 */
	else if (!strcmp(paper, "specific"))
	{
		strcpy(scratch, FINDVALUE(dirp, F_EMAIL));
		if (space = strtok(scratch, " \n\t,"))
		{
			if (strlen(space) <= 50)
			{
				printf("%d:%s\t%d:%s\n", F_UNIVID, id, F_EMAIL, space);
				return;
			}
		}
	}
	/*
	 * print the alias-based address
	 */
	if (maild == NULL)
	{
		for (opt = Strings; *opt; opt += 2)
			if (!strcmp(opt[0], "MAILDOMAIN"))
				maild = opt[1];
		if (maild == NULL)
		{
			fprintf(stderr, "Couldn't determine MAILDOMAIN.\n");
			exit(1);
		}
	}
	printf("%d:%s\t%d:%s@%s\n", F_UNIVID, id,
	       F_EMAIL, FINDVALUE(dirp, F_ALIAS), maild);
}

/*
 *
 */
/*ARGSUSED*/
dump_nothing(dirp)
	char   **dirp;
{
	return;
}

/*
 *
 */
dump_aiss(dirp)
	char   **dirp;
{
	int	len;
	char	name[51], email[30];

	name[sizeof (name) - 1] = email[sizeof (email) - 1] = '\0';
	strncpy(name, FINDVALUE(dirp, F_NAME), sizeof (name) - 1);
	strncpy(email, FINDVALUE(dirp, F_EMAIL), sizeof (email) - 1);
	printf("%-*s %*s\n", sizeof (name) - 1, name, sizeof (email) - 1, email);
}

/*
 * Generic addresses for use in sendmail v8.6.9 generics table
 */
dump_generics(dirp)
	char   **dirp;
{
	register char *email = FINDVALUE(dirp, F_EMAIL);
	register char *name = FINDVALUE(dirp, F_NAME);
	register char *alias = FINDVALUE(dirp, F_ALIAS);
	register char *pnt;

	for (; *email && isspace(*email); email++)
		;
	for (pnt = email; *pnt && !isspace(*pnt) && *pnt != ','; pnt++)
		;
	*pnt = '\0';
#ifdef MAILDOMAIN
	printf("%s\t%s <%s@%s>\n", email, name, alias, MAILDOMAIN);
#else
	printf("%s\t%s <%s>\n", email, name, alias);
#endif
}


/*
 *
 */
/*ARGSUSED*/
sel_all(dirp)
	char   **dirp;
{
	return (1);
}

dump_aliases(dirp)
	char   **dirp;
{
	register char *alias = FINDVALUE(dirp, F_ALIAS);

	if (*alias)
		printf("%d:%s\n", F_ALIAS, alias);
}

dump_email(dirp)
	char   **dirp;
{
	register char *email = FINDVALUE(dirp, F_EMAIL);
	register char *pnt;

	for (; email && *email && isspace(*email); email++)
		;
	for (pnt = email; pnt && *pnt && !isspace(*pnt) && *pnt != ','; pnt++)
		;
	if (pnt && *pnt)
		*pnt = '\0';
	if (!email || *email == '\0')
		email = "(no account)";
	printf("%s\t%-8s\t%s\n", FINDVALUE(dirp, F_UNIVID),
		FINDVALUE(dirp, F_ALIAS), email);
}

/*
 *
 */
dump_ruchi(dirp)
	char   **dirp;
{
#define PUT_OUT(num) printf("$#$%s",escapeChars(FINDVALUE(dirp,num)))
	PUT_OUT(F_UNIVID);
	PUT_OUT(F_NAME);
	PUT_OUT(F_ALIAS);
	PUT_OUT(F_EMAIL);
	PUT_OUT(F_PHONE);
	PUT_OUT(F_ADDRESS);
	PUT_OUT(F_DEPARTMENT);
	PUT_OUT(F_TITLE);
	PUT_OUT(19);		/* office location */
	PUT_OUT(20);		/* home address */
	PUT_OUT(23);		/* nickname */
	PUT_OUT(22);		/* office address */
	PUT_OUT(32);		/* office phone */
	PUT_OUT(33);		/* home phone */
	putchar('\n');
}

/*
 *
 */
dump_emdir(dirp)
	char   **dirp;
{
	static char *maild = NULL;
	char	**opt;

	if (maild == NULL)
	{
		for (opt = Strings; *opt; opt += 2)
			if (!strcmp(opt[0], "MAILDOMAIN"))
				maild = opt[1];
		if (maild == NULL)
		{
			fprintf(stderr, "Couldn't determine MAILDOMAIN.\n");
			exit(1);
		}
	}
	if (!*FINDVALUE(dirp, F_CURRICULUM) && *FINDVALUE(dirp, F_EMAIL))
		printf("%-25s %s@%s\n", FINDVALUE(dirp, F_NAME),
			FINDVALUE(dirp, F_ALIAS), maild);
}

/*
 *
 */
dump_size(dirp)
	char   **dirp;
{
	int	size = 0;
	char   **sp = dirp;

	for (sp = dirp; *sp; sp++)
	{
		size += strlen(*sp);
		size++;
	}
	printf("%d\n", size);
}

/*
 *
 */
dump_all(dirp)
	char   **dirp;
{
	char	*colon;
	int	f;
	FDESC	*fd;

	printf("%d:%s", F_UNIVID, escapeChars(FINDVALUE(dirp, F_UNIVID)));
	for (; *dirp; dirp++)
	{
		f = atoi(*dirp);
		colon = (char *)strchr(*dirp, ':');
		if (colon == *dirp)
			fprintf(stderr, "\"%s\" lacks key =%s=\n",
				FINDVALUE(dirp, F_NAME), colon);
		else if (f != F_UNIVID && colon && colon[1])
		{
			if (fd = FindFDI(f))
			{
				putchar('\t');
				fputs(escapeChars(*dirp), stdout);
			} else
				fprintf(stderr, "\"%s\" unknown field %d\n",
					FINDVALUE(dirp, F_NAME), f);
		}
	}
	putchar('\n');
}

/*
 *
 */
dump_4merge(dirp)
	char   **dirp;
{
	char	*colon;
	int	f;
	FDESC	*fd;

	printf("%d:%s", F_UNIVID, escapeChars(FINDVALUE(dirp, F_UNIVID)));
	for (; *dirp; dirp++)
	{
		f = atoi(*dirp);
		colon = (char *)strchr(*dirp, ':');
		if (colon == *dirp)
			fprintf(stderr, "\"%s\" lacks key =%s=\n",
				FINDVALUE(dirp, F_NAME), colon);
		else if (f != F_UNIVID && colon && colon[1])
		{
			if (fd = FindFDI(f))
			{
				if (*fd -> fdMerge)
				{
					putchar('\t');
					fputs(escapeChars(*dirp), stdout);
				}
			} else
				fprintf(stderr, "\"%s\" unknown field %d\n",
					FINDVALUE(dirp, F_NAME), f);
		}
	}
	putchar('\n');
}

/*
 * replace tabs and newlines with escaped equivalents
 */
char	*
escapeChars(s)
	char	*s;
{
	register char *cp;
	static char value[8192];
	register char *vp;

	vp = value;
	for (cp = s; *cp; cp++)
		if (*cp == '\t')
		{
			*vp++ = '\\';
			*vp++ = 't';
		} else if (*cp == '\n')
		{
			*vp++ = '\\';
			*vp++ = 'n';
		} else if (*cp == '\\')
		{
			*vp++ = '\\';
			*vp++ = '\\';
		} else
			*vp++ = *cp;
	*vp = '\0';
	return (value);
}

/*
 * Free a dir structure (modified for mdump)
 */
MdumpFreeDir(dir)
	QDIR *dir;
{
	char   **p;

	if (*dir)
	{
		for (p = *dir; *p; p++)
			free(*p);
		/*free(*dir);
		*dir = 0;*/
	}
}

sel_files(dirp)
	char   **dirp;
{
	return (*FINDVALUE(dirp, 104));
}

dump_files(dirp)
	char   **dirp;
{
	char	*token;

	for (token = strtok(FINDVALUE(dirp, 104), " \n\t,"); token; token = strtok(NULL, " \t\n,"))
		puts(token);
}

dump_date(dirp)
	char   **dirp;
{
	printf("%ld %s\n", CurrentDate(), FINDVALUE(dirp, F_ALIAS));
}

dump_kerb(dirp)
	char   **dirp;
{
	char *alias = FINDVALUE(dirp, F_ALIAS);
	char *pass = FINDVALUE(dirp, F_PASSWORD);
	char **opt, *cp;

	if (Kdomain == NULL)
	{
		for (opt = Strings; *opt; opt += 2)
			if (!strcmp(opt[0], "KDOMAIN"))
				Kdomain = strdup(opt[1]);
		for (opt = Strings; Kdomain == NULL && *opt; opt += 2)
			if (!strcmp(opt[0], "MAILDOMAIN"))
				Kdomain = strdup(opt[1]);
		if (Kdomain == NULL)
		{
			fprintf(stderr, "Couldn't determine KDOMAIN or MAILDOMAIN.\n");
			exit(1);
		}
		for (cp = Kdomain; *cp; cp++)
			if (islower(*cp))
				*cp = toupper(*cp);
	}
	
	if (strlen(alias) > 8 || *pass == '\0')
		return;
	printf("av4k %s@%s\n%s\n", alias, Kdomain, pass);
}
