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
static char  RcsId[] = "@(#)$Id: auth.c,v 1.27 1995/06/23 19:22:14 p-pomes Exp $";
#endif

#include "protos.h"

static int IsProxy __P((QDIR, char *));
extern int LocalUser, InputType;

/*
 * check if the logged in user is allowed to add entries
 */
int 
CanAddEntries()
{
	return (AmHero);
}

/*
 * can the current user see the field in question?
 */
int 
CanSee(dir, fd, suppress)
	QDIR dir;
	FDESC *fd;
	int suppress;
{
	if (AmHero || AmOpr)
		return (1);	/* the supreme and semi-supreme being */

	if (User != NULL)
	{
		if (!stricmp(FINDVALUE(dir, F_ALIAS), UserAlias))
			return (1);	/* user's own record */

		if (IsProxy(dir, UserAlias))
			return (1);	/* (one of) the user's proxy(ies) */
	}
	if (suppress && !fd->fdForcePub)
		return (0);
	return (fd->fdPublic || (fd->fdLocalPub && LocalUser));
}

/*
 * is the given alias a proxy of the given dir
 */
static int 
IsProxy(dir, whichAlias)
	QDIR dir;
	char *whichAlias;
{
	char	*token;
	char	proxies[2048];

	strcpy(proxies, FINDVALUE(dir, F_PROXY));
	for (token = strtok(proxies, " \t,\n"); token && *token; token = strtok((char *) 0, " \t,\n"))
		if (!stricmp(whichAlias, token))
			return (1);
	return (0);
}

/*
 * can the current user do a lookup using the field in question?
 */
int 
CanLookup(fd, arg)
	FDESC *fd;
	char *arg;
{
	if (AmOpr || AmHero)
		return (1);
	if (!fd->fdLookup)
		return (0);
	if (fd->fdNoMeta && anyof(arg, METAS))
		return (0);
	return (1);
}

/*
 * can the current user change anything in an entry?
 */
int 
UserCanChange(dir)
	QDIR dir;
{
	if (AmHero)
		return (1);	/* the supreme being */

	if (!User)
		return (0);	/* nobody logged in */

	if (!stricmp(FINDVALUE(dir, F_ALIAS), UserAlias))
		return (1);	/* user's own entry */

	if (IsProxy(dir, UserAlias))
		return (1);	/* (one of) the user's proxy(ies) */

	if (AmOpr)
	{
		if (*FINDVALUE(dir, F_HERO))
			return (0);	/* oper not allowed to edit hero/opr */
		return (1);
	}

	return (0);		/* sorry... */
}

/*
 * can the current user change a field in an entry?
 */
int 
CanChange(dir, fd)
	QDIR dir;
	FDESC *fd;
{
	int	isPerson;

	if (fd->fdId == F_HERO && InputType != IT_TTY &&
	    InputType != IT_PIPE && InputType != IT_FILE)
		return(0);	/* hero field can't be touched from the net */

	if (AmHero)
		return (1);	/* the supreme being */

	if (!User)
		return (0);	/* nobody logged in */

	isPerson = *FINDVALUE(dir, F_TYPE) == 'p';

	if (!stricmp(FINDVALUE(dir, F_ALIAS), UserAlias))
		return (fd->fdChange || fd->fdNoPeople && !isPerson);

	if (IsProxy(dir, UserAlias) || AmOpr)
		return (fd->fdChange || fd->fdNoPeople && !isPerson);

	return (0);		/* sorry... */
}

/*
 * Can the logged in user delete any entries at all?
 */
int 
UserCanDelete()
{
	return (AmHero);
}

/*
 * Can the logged in user delete a particular entry?
 */
int 
CanDelete(dir)
	QDIR dir;
{
	/* no one may delete the Hero entry */
	/* return(AmHero && stricmp(Hero, FINDVALUE(dir,F_ALIAS))); */
	return (AmHero);
}
