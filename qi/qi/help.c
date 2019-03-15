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
static char  RcsId[] = "@(#)$Id: help.c,v 1.14 1995/06/27 02:20:49 p-pomes Exp $";
#endif

#include "protos.h"

#include <dirent.h>
#include <sys/param.h>

static int HelpCopy __P((int, FILE *));
static int HelpText __P((int, char *, char *));
static int ListTopics __P((int, char *));

/*
 * give the user some help
 */
void 
DoHelp(argp)
	ARG *argp;
{
	char	*me;
	char	*client;
	int	foundOne = 0;
	int	ix = 1;
	char	*topic;

	argp = argp->aNext;	/* skip the command name */

	if (!argp)		/* no client specified */
	{
		DoReply(-LR_OK, "%d: The following clients have help:", ix);
		foundOne = HelpText(ix, 0, 0);
	} else
	{
		client = argp->aFirst;
		if (strchr(client, '/'))
			DoReply(-LR_BADHELP, "Valid help clients don't contain /'s.");
		else
			do
			{
				argp = argp->aNext;
				topic = argp ? argp->aFirst : NULL;
				if (topic && strchr(topic, '/'))
				{
					DoReply(-LR_BADHELP, "%d:%s: Valid help topics don't contain /'s.", ix, topic);
					continue;
				}
				if (topic)
					DoReply(-LR_OK, "%d:%s: ", ix, topic);
				else
					DoReply(-LR_OK, "%d: These ``%s'' help topics are available:", ix, client);
				if (HelpText(ix, client, topic) || HelpText(ix, NATIVESUBDIR, topic))
					foundOne = 1;
				else if (!HelpText(ix, client, NOHELP))
					HelpText(ix, NATIVESUBDIR, NOHELP);
				ix++;
			}
			while (argp && argp->aNext);
	}
	if (foundOne)
		DoReply(LR_OK, "Ok.");
	else
		DoReply(LR_ERROR, "No help available.");
}

/*
 * give the user a specific help file
 */
static int 
HelpText(ix, client, topic)
	int ix;
	char *client, *topic;
{
	FILE	*helpFile;
	char	buffer[MAXPATHLEN];
	char	*ep;
	int	code;

	if (!topic)
	{
		code = ListTopics(ix, client);
		if (code)
		{
			if (client && !strcmp(client, NATIVESUBDIR))
				DoReply(-LR_OK, "%d: To view one of these topics, type ``help %s name-of-topic-you-want''.", ix, NATIVESUBDIR);
			else
				DoReply(-LR_OK, "%d: To view one of these topics, type ``help name-of-topic-you-want''.", ix);
		}
		if (!code || client && strcmp(client, NATIVESUBDIR))
		{
			DoReply(-LR_OK, "%d: These ``%s'' help topics are also available:", ++ix, NATIVESUBDIR);
			ListTopics(ix, NATIVESUBDIR);
			DoReply(-LR_OK, "%d: To view one of these topics, type ``help %s name-of-topic-you-want''.", ix, NATIVESUBDIR);
		}
		return (code);
	} else
	{
		(void) sprintf(buffer, "%s/%s/%s", HELPDIR, client, topic);
		if ((helpFile = fopen(buffer, "r")) == NULL)
			return (0);
		HelpCopy(ix, helpFile);
		(void) fclose(helpFile);
		return (1);
	}
}

static int
XXcmp(x, y)
char **x, **y;
{
	return stricmp(*x, *y);
}

/*
 * list the help topics available
 */
static int 
ListTopics(indx, client)
	int indx;
	char *client;
{
	char	line[81], dirname[MAXPATHLEN];
	DIR	*dirp;
	struct dirent *direntp;
	char	*p, **names, **np;
	int	i, r, c, rows, cols, cnt=50, longest=0;


	if (client)
		(void) sprintf(dirname, "%s/%s", HELPDIR, client);
	else
		(void) strcpy(dirname, HELPDIR);

	if (dirname == NULL || *dirname == '\0')
		return(1);

	if ((dirp = opendir(dirname)) == NULL)
		return(1);

	names = (char **) malloc((cnt) * sizeof(char *));
	np = names;

	while ((direntp = readdir(dirp)) != NULL) {
		if (*direntp->d_name == '.')
			continue;
		*np = strdup(direntp->d_name);
		if ((i = strlen(*np)) > longest)
			longest = i;
		np++;
		if (np == &names[cnt]) {
			cnt += 50;
			names = (char **)realloc(names, (cnt) * sizeof(char *));
			np = &names[cnt - 50];
		}
	}
	(void) closedir(dirp);
	*np = NULL;
	cnt = np - names;
	qsort(names, cnt, sizeof(char *), XXcmp);

	longest += 2;
	cols = 70 / longest;
	longest = 70 / cols;
	rows = cnt / cols;
	if (cnt % cols)
		rows++;

	for (r = 0; r < rows; r++) {
		for (c = 0; c < cols; c++) {
			i = (c * rows) + r;
			if (i >= cnt)
				p = strdup("");
			else
				p = names[i];
			sprintf(&line[c*longest], "%-*s", longest, p);
			free(p);
		}
		DoReply(-LR_OK, "%d: %s", indx, line);
	}
	free(names);
	return (1);
}

/*
 * copy help lines from a file pointer to the output
 */
static int 
HelpCopy(indx, helpFile)
	int indx;
	FILE *helpFile;
{
	char	buffer[256];
	char	*ep;

	while (fgets(buffer, sizeof (buffer), helpFile))
	{
		ep = buffer + strlen(buffer) - 1;
		if (*ep == '\n')
			*ep = 0;
		DoReply(-LR_OK, "%d: %s", indx, buffer);
	}
}
