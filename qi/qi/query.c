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
static char  RcsId[] = "@(#)$Id: query.c,v 1.63 1995/06/28 17:40:32 p-pomes Exp $";
#endif

#include "protos.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#ifndef RLIM_INFINITY
# define	RLIM_INFINITY	0x7fffffff
#endif

extern int OldPh;		/* language.l */
extern FILE *TempOutput;
extern int InputType;
extern int LocalUser;
extern int OffCampus;

static int People,Queries;

#ifndef CPU_LIMIT
# define CPU_LIMIT 7
#endif
int	IndicateAlways = 0;

#ifdef SORTED
struct sortStruct
{
	INT32	entNum;
	char	*key;
};
static int KeyComp __P((struct sortStruct *, struct sortStruct *));
static void SortEntries __P((INT32 *, ARG *));
#endif

static int DirMatch __P((QDIR, int, char *, int, int));
static ARG *GetAllFields ();
static ARG *getplusAlways __P((ARG *));
static ARG *GetPrintDefaults ();
static void LimitTime __P((int));
static void LimitHit __P((int));
static int PrintFields __P((INT32 *, ARG *, int));
static void PrintOld __P((INT32 *));
static void PrintThem __P((INT32 *, ARG *));
static int ThinArgs __P((ARG *, int));

/*
 * Decide if a query request is valid returns the number of key fields found
 */
int 
ValidQuery(argp, cmd)
	ARG *argp;
	int cmd;
{
	FDESC	*fd;
	ARG	*origArgs;
	int	haveError = 0; /* have we detected an error yet? */
	int	count = 0;	/* count of arguments */
	int	keyCount = 0;	/* number of key fields */
	char	*cp;
	char	decrypted[MAX_LEN];
	int	dontCrypt;

	IndicateAlways = 0;
	origArgs = argp = argp->aNext;	/* skip command name */
	count++;

	/* collect arguments before ``return'' token */
	for (; argp; count++, argp = argp->aNext)
	{
		switch (argp->aType)
		{
		    case RETURN:
			dontCrypt = !strcmp(argp->aFirst, "force");
			goto keyEnd;
			break;

#ifdef DO_TILDE
		    case EQUAL | TILD_E:
#endif
		    case EQUAL:
			DoReply(-LR_SYNTAX, "=:No field or value specified.");
			break;

		    case VALUE | EQUAL:
			argp->aSecond = strdup("");
			goto canonical;

		    case VALUE:
			argp->aSecond = argp->aFirst;
			/* fall-through is deliberate */
		    case EQUAL | VALUE2:
#ifndef DEFQUERY
#define DEFQUERY "name"		/* which field searched by default */
#endif /*DEFQUERY*/
			argp->aFirst = strdup(DEFQUERY);
			argp->aType = VALUE | EQUAL | VALUE2;
			/* again, we _should_ fall through here */
		      canonical:
#ifdef DO_TILDE
		    case VALUE | EQUAL | TILD_E | VALUE2:
#endif
		    case VALUE | EQUAL | VALUE2:
			if ((fd = FindFD(argp->aFirst)) &&
			    ! (fd->fdLocalPub && !LocalUser))
			{
				if (CanLookup(fd, argp->aSecond))
				{
					argp->aKey = fd->fdIndexed && (fd->fdLookup || AmHero || AmOpr) &&
					    strlen(argp->aSecond) > 1 && !AllMeta(argp->aSecond);
#ifdef DO_TILDE
					argp->aKey = argp->aKey && !(argp->aType & TILD_E);
#endif
					if (argp->aKey)
					{
						if (fd->fdId == F_SOUND)
						{
							cp = phonemify(argp->aSecond);
							if (*cp)
								cp[strlen(cp) - 1] = '\0';	/* trim */
							free(argp->aSecond);
							argp->aSecond = strdup(cp);
						}
						argp->aRating = RateAKey(argp->aSecond);
						keyCount++;
					}
					argp->aFD = fd;
				} else
				{
					DoReply(-LR_ASEARCH,
						"%s:you may not use this field for lookup.",
						argp->aFirst);
					haveError = 1;
				}
			} else
			{
				DoReply(-LR_FIELD, "%s:unknown field.", argp->aFirst);
				haveError = 1;
			}
			break;

		    default:
			DoReply(-LR_ERROR, "Argument %d:parsing error.", count);
			haveError = 1;
			break;
		}
	}
      keyEnd:

	if (!keyCount)
	{
		haveError = 1;
		DoReply(-LR_NOKEY, "no non-null key field in query.");
	}
	keyCount = ThinArgs(origArgs, keyCount);

	if (!keyCount)
	{
		haveError = 1;
		DoReply(-LR_NOKEY, "Initial metas may be used as qualifiers only.");
	}
	if (argp)
	{
		if (cmd == C_DELETE)
		{
			DoReply(-LR_SYNTAX, "%s: unexpected.", argp->aFirst);
			haveError = 1;
			goto giveUp;
		}
		count++;
		argp = argp->aNext;	/* skip return token */
	} else if (cmd == C_CHANGE)
	{
		haveError = 1;
		DoReply(-LR_SYNTAX, "No changes requested.");
	}
	for (; argp; count++, argp = argp->aNext)
	{
		if (cmd == C_QUERY && argp->aType != VALUE)
		{
			haveError = 1;
			DoReply(-LR_SYNTAX, "Argument %d: must be field name.", count);
		} else if (cmd == C_CHANGE && (argp->aType != (VALUE | EQUAL | VALUE2)))
		{
			haveError = 1;
			DoReply(-LR_SYNTAX, "Argument %d (begins %s): must be field=value pair.", count, argp->aFirst);
		} else
		{
			if ((fd = FindFD(argp->aFirst)) &&
			    ! (fd->fdLocalPub && !LocalUser))
			{
				argp->aFD = fd;
				if (InputType == IT_NET && fd->fdEncrypt && !dontCrypt)
				{
					if (!argp->aSecond)
						argp->aSecond = strdup("");
					decrypt(decrypted, argp->aSecond);
					free(argp->aSecond);
					argp->aSecond = strdup(decrypted);
				}
			} else
			{
				DoReply(-LR_FIELD, "%s:unknown field.", argp->aFirst);
				haveError = 1;
			}
		}
	}

      giveUp:
	return (haveError ? 0 : keyCount);
}

/*
 * lookup some stuff in the database returns a pointer to a list of entries
 */
INT32 *
DoLookup(argp)
	ARG *argp;
{
	int	cnt;
	char	*keyStrings[MAX_KEYS + 1];
	char	**aString;
	char	scratch[MAX_LEN];
	char	*token;
	ARG	*anArg;
	INT32	*entries;
	INT32	*anEntry;
	INT32	*goodEntry;
	QDIR	dirp = 0;
	int	exempt = ExemptQuery(argp);
	int	suppress;
	int	notme;

	argp = argp->aNext;	/* skip command name */
	People = Queries = 0;

	/* collect keys */
	aString = keyStrings;
	for (anArg = argp; anArg; anArg = anArg->aNext)
		if (anArg->aKey && aString - keyStrings < MAX_KEYS)
		{
			strncpy(scratch, anArg->aSecond, MAX_LEN-1);
			for (token = strtok(scratch, IDX_DELIM);
			     token && aString - keyStrings < MAX_KEYS;
			     token = strtok(0, IDX_DELIM))
				if (strlen(token) > 1)
					*aString++ = strdup(token);
		}
	*aString = NULL;

	/* do key lookup */
	LimitTime(-CPU_LIMIT);
	entries = do_lookup(keyStrings, NULL);
	for (aString = keyStrings; *aString; aString++) {
		free(*aString);
		*aString = 0;
	}
	cnt = length(entries);

	if (cnt == 0)
	{
		free(entries);
		LimitTime(RLIM_INFINITY);
		return (NULL);
	}
	/* sift entries by field matches */
	for (goodEntry = anEntry = entries; *anEntry; anEntry++)
	{
		if (!next_ent(*anEntry))
		{
			IssueMessage(LOG_ERR, "DoLookup: database error on 0x%x.", *anEntry);
			continue;
		}
		getdata(&dirp);
		suppress = *FINDVALUE(dirp, F_SUPPRESS);
		notme = !User || strcmp(UserAlias, FINDVALUE(dirp, F_ALIAS));
		for (anArg = argp; anArg && anArg->aType != RETURN; anArg = anArg->aNext)
#ifdef DO_TILDE
			if (anArg->aType & TILD_E)
			{
				if (*FINDVALUE(dirp, anArg->aFD->fdId) &&
				    !DirMatch(dirp, anArg->aFD->fdId, anArg->aSecond, notme && suppress, anArg->aFlag))
					goto nextEntry;
			} else
#endif
			if (stricmp(anArg->aFirst, "Any"))	/* any ? */
			{
				if (!DirMatch(dirp, anArg->aFD->fdId, anArg->aSecond, notme && suppress, anArg->aFlag))
					goto nextEntry;
			} else
			{
				FDESC	**fd;
				int	anyfound = 0;

				/* loop through fields looking for any */

				for (fd = FieldDescriptors; *fd; fd++)
				{
					if ((*fd)->fdAny)
					{
						if (DirMatch(dirp, (*fd)->fdId, anArg->aSecond,
							  notme && suppress, anArg->aFlag))
							anyfound = 1;
					}
				}
				if (!anyfound)
					goto nextEntry;
			}
		*goodEntry++ = *anEntry;
#ifdef PERSONLIMIT
		/*
		 * count the number of id fields we find; this is more or less
		 * the number of real live people involved; we'll only count
		 * this toward the max number of entries to return thing.
		 */
		if (*FINDVALUE(dirp, F_TYPE) == 'p')
			if ((++People >= PERSONLIMIT) && !AmHero && !exempt)
			{
				FreeDir(&dirp);
				break;
			}
#endif
#ifdef QUERYLIMIT
		if ((++Queries >= QUERYLIMIT) && !AmHero && !exempt)
			{
				FreeDir(&dirp);
				break;
			}
#endif

	      nextEntry:
		FreeDir(&dirp);
	}
	*goodEntry = 0;

	LimitTime(RLIM_INFINITY);
	if (length(entries) == 0)
	{
		free(entries);
		return (NULL);
	}
	return (entries);
}

/*
* execute a query request
 */
void 
DoQuery(argp)
	ARG *argp;
{
	INT32	*entries;
	int	count;
	char	sbuf[80];

	if (!ValidQuery(argp, C_QUERY))
	{
		DoReply(LR_ERROR, "Did not understand %s.", argp->aFirst);
		return;
	}
	if (!GonnaRead("DoQuery"))
	{
		/* Lock routines give their own errors */ ;
		return;
	}
	entries = DoLookup(argp);
	Unlock("DoQuery");
	if (entries)
	{
		count = length(entries);
		if (People && OffCampus && !AmHero && !User)
		{
			DoReply(LR_OFFCAMPUS, "Remote queries not permitted.");
		} else
#if defined(PERSONLIMIT) || defined(QUERYLIMIT)
		if (AmHero || (
#ifdef QUERYLIMIT
			       Queries < QUERYLIMIT
#else
			       1
#endif
#ifdef PERSONLIMIT
			       && People < PERSONLIMIT
#endif
			       ) || ExemptQuery(argp))
#endif
		{
			/* skip to what we are supposed to print */
			for (; argp; argp = argp->aNext)
				if (argp->aType == RETURN)
				{
					argp = argp->aNext;
					break;
				}
			/* do the printing */
			sprintf(sbuf, "There %s %d match%s to your request.",
				count > 1 ? "were" : "was",
				count,
				count > 1 ? "es" : "");
			if (OldPh) {
				syslog(LOG_ERR, "Old client");
				fprintf(TempOutput, "%s\n", sbuf);
			}
			DoReply(LR_NUMRET, sbuf);
			PrintThem(entries, argp);
			DoReply(LR_OK, "Ok.");
		}
#if defined(PERSONLIMIT) || defined(QUERYLIMIT)
		else
		{
			if (OldPh)
				fprintf(TempOutput, "Too many entries to print.\n", count);
			else
				DoReply(LR_TOOMANY, "Too many entries to print.", count);
		}
#endif

		free(entries);
	} else
	{
		if (OldPh)
			fprintf(TempOutput, "No matches to your query.\n");
		else
			DoReply(LR_NOMATCH, "No matches to your query.");
	}
}

/*
* check for exempt query for mac client.
* returns 1 if query is in form:
*    proxy=loginAlias return alias
* else returns 0.
* the mac Ph client uses this query to build its "proxy" menu.
 */
int 
ExemptQuery(argp)
	ARG *argp;
{
	if (!User)
		return (0);	/* if not logged in */
	argp = argp->aNext;	/* skip command name */
	if (!argp)
		return (0);
	if (argp->aType != (VALUE | EQUAL | VALUE2))
		return (0);
	if (argp->aFD->fdId != F_PROXY)
		return (0);
	if (stricmp(argp->aSecond, UserAlias))
		return (0);
	return (1);
}

/*
* See if a directory entry matches a specification
 */
static int 
DirMatch(dirp, field, value, suppress, flag)
	QDIR dirp;
	int field, suppress, flag;
	char *value;
{
	int	fIndex;
	char	scratch[MAX_LEN];
	char	*aWord;

	/* is the field there? */
	if ((fIndex = FindField(dirp, field)) == -1)
		return (0);

	/* is suppression turned on and the field suppressible? */
	if (!AmHero && !AmOpr && suppress && !FindFDI(field)->fdForcePub)
		return (0);

	/* check whole string */
	if (!pmatch(FIELDVALUE(dirp, fIndex), value))
		return (1);

	/* nope.  check each word in string */
	strncpy(scratch, FIELDVALUE(dirp, fIndex), MAX_LEN-1);

	if (anyof(value, IDX_DELIM))
		for (aWord = strtok(scratch, IDX_DELIM); aWord; aWord = strtok(NULL, IDX_DELIM))
		{
			if (!pmatch(aWord, value))
				return (1);
	} else
		for (aWord = strtok(scratch, IDX_DELIM); aWord; aWord = strtok(NULL, IDX_DELIM))
		{
			if (!pmatch(aWord, value))
				return (1);
		}

	/* if it's the name field, check nickname as well */
	if (field == F_NAME && !(flag & A_NO_RECURSE))
		return (DirMatch(dirp, F_NICKNAME, value, suppress, flag));

	/* nope */
	return (0);
}

/*
 * print a list of entries according to the requested arguments, if any
 */
static void 
PrintThem(entries, argp)
	INT32 *entries;
	ARG *argp;
{
	static ARG *defaultArgs;
	static ARG *allArgs;
	static ARG *alwaysArgs;

	if (argp)
	{
		if (stricmp(argp->aFirst, "all"))
		{
			if (IndicateAlways)
			{
				alwaysArgs = getplusAlways(argp);
				IndicateAlways = 0;	/* only effects that query */
				PrintFields(entries, alwaysArgs, 1);
			} else
				PrintFields(entries, argp, 1);
		} else
		{
			if (!allArgs)
				allArgs = GetAllFields();
			PrintFields(entries, allArgs, 0);
		}
	} else if (OldPh)
	{
		PrintOld(entries);
	} else
	{
		if (!defaultArgs)
			defaultArgs = GetPrintDefaults();
		PrintFields(entries, defaultArgs, 0);
	}
}

/*
 *    getplus Always - get the always fields in the prod.cnf then add
 *		       only the unique fields from the query command
 */
static ARG *
getplusAlways(a)
	ARG *a;
{
	ARG     *first, *argp, *arga, *argb, *old;
	FDESC	**fd;
	int     found = 0;

	first = old = argp = FreshArg();

	for (fd = FieldDescriptors; *fd; fd++)
	{
		if ((*fd)->fdAlways)
		{
			argp->aFD = *fd;
			argp->aNext = FreshArg();
			old = argp;
			argp = argp->aNext;
		}
	}

	argp->aNext = NULL;
	/* run through the always fields selecting
	   only unique fields from the query return command */
	for (arga = a; arga; arga = arga->aNext)
	{
		found = 0;
		for (argb = first; argb; argb = argb->aNext)
			if (arga->aFD == argb->aFD)
			{
				found = 1;
				break;
			}
		if (!found)
		{
			if (stricmp(arga->aFirst, "Always"))	/* If this field is always don't save */
			{
				argp->aFD = arga->aFD;
				argp->aNext = FreshArg();
				old = argp;
				argp = argp->aNext;
				argp->aNext = NULL;
			}
		}
	}
	free(argp);
	old->aNext = NULL;
	return (first);
}

/*
 * Print select fields from entries
 */
static int 
PrintFields(entries, argp, printEmpty)
	INT32 *entries;
	ARG *argp;
	int printEmpty;
{
	int	count = 0;
	ARG	*anArg;
	QDIR	dirp;
	char	scratch[MAX_LEN];
	char	*value;
	int	indx;
	int	width = 9;	/* leave enough room for "email to" */
	int	len;
	int	suppress;
	char	*nl;

	for (anArg = argp; anArg; anArg = anArg->aNext)
		if (width <= (len = strlen(anArg->aFD->fdName)))
			width = len + 1;

#ifdef SORTED
	SortEntries(entries, argp);
#endif

	for (; *entries; entries++)
	{
		count++;
		if (!next_ent(*entries))
		{
			DoReply(-LR_ERROR, "%d:database error.", count);
			continue;
		}
		getdata(&dirp);
		suppress = *FINDVALUE(dirp, F_SUPPRESS);


		for (anArg = argp; anArg; anArg = anArg->aNext)
		{
			if (!CanSee(dirp, anArg->aFD, suppress))	/* check auth. first */
			{
				if (printEmpty)
				{
					DoReply(-LR_AINFO, "%d:%*s: You may not view this field.",
					count, width, anArg->aFD->fdName);
					IssueMessage(LOG_INFO, "%s attempting to view %s.",
						 (UserAlias) ? UserAlias : "ANON", anArg->aFirst);
				}
			} else if ((indx = FindField(dirp, anArg->aFD->fdId)) == -1)
			{
				if (printEmpty)
					DoReply(-LR_ABSENT, "%d:%*s: Not present in entry.",
					count, width, anArg->aFD->fdName);
			} else
			{
				value = FIELDVALUE(dirp, indx);
				if (!*value || *value == '*' && !CanChange(dirp, anArg->aFD) &&
				    anArg->aFD->fdTurn)
				{
					if (printEmpty)
						DoReply(-LR_ABSENT, "%d:%*s: Not present in entry.",
							count, width, anArg->aFD->fdName);
				} else
				{
					if (InputType == IT_NET && anArg->aFD->fdEncrypt)
					{
						if (!User)
							DoReply(-LR_NOTLOG, "%d:%*s: Must be logged in to view.",
								count, width, anArg->aFD->fdName);
						else
							DoReply(-LR_ISCRYPT, "%d:%*s: Encrypted; cannot be viewed.",
								count, width, anArg->aFD->fdName);
						continue;
					} else
						strncpy(scratch, value, MAX_LEN-1);
					for (value = scratch;; value = nl + 1)
					{
						nl = strchr(value, '\n');
						if (nl)
							*nl = 0;
						DoReply(-LR_OK, "%d:%*s: %s", count, width,
							value == scratch ? anArg->aFD->fdName : "",
							value);
						if (!nl)
							break;
					}
				}
			}
		}

		FreeDir(&dirp);
	}
}

/*
 * get a list of all the fields
 */
static ARG *
GetAllFields()
{
	ARG	*first, *argp, *old;
	FDESC	**fd;

	first = old = argp = FreshArg();

	for (fd = FieldDescriptors; *fd; fd++)
	{
		argp->aFD = *fd;
		argp->aNext = FreshArg();
		old = argp;
		argp = argp->aNext;
	}

	free(argp);
	old->aNext = NULL;
	return (first);
}

/*
 * get the default fields
 */
static ARG *
GetPrintDefaults()
{
	ARG	*first, *argp, *old;
	FDESC	**fd;

	first = old = argp = FreshArg();

	for (fd = FieldDescriptors; *fd; fd++)
	{
		if ((*fd)->fdDefault)
		{
			argp->aFD = *fd;
			argp->aNext = FreshArg();
			old = argp;
			argp = argp->aNext;
		}
	}

	free(argp);
	old->aNext = NULL;
	return (first);
}

#define R_META -2
#define R_STAR -10
#define R_INITIAL -100
#define R_PLAIN 1
/*
 * Pick out the ``best'' keys for use
 */
static int 
ThinArgs(argp, cnt)
	ARG *argp;
	int cnt;
{
	int	left;
	ARG	*anArg;
	int	bestRating = -1000;

	for (left = cnt, anArg = argp; left; anArg = anArg->aNext)
	{
		if (anArg->aKey)
		{
			left--;
			if (anArg->aRating > bestRating)
				bestRating = anArg->aRating;
		}
	}

	for (left = cnt, anArg = argp; left; anArg = anArg->aNext)
	{
		if (anArg->aKey)
		{
			left--;
			if (anArg->aRating < 0 && anArg->aRating < bestRating)
			{
				anArg->aKey = 0;
				cnt--;
			}
		}
	}
	return ((bestRating > R_INITIAL || AmHero) ? cnt : 0);
}

/*
 * badKeys are names with high frequency counts in the database.  This
 * list will need adjustment for non-US databases.  It's probably not
 * needed on faster platforms with smaller databases.  At UIUC, the 
 * database has 85,000 entries and runs on a VAX-3500.  We also run
 * a addnickname script that adds an entra indexed field, nickname, to
 * entries with common first names, e.g, 6:Pomes Michael Andrew has
 * 23:Mike added.  Thus nicknames must be added to the list of common
 * formal names.  If such a script isn't run, then nicknames are scarce
 * and should be removed from the list.
 */

int 
RateAKey(key)
	char *key;
{
	int	rating = 0;
	int	c;
	char	*cp = key;
	char	**beg, **mid, **end;
	static char *badKeys[] =
	{
		"alan",
		"andrew",
		"andy",
		"ann",
		"anne",
		"bill",
		"bob",
		"brian",
		"charles",
		"chris",
		"christopher",
		"dan",
		"daniel",
		"dave",
		"david",
		"ed",
		"edward",
		"eric",
		"james",
		"jeff",
		"jeffrey",
		"jennifer",
		"jenny",
		"jim",
		"joe",
		"john",
		"joseph",
		"kevin",
		"lee",
		"lynn",
		"marie",
		"mark",
		"mary",
		"matt",
		"matthew",
		"michael",
		"mike",
		"paul",
		"rich",
		"richard",
		"rob",
		"robert",
		"scott",
		"steven",
		"susan",
		"thomas",
		"tom",
		"will",
		"william"
	};

	if (strchr("[]*?", *key))
		rating = R_INITIAL;

	for (; c = *cp; cp++)
	{
		if (c == '*')
			rating += R_STAR;
		else if (strchr("[]?", c))
			rating += R_META;
		else
			rating += R_PLAIN;
	}

	beg = badKeys;
	end = beg + sizeof (badKeys) / sizeof (char *) - 1;

	while (beg <= end)
	{
		mid = beg + (end - beg) / 2;
		c = strcmp(*mid, key);
		if (!c)
			return (R_STAR * 2);
		if (c < 0)
			beg = mid + 1;
		else
			end = mid - 1;
	}

	return (rating);
}

/*
 * print a simple request in a simple format
 */
static void 
PrintOld(entries)
	INT32 *entries;
{
	QDIR	dirp;
	char	*value;

	for (; *entries; entries++)
	{
		if (!next_ent(*entries))
		{
			fprintf(TempOutput, "Error:  couldn't read entry.\n");
			continue;
		}
		fputs("-----------------------------------\n", TempOutput);

		getdata(&dirp);
		if (*(value = FINDVALUE(dirp, F_NAME)))
			fprintf(TempOutput, "%s\n", value);

		if (*(value = FINDVALUE(dirp, F_EMAIL)))
			fprintf(TempOutput, "%s\n", value);

		if (*(value = FINDVALUE(dirp, F_PHONE)))
			fprintf(TempOutput, "%s\n", value);

		if ((*(value = FINDVALUE(dirp, F_TITLE))) ||
		    (*(value = FINDVALUE(dirp, F_CURRICULUM))))
			fprintf(TempOutput, "%s\n", value);

		if (*(value = FINDVALUE(dirp, F_DEPARTMENT)))
			fprintf(TempOutput, "%s\n", value);

		if (*(value = FINDVALUE(dirp, F_ADDRESS)))
			fprintf(TempOutput, "%s\n", value);

		FreeDir(&dirp);
	}
	fputs("-----------------------------------\n", TempOutput);
}

/*
 * LimitTime - set the soft CPU limit for this process.
 *  If secs<0, it means |secs| more time; otherwise, limit is set to secs.
 */
#ifdef hpux
# include <sys/syscall.h>
# ifndef RLIMIT_CPU
#  define RLIMIT_CPU	0
# endif /* !RLIMIT_CPU */
# define getrusage(a, b)  syscall(SYS_GETRUSAGE, a, b)
#endif /* hpux */

static void 
LimitTime(secs)
	int secs;
{
#ifdef SIGXCPU
	struct rlimit lim;
# ifdef RUSAGE_SELF
	struct rusage foo;
	int now;
	getrusage(RUSAGE_SELF, &foo);
	now = ((foo.ru_utime.tv_sec + foo.ru_stime.tv_sec) * 1000000 +
		foo.ru_utime.tv_usec + foo.ru_stime.tv_usec) / 1000000;
# else /* !RUSAGE_SELF */
	clock_t now = clock();
#  ifdef CLK_TCK
	now = now / CLK_TCK;
#  else /* !CLK_TCK */
#   ifdef CLOCKS_PER_SEC
	now = now / CLOCKS_PER_SEC;
#   else /* !CLOCKS_PER_SEC */
	DONT_KNOW_HOW_TO_MAKE_CLOCK_RETURN_SECONDS
#   endif /* CLOCKS_PER_SEC */
#  endif /* CLK_TCK */
# endif /* RUSAGE_SELF */

	lim.rlim_max = RLIM_INFINITY;
	if (secs < 0)
	{
		lim.rlim_cur = now - secs;
	} else
		lim.rlim_cur = secs;

	setrlimit(RLIMIT_CPU, &lim);
	(void) signal(SIGXCPU, LimitHit);
#endif /* SIGXCPU */
}

/*
 * exit cleanly when limit hit
 */
static void
LimitHit(sig)
	int sig;
{
#ifdef SIGXCPU
	signal(SIGXCPU, SIG_IGN);
	fprintf(Output, "%d: CPU time limit exceeded for this session.\n", LR_XCPU);
	DoQuit(NULL);
#endif /* SIGXCPU */
}

#ifdef SORTED
static void 
SortEntries(entries, argp)
	INT32 *entries;
	ARG *argp;
{
	INT32	*e;
	struct sortStruct *aKey, *keys = NULL;
	QDIR	dir;
	int	n;

	for (e = entries; *e; e++) ;
	n = e - entries;
	aKey = keys = malloc(n * sizeof (struct sortStruct));
	memset(keys, (char)0, n * sizeof (struct sortStruct));

	for (e = entries; *e; e++)
	{
		if (!next_ent(*e))
			goto done;
		getdata(&dir);
		aKey->entNum = *e;
		aKey++->key = strdup(FINDVALUE(dir, argp->aFD->fdId));
		FreeDir(&dir);
	}
	qsort(keys, n, sizeof (struct sortStruct), KeyComp);

	for (aKey = keys; aKey < keys + n; aKey++)
		*entries++ = aKey->entNum;

      done:
	if (keys)
	{
		for (aKey = keys; aKey < keys + n; aKey++)
			if (aKey->key)
				free(aKey->key);
			else
				break;
		free(keys);
	}
}

static int 
KeyComp(a, b)
	struct sortStruct *a, *b;
{
	return (strcmp(a->key, b->key));
}

#endif
