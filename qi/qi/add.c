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
static char  RcsId[] = "@(#)$Id: add.c,v 1.29 1995/06/23 19:21:28 p-pomes Exp $";
#endif

#include "protos.h"

static QDIR MakeADir __P((ARG *));
static int ValidAdd __P((ARG *));

extern int	InputType;
extern char	*Kdomain;

/*
 * Do an add command
 */
void 
DoAdd(args)
	ARG *args;
{
	INT32	entry;
	QDIR	dir;
	ARG	*parg;

	if (!AmHero && !User)
		DoReply(LR_NOTLOG, "You must be logged in to use this command.");
	else if (!CanAddEntries())
	{
		IssueMessage(LOG_INFO, "%s:not authorized for add", UserAlias);
		DoReply(LR_ADD, "You may not add NameServer entries.");
	}
	else if (!GonnaWrite("DoAdd"))
		/* GonnaWrite will issue an error message */
		;
	else if (!ValidAdd(args))
	{
		Unlock("DoAdd");
		DoReply(LR_SYNTAX, "Add command not understood.");
	}
	else
	{
		dir = MakeADir(args->aNext);
		entry = new_ent();
		if (!putdata(dir))
		{
			Unlock("DoAdd");
			FreeDir(&dir);
			IssueMessage(LOG_ERR, "DoAdd: putdata failed");
			DoReply(LR_TEMP, "Store failed.");
			return;
		}
		MakeLookup(dir, entry, make_lookup);
		set_date(0);
		store_ent();
		Unlock("DoAdd");
		FreeDir(&dir);

#ifdef KDB
		for (parg = args->aNext; parg; parg = parg->aNext)
		{
			if (!strcmp(parg->aFirst, "alias"))
			{
				kdb_add_entry(parg->aSecond);
				break;
			}
		}
#endif /* KDB */
		DoReply(LR_OK, "Ok.");
	}
}

/*
 * Make a dir entry from an argument list
 */
static QDIR 
MakeADir(args)
	ARG *args;
{
	QDIR	dir;

	dir = (QDIR) malloc(sizeof (char *));

	*dir = NULL;

	for (; args; args = args->aNext)
	{
		if (strcmp(args->aSecond, "none"))
			if (!ChangeDir(&dir, args->aFD, args->aSecond))
				IssueMessage(LOG_ERR, "MakeADir: ChangeDir failed");
	}
	return (dir);
}

/*
 * validate an argument list for an add command
 */
static int 
ValidAdd(args)
	ARG *args;
{
	int	isBad = 0;
	int	count = 0;
	char	*reason = NULL;

	for (args = args->aNext; args; args = args->aNext)
	{
		count++;
		if (args->aType != (VALUE | EQUAL | VALUE2))
		{
			isBad = 1;
			DoReply(-LR_SYNTAX, "%d:argument is not field=value pair.", count);
		} else if ((args->aFD = FindFD(args->aFirst)) == NULL)
		{
			isBad = 1;
			DoReply(-LR_FIELD, "%d:unknown field.", args->aFD->fdId);
		} else if (!strcmp(args->aSecond, "none"))
		{
			isBad = 1;
			DoReply(-LR_SYNTAX, "``None'' not allowed on add.");
		} else if (args->aFD->fdId == F_HERO && InputType != IT_TTY &&
			   InputType != IT_PIPE && InputType != IT_FILE)
		{
			DoReply(-LR_ACHANGE, "Hero field may not be set from a network session.");
			isBad = 1;
			continue;
		} else if (args->aFD->fdId == F_ALIAS)
		{
			if ((reason = BadAlias(NULL, args->aSecond)) != NULL) {
			    DoReply(-LR_VALUE, reason);
			    isBad = 1;
			    continue;
			} else if (AliasIsUsed(args->aSecond, 0))
			{
				DoReply(-LR_ALIAS, "Alias %s in use or is too common a name.",
					args->aSecond);
				isBad = 1;
				continue;
			}
		}
#ifdef PRE_ENCRYPT
		else if (args->aFD->fdId == F_PASSWORD)
		{
			char	pwCrypt[14];

			strncpy(pwCrypt, crypt(args->aSecond, args->aSecond), 13);
			pwCrypt[13] = 0;
			free(args->aSecond);
			args->aSecond = strdup(pwCrypt);
		}
#endif
	}

	return (!isBad);
}
