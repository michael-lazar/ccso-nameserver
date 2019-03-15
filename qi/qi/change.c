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
static char  RcsId[] = "@(#)$Id: change.c,v 1.53 1995/06/10 03:40:37 p-pomes Exp $";
#endif

#include "protos.h"
#include <signal.h>

#ifdef KDB
# include <krb5/krb5.h>
# include <krb5/kdb.h>
#endif /* KDB */

static int ChangeEntries __P((INT32 *, ARG *));

extern int end;
extern int InputType;

/*
 * Do a change command
 */
void
DoChange(arg)
    ARG *arg;
{
    INT32 *entries = NULL;
    int  changedCount;
    int  foundCount;

    if (ReadOnly) {
	DoReply(LR_READONLY, "changes not allowed to read-only database.");
	return;
    } if (!AmHero && !AmOpr && !User) {
	DoReply(LR_NOTLOG, "You must be logged in to use this command.");
	return;
    } if (!ValidQuery(arg, C_CHANGE)) {
	DoReply(LR_SYNTAX, "Did not understand change command.");
	return;
    }
    else if (!GonnaWrite("DoChange")) {
	/* GonnaWrite will issue an error message */ ;
	return;
    }
    if (!(entries = DoLookup(arg))) {
	Unlock("DoChange");
	free((char *) entries);
	DoReply(LR_NOMATCH, "No matches to specification.");
	return;
    }
    while (arg->aType != RETURN)
	arg = arg->aNext;	/* skip query */
    arg = arg->aNext;

    foundCount = length(entries);
    if (OP_VALUE(LIMIT_OP) && foundCount > atoi(OP_VALUE(LIMIT_OP)))
    {
	Unlock("DoChange");
	free((char *) entries);
	DoReply(LR_LIMIT, "Too many entries (%d) selected; limit is %s.",
		foundCount, OP_VALUE(LIMIT_OP));
	return;
    }
    changedCount = ChangeEntries(entries, arg);
    Unlock("DoChange");
    free((char *) entries);
    entries = NULL;
    if (foundCount == changedCount)
	DoReply(LR_OK, "%d entr%s changed.", changedCount,
		changedCount > 1 ? "ies" : "y");
    else if (changedCount)
	DoReply(LR_ERROR, "Only %d entr%s changed (%d found).", changedCount,
		changedCount > 1 ? "ies" : "y", foundCount);
    else
	DoReply(LR_ERROR, "%d entr%s found, none changed.",
		foundCount, foundCount > 1 ? "ies" : "y");
}

/*
** Change selected fields in entries
*/
static int
ChangeEntries(entries, InArgs)
    INT32 *entries;
    ARG *InArgs;
{
    ARG *arg;
    QDIR dir;
    QDIR oldDir;
    int  successes;
    int  entryDirty;
    int  indexDirty;
    int  i;

#ifdef PASS_AUTH
    int  pwDirty;
#endif /* PASS_AUTH */
    char *reason = NULL;
#ifdef SIG_BLOCK
    sigset_t set, oldMask;
#else
    int  oldMask;
#endif

    /* loop through all requested entries */
    for (successes = 0; *entries; entries++) {
	/* retrieve the entry */
	if (!next_ent(*entries)) {
	    DoReply(-LR_TEMP, "%d:couldn't fetch.", *entries);
	    IssueMessage(LOG_ERR, "ChangeEntries: %d:couldn't fetch", *entries);
	    continue;
	}
	getdata(&dir);
	getdata(&oldDir);
	if (!UserCanChange(dir)) {
	    IssueMessage(LOG_INFO, "%s not authorized to change entry for %s",
			 UserAlias, FINDVALUE(dir, F_ALIAS));
	    DoReply(-LR_AENTRY, "%s:You may not change this entry.",
		    FINDVALUE(dir, F_ALIAS));
	}
	else {
#ifdef PASS_AUTH
	    entryDirty = indexDirty = pwDirty = 0;
#else /* !PASS_AUTH */
	    entryDirty = indexDirty = 0;
#endif /* PASS_AUTH */
	    for (arg = InArgs; arg; arg = arg->aNext) {
		if (CanChange(dir, arg->aFD)) {
		    if (OP_VALUE(ADDONLY_OP) && *FINDVALUE(dir, arg->aFD->fdId)) {
			DoReply(-LR_ADDONLY, "Field has a value.");
		    }
		    else {
			if (arg->aFD->fdId == F_ALIAS) {
			    if ((reason = BadAlias(dir,arg->aSecond)) != NULL) {
				DoReply(-LR_VALUE, reason);
				continue;
			    }
			    else if (AliasIsUsed(arg->aSecond, *entries)) {
				DoReply(-LR_ALIAS, "Alias %s conflicts with other users.",
					arg->aSecond);
				continue;
			    }
			}
#ifdef PASS_AUTH
			else if (arg->aFD->fdId == F_PASSWORD) {
			    register char *k;

			    for (k = arg->aSecond; *k; k++)
				if (*k < ' ' || *k > '~') {
				    DoReply(-LR_VALUE, "Passwords must use only printable characters; sorry.");
				    DoReply(-LR_VALUE, "If your password met this rule, reissue your login command, and try again.");
				    break;
			    }
			    if (*k)
				continue;
#ifdef CRACKLIB
			    if (k = (char *) FascistCheck(dir, arg->aSecond, CRACKLIB)) {
				DoReply(-LR_VALUE, "Please use a different password.");
				DoReply(-LR_VALUE, "The one offered is unsuitable because %s.", k);
				continue;
			    }
#endif /* CRACKLIB */
#ifdef PRE_ENCRYPT
			    {
				char pwCrypt[14];

				strncpy(pwCrypt, crypt(arg->aSecond, arg->aSecond), 13);
				pwCrypt[13] = '\0';
				free(arg->aSecond);
				arg->aSecond = strdup(pwCrypt);
			    }
#endif /* PRE_ENCRYPT */
			}
#endif /* PASS_AUTH */
#ifdef MAILDOMAIN
			else if (arg->aFD->fdId == F_EMAIL) {
			    char scratch[256];

			    strcpy(scratch, "@");
			    strcat(scratch, MAILDOMAIN);
			    if (strlen(scratch) > 1 &&
				issub(arg->aSecond, scratch)) {
				DoReply(-LR_VALUE, "Your email field must not contain addresses ending in @%s;", MAILDOMAIN);
				DoReply(-LR_VALUE, "Use a specific login and machine instead.");
				break;
			    }
			}
#endif /* MAILDOMAIN */
#ifdef DOSOUND
			else if (arg->aFD->fdId == F_NAME) {	/* change sound, too */
			    if (!ChangeDir(&dir, FindFDI(F_SOUND), phonemify(arg->aSecond)))
				IssueMessage(LOG_ERR, "ChangeEntries: couldn't change sound");
			}
#endif
			if (!ChangeDir(&dir, arg->aFD, arg->aSecond)) {
			    IssueMessage(LOG_ERR, "ChangeEntries: couldn't change dir entry");
			    DoReply(-LR_TEMP, "Having difficulties.  Change has failed.");
			}
			else {
#if defined(KRB4_AUTH) && defined(PASS_AUTH)
			    /* Ph password changes don't go to Kerberos */
			    if (arg->aFD->fdId == F_PASSWORD &&
			      *FINDVALUE(dir, F_ALIAS)) {
				DoReply(-LR_OK, "Ph password change for %s successful.", FINDVALUE(dir, F_ALIAS));
				DoReply(-LR_OK, "Kerberos password for %s must be changed outside of ph with kpasswd.", FINDVALUE(dir, F_ALIAS));
			    }
#endif /* KRB4_AUTH && PASS_AUTH */
#ifdef KDB
			    if (arg->aFD->fdId == F_ALIAS &&
			      *FINDVALUE(oldDir, F_ALIAS)) {
				kdb_del_entry(FINDVALUE(oldDir, F_ALIAS));
				if (!kdb_add_entry(arg->aSecond))
				    DoReply(-LR_OK, "Kerberos password for %s must be reset outside of ph with kpasswd.",
					arg->aSecond);
			    }
#endif /* KDB */
			}
#ifdef PASS_AUTH
			pwDirty = pwDirty || arg->aFD->fdId == F_PASSWORD;
#endif /* PASS_AUTH */
			entryDirty = 1;
			indexDirty = indexDirty || arg->aFD->fdIndexed;
		    }
		}
		else
		    DoReply(-LR_ACHANGE, "%s:you may not change this field.",
			    arg->aFD->fdName);
	    } if (entryDirty) {
		if (!putdata(dir))
		    DoReply(-LR_TEMP, "%s:Couldn't store.",
			    FINDVALUE(dir, F_ALIAS));
		else {
		    successes++;
#ifdef SIGXCPU
# ifdef SIG_BLOCK
		    sigemptyset(&set);
		    sigaddset(&set, SIGXCPU);
		    (void) sigprocmask(SIG_BLOCK, &set, &oldMask);
# else /* !SIG_BLOCK */
		    oldMask = sigblock(sigmask(SIGXCPU));
# endif /* SIG_BLOCK */
#endif /* SIGXCPU */
		    set_date(1);
		    store_ent();
		    if (indexDirty) {
			MakeLookup(oldDir, *entries, unmake_lookup);
			MakeLookup(dir, *entries, make_lookup);
		    }
#ifdef SIGXCPU
# ifdef SIG_BLOCK
		    (void) sigprocmask(SIG_SETMASK, &oldMask, NULL);
# else /* !SIG_BLOCK */
		    sigsetmask(oldMask);
# endif /* SIG_BLOCK */
#endif /* SIGXCPU */
		    if (OP_VALUE(VERBOSE_OP))
			DoReply(LR_PROGRESS, "%s:changed.", FINDVALUE(dir, F_ALIAS));
		    if (*entries == UserEnt) {	/* replace User dir with new dir */
			FreeDir(&User);
			FreeDir(&oldDir);
			User = dir;
			dir = NULL;
#ifdef PASS_AUTH
			if (pwDirty)
			    crypt_start(PasswordOf(User));
#endif /* PASS_AUTH */
			UserAlias = FINDVALUE(User, F_ALIAS);
			continue;	/* avoid freeing dir, which is now User */
		    }
		}
	    }
	}
	FreeDir(&dir);
	FreeDir(&oldDir);
    } return (successes);
}

/*
** change a dir entry
*/
int
ChangeDir(dir, fd, value)
    QDIR *dir;
    FDESC *fd;
    char *value;
{
    char **ptr;
    int  indx;
    int  count;
    char scratch[MAX_LEN+6];	/* to cope with the added NNNN: tag */

    if ((indx = FindField(*dir, fd->fdId)) >= 0) {
	ptr = (*dir) + indx;
	free(*ptr);
    }
    else if (!*value || strcmp(value, "none")) {
	count = Plength((INT32 *) *dir) + 2;
	*dir = (QDIR) realloc((char *) *dir, (unsigned) (count * sizeof (char *)));

	ptr = (*dir) + count - 1;
	*ptr = NULL;		/* dir terminator */
	ptr--;			/* back up to right spot */
    }
    else {
	/* we are deleting, && no value presently exists.  do nothing */
	return (1);
    }

    /* ptr now points to the proper char pointer */
    if (strcmp(value, "none")) {
	sprintf(scratch, "%d:%s", fd->fdId, value);

	/* enforce max length */
	indx = fd->fdMax + ((char *)strchr(scratch, ':') - scratch) + 1;
	scratch[indx] = '\0';
	*ptr = strdup(scratch);
    }
    else {
	/* remove pointer at ptr */
	do {
	    *ptr = *(ptr + 1);
	} while (*++ptr);
    } return (1);
}

/*
** Check to see if alias is in use as an alias or name by any record
** other than the one belonging to the current user.  (New JLN version).
*/
int
AliasIsUsed(alias, requestor)
    char *alias;
    INT32 requestor;
{
    QDIR user;
    ARG *argList;
    ARG *arg;
    INT32 *entry, *e1;
    int  result = ! AmHero;	/* superuser knows what they're doing (?) */
    char *p, tc;
#ifdef KDB
    krb5_boolean more;
    krb5_error_code retval;
    krb5_db_entry kentry;
    krb5_principal newprinc;
    int nprincs = 1;
    int exists = FALSE;

    /* Can't re-use an existing principal no matter who you are. */
    if (retval = krb5_parse_name(alias, &newprinc)) {
	IssueMessage(LOG_ERR, "AliasIsUsed: krb5_parse_name(%s): %s.",
	    alias, error_message(retval));
	return(1);		/* Protocol Failure */
    }
    if (retval = krb5_db_get_principal(newprinc, &kentry, &nprincs, &more))
    {
	IssueMessage(LOG_ERR, "AliasIsUsed: krb5_db_get_principal(%s): %s.",
	    alias, error_message(retval));
	return(1);
    }
    krb5_free_principal(newprinc);
    krb5_db_free_principal(&kentry, nprincs);
    if (nprincs)
	exists = TRUE;
#endif /* KDB */

    /* Can't re-use an existing alias no matter who you are. */
    if (user = GetAliasDir(alias)) {
	FreeDir(&user);
#ifdef KDB
	if (!exists) {
	    IssueMessage(LOG_ERR, "AliasIsUsed: %s in qi but not KDC.", alias);
	    return(1);
	}
#endif /* KDB */
	return(1);
    }
#ifdef KDB
    else if (exists) {
	IssueMessage(LOG_ERR, "AliasIsUsed: %s in KDC but not in qi.", alias);
	return(1);
    }
#endif /* KDB */

    /* This check is bypassed if a Hero made the request. */
    if (result) {

	/*
	** Build a ARG list for making name queries.  Set A_NO_RECURSE to
	** disable tail recursion into nickname.
	*/
	arg = argList = FreshArg();
	arg->aType = COMMAND;
	arg->aFirst = strdup("query");
	arg->aFlag = A_NO_RECURSE;
	arg->aNext = FreshArg();
	arg = arg->aNext;
	arg->aType = VALUE | EQUAL | VALUE2;
	arg->aFirst = strdup("name");
	arg->aSecond = strdup(alias);
	arg->aFlag = A_NO_RECURSE;

	/*
	** Loop through checks until we either match the user making the
	** request (result=0) or exhaust permutations (result=1).
	*/
	while (1) {
	    (void) ValidQuery(argList, C_QUERY);
	    if ((entry = DoLookup(argList)) != NULL) {
		for (result = 1, e1 = entry; *e1 && result; e1++) {
		    next_ent(*e1);
		    result = (CurrentIndex() == requestor) ? 0 : 1;
		}
		free((char *) entry);
		entry = NULL;
		break;
	    }

	    /*
	    ** If the first DoLookup() failed to match anyone, break up the
	    ** requested alias into separate words.  If any string is a single
	    ** character, make it into a wildcard, e.g, "p-pomes" becomes
	    ** "p*" and "pomes".
	    */
	    if (p = strchr(arg->aSecond, '-')) {
		*p = '\0';
		if (strlen(arg->aSecond) == 1) {
		    tc = *arg->aSecond;
		    arg->aSecond = strdup("  ");
		    (void) sprintf(arg->aSecond, "%c*", tc);
		}
		if (p[1]) {
		    arg->aNext = FreshArg();
		    arg = arg->aNext;
		    arg->aType = VALUE | EQUAL | VALUE2;
		    arg->aFirst = strdup("name");
		    arg->aSecond = strdup(p + 1);
		    arg->aFlag = A_NO_RECURSE;
		    if (strlen(arg->aSecond) == 1) {
			tc = *arg->aSecond;
			arg->aSecond = strdup("  ");
			(void) sprintf(arg->aSecond, "%c*", tc);
		    }
		}
		continue;
	    }
	    result = 0;
	    break;
	}
	FreeArgs(argList);
    }
    if (requestor && CurrentIndex() != requestor) {
	if (!result)
	    IssueMessage(LOG_INFO, "Caught one; %x was going to be stored in %x",
			 requestor, CurrentIndex());
	if (!next_ent(requestor)) {
	    DoReply(-LR_ERROR, "Fatal database error.");
	    cleanup(-1);
	}
    }
    return result;
}

/*
** Check to see if an alias is reasonable
*/
static char *
BadReason[] = {
    "Alias is too long or too short.",
    "Alias field is locked and may not be changed.",
    "Aliases must begin with an alpha character, a-z",
    "Only alphanumerics and ``-'' are allowed in aliases.",
    "Alias is too common a name.",
};

char *
BadAlias(dir, alias)
    QDIR dir;
    char *alias;
{
    char *cp;
    int  len;

    if (alias == NULL)
	return (BadReason[0]);
    len = strlen(alias);
    if (len < MIN_ALIAS && !(len == 0 && AmHero) ||
	len > FindFDI(F_ALIAS)->fdMax)
	return (BadReason[0]);

#ifdef MAX_ALIAS
    if (len > MAX_ALIAS)
	return (BadReason[0]);
    if (dir != NULL && !AmOpr && !AmHero && *FINDVALUE(dir, F_LOCKED))
	return (BadReason[1]);
#endif
    if (!isalpha(*alias))
	return (BadReason[2]);
    for (cp = alias; *cp; cp++)
	if (!isalnum(*cp) && *cp != '-')
	    return (BadReason[3]);
    if (RateAKey(alias) < 0)
	return (BadReason[4]);

    return (NULL);
}
