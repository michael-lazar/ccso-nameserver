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
static char  RcsId[] = "@(#)$Id: field.c,v 1.30 1994/12/17 14:45:21 p-pomes Exp $";
#endif

#include "protos.h"

#include <sys/param.h>

/*
 * this file contains functions that deal with fields
 */
FDESC	**FieldDescriptors;
static int LineNumber;

#define FREEFD(FD)    {free((FD)->fdName);free(FD);FD=0;}
#define IS_A_COMMENT(l) (*(l) == '#')
#define IS_BLANK(l)   (!(*(StrSkip(l," \t\n\f"))))

static void DumpFD __P((FILE *, FDESC *));
static FDESC *GetFD __P((FILE *));
static char *StrSkip __P((char *, char *));

extern int IndicateAlways;

/*
 * read the field config file.	Read into global FieldDescriptors
 */
int 
GetFieldConfig()
{
	int	i;
	FILE	*fp;
	FDESC	*fd;
	char	fconfig[MAXPATHLEN];

	(void) sprintf(fconfig, "%s.cnf", Database);

	if ((fp = fopen(fconfig, "r")) == NULL)
	{
		IssueMessage(LOG_ERR, "GetFieldConfig: fopen(%s): %s",
			fconfig, strerror(errno));
		return (0);
	}
	if (FieldDescriptors)
	{
		for (; *FieldDescriptors; FieldDescriptors++)
			FREEFD(*FieldDescriptors);
		FieldDescriptors = NULL;
	}
	FieldDescriptors = (FDESC **) malloc(sizeof (FDESC *));
	LineNumber = 0;

	i = 0;
	while (fd = GetFD(fp))
	{
		*(FieldDescriptors + i) = fd;
		/* DumpFD(stdout,*(FieldDescriptors+i)); */
		i++;
		FieldDescriptors = (FDESC **) realloc(FieldDescriptors, (i+1) * sizeof (FDESC *));
	}
	*(FieldDescriptors + i) = NULL;
	return (i);
}

/*
 * read a single field descriptor from a file.	storage is malloc'ed
 */
static FDESC *
GetFD(fp)
	FILE *fp;
{
	static char Line[MAX_LINE + 2];
	FDESC	*fd;
	FDESC	tempFD;
	int	number;
	char	*token;
	char	help[MAX_LINE];

	*Line = '\0';
	do
	{
		if (fgets(Line, MAX_LINE + 1, fp) == NULL)
			return (NULL);
		LineNumber++;
	}
	while (!*Line || IS_A_COMMENT(Line) || IS_BLANK(Line));

	/* got a non-comment, non-blank line.  now try to parse it. */
	memset(&tempFD, 0, sizeof(FDESC));

	/* strip the newline */
	/* Line[strlen(Line) - 2] = '\0'; */
	if ((token = strchr(Line, '\n')) != NULL)
		*token = '\0';

	/* Id number */
	if (!(token = strtok(Line, ":")))
		goto err;
	tempFD.fdId = atoi(token);

	/* name */
	if (!(token = strtok(NULL, ":")))
		goto err;
	tempFD.fdName = strdup(token);

	/* max length */
	if (!(token = strtok(NULL, ":")))
	{
		free(tempFD.fdName);
		goto err;
	}
	tempFD.fdMax = atoi(token);

	/* help */
	*help = '\0';
	for (token = strtok(NULL, ":"); token; token = strtok(NULL, ":"))
	{
		number = strlen(token);
		if (number)
		{
			if (token[number - 1] == '\\')	/* ends in backslash? */
				token[number - 1] = ':';	/* turn it into colon */
			else
				number = 0;
		}
		strcate(help, token);
		if (!number)
			break;	/* stop with help */
	}
	tempFD.fdHelp = strdup(help);

	/* merge flags */
	if (token = strtok(NULL, ":"))
		tempFD.fdMerge = strdup(token);
	else
		tempFD.fdMerge = "";

	/* flags */
	for (token = strtok(NULL, ":"); token; token = strtok(NULL, ":"))
	{
		if (islower(*token))
			*token = toupper(*token);
		switch (*token)
		{
		    case 'A':
			tempFD.fdAlways = 1;
			break;
		    case 'C':
			tempFD.fdChange = 1;
			break;
		    case 'D':
			tempFD.fdDefault = 1;
			break;
		    case 'E':
			tempFD.fdEncrypt = 1;
			break;
		    case 'F':
			tempFD.fdForcePub = 1;
			break;
		    case 'I':
			tempFD.fdIndexed = 1;
			break;
		    case 'L':
			if (!stricmp(token, "LookUp"))
				tempFD.fdLookup = 1;
			else
				tempFD.fdLocalPub = 1;
			break;
		    case 'N':
			if (!stricmp(token, "NoMeta"))
				tempFD.fdNoMeta = 1;
			else
				tempFD.fdNoPeople = 1;
			break;
		    case 'P':
			tempFD.fdPublic = 1;
			break;
		    case 'S':
			tempFD.fdSacred = 1;
			break;
		    case 'T':
			tempFD.fdTurn = 1;
			break;
		    case 'W':
			tempFD.fdAny = 1;
			break;	/* Wildany */
		    default:
			IssueMessage(LOG_ERR, "%s: unknown field flag\n", token);
			break;
		}
	}

	fd = (FDESC *) malloc(sizeof (FDESC));
	memcpy((void *)fd, (void *)&tempFD, sizeof (FDESC));
	return (fd);
err:
	IssueMessage(LOG_ERR, "GetFD: config file error, line %d", LineNumber);
	return (NULL);
}

/*
 * dump a field descriptor
 */
static void 
DumpFD(fp, fd)
	FILE *fp;
	FDESC *fd;
{
	fprintf(fp, "%d\t%s\t%s\t%s\t%s\t%s\t%d\t%s\n%s",
		fd->fdId,
		fd->fdIndexed ? "idx" : "!idx",
		fd->fdLookup ? "lup" : "!lup",
		fd->fdNoMeta ? "nmt" : "!nmt",
		fd->fdPublic ? "pub" : "!pub",
		fd->fdLocalPub ? "lpu" : "!lpu",
		fd->fdDefault ? "dft" : "!dft",
		fd->fdAlways ? "alw" : "!alw",
		fd->fdAny ? "any" : "!any",
		fd->fdChange ? "chg" : "!chg",
		fd->fdNoPeople ? "npl" : "!npl",
		fd->fdMax,
		fd->fdName,
		fd->fdHelp);
}

/*
 * advance a string pointer past all occurrences of a set of chars.
 */
static char *
StrSkip(str, skipChars)
	char *str, *skipChars;
{
	register char *skipThis;/* current char in skip set */
	register char chr;	/* current char in str */

	for (chr = *str;; chr = *(++str))
	{
		for (skipThis = skipChars; *skipThis; skipThis++)
			if (chr == *skipThis)
				goto outerFor;	/* chr is in skipChars */
		return (str);	/* chr is NOT in skipChars */
	      outerFor:;
	}
}

/*
 * find a numbered field in a QDIR structure
 * returns index of field, or -1 if not found
 */
int 
FindField(indir, num)
	QDIR indir;
	int num;
{
	char	ascii[20];
	register char **dir;
	int	len;

	(void) sprintf(ascii, "%d:", num);
	len = strlen(ascii);

	for (dir = indir; *dir; dir++)
		if (!strncmp(ascii, *dir, len))
			return (dir - indir);

	return (-1);
}

/*
 * find a field descriptor by Id
 */
FDESC *
FindFDI(indx)
	int indx;
{
	FDESC **fd;

	for (fd = FieldDescriptors; *fd; fd++)
		if ((*fd)->fdId == indx)
			return (*fd);

	return (NULL);
}

/*
 * find a field descriptor by name
 */
FDESC *
FindFD(name)
	char *name;
{
	FDESC **fd;

	for (fd = FieldDescriptors; *fd; fd++)
		if (!stricmp((*fd)->fdName, name))
		{		/* always field/property. Probably a better way to do this.  mae */
			if (!stricmp("Always", name))
			{
				IndicateAlways = 1;
			}
			return (*fd);
		}
	return (NULL);
}
