/*
 * This program is copyright Alec Muffett 1993. The author disclaims all 
 * responsibility or liability with respect to it's usage or its effect 
 * upon hardware or computer systems, and maintains copyright as set out 
 * in the "LICENCE" document which accompanies distributions of Crack v4.0 
 * and upwards.
 */

static char vers_id[] = "fascist.c : v2.3p2 Alec Muffett 18 May 1993";

#include "protos.h"

#ifdef CRACKLIB

#include "packer.h"
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>

#define ISSKIP(x) (isspace(x) || ispunct(x))

#define MINDIFF	5
#define MINLEN 6
#define MAXSTEP 4

static char *r_destructors[] = {
    ":",			/* noop - must do this to test raw word. */

    "[",			/* trimming leading/trailing junk */
    "]",
    "[[",
    "]]",
    "[[[",
    "]]]",

    "/?p@?p",			/* purging out punctuation/symbols/junk */
    "/?s@?s",
    "/?X@?X",

    "/$s$s",			/* transmogrifying "$" back into "s", etc... */
    "/4s4a",
    "/2s2a",
    "/3s3e",
    "/0s0o",
    "/1s1i",
    "/4s4h",
    "/1s1l",
    "/$s$s/1s1l",
    "/2s2a/3s3e",
    "/0s0o/4s4h",
    "/1s1l/4s4a",
    "/2s2a/4s4h",
    "/$s$s/3s3e",
    "/$s$s/0s0o",
    "/1s1i/4s4a",
    "/$s$s/2s2a",
    "/$s$s/4s4a",
    "/0s0o/1s1l",
    "/3s3e/4s4h",
    "/1s1l/4s4h",
    "/$s$s/4s4h",
    "/1s1i/4s4h",
    "/2s2a/4s4a",
    "/$s$s/1s1i",
    "/0s0o/1s1i",
    "/1s1i/2s2a",
    "/1s1i/3s3e",
    "/0s0o/4s4a",
    "/1s1l/3s3e",
    "/3s3e/4s4a",
    "/0s0o/2s2a",
    "/0s0o/3s3e",
    "/1s1l/2s2a",
    "/0s0o/1s1l/2s2a",
    "/0s0o/1s1i/4s4h",
    "/1s1l/2s2a/4s4a",
    "/2s2a/3s3e/4s4a",
    "/0s0o/1s1i/2s2a",
    "/1s1i/2s2a/3s3e",
    "/0s0o/1s1l/4s4a",
    "/1s1l/2s2a/4s4h",
    "/$s$s/0s0o/1s1l",
    "/$s$s/0s0o/2s2a",
    "/0s0o/1s1i/3s3e",
    "/$s$s/0s0o/3s3e",
    "/1s1i/2s2a/4s4a",
    "/$s$s/0s0o/4s4a",
    "/$s$s/0s0o/4s4h",
    "/0s0o/1s1l/4s4h",
    "/1s1i/2s2a/4s4h",
    "/0s0o/1s1l/3s3e",
    "/$s$s/0s0o/1s1i",
    "/0s0o/1s1i/4s4a",
    "/2s2a/3s3e/4s4h",
    "/1s1l/2s2a/3s3e",
    "/$s$s/0s0o/1s1l/3s3e",
    "/0s0o/1s1l/2s2a/4s4a",
    "/0s0o/1s1i/2s2a/3s3e",
    "/$s$s/0s0o/1s1i/4s4a",
    "/$s$s/0s0o/1s1i/3s3e",
    "/$s$s/0s0o/1s1l/4s4a",
    "/0s0o/1s1l/2s2a/3s3e",
    "/$s$s/0s0o/1s1i/2s2a",
    "/1s1l/2s2a/3s3e/4s4a",
    "/$s$s/0s0o/1s1l/2s2a",
    "/$s$s/0s0o/1s1i/4s4h",
    "/1s1i/2s2a/3s3e/4s4a",
    "/0s0o/1s1i/2s2a/4s4a",
    "/0s0o/1s1l/2s2a/4s4h",
    "/1s1i/2s2a/3s3e/4s4h",
    "/1s1l/2s2a/3s3e/4s4h",
    "/0s0o/1s1i/2s2a/4s4h",
    "/$s$s/0s0o/1s1l/4s4h",
    "/$s$s/0s0o/1s1i/2s2a/4s4a",
    "/$s$s/0s0o/1s1l/2s2a/4s4h",
    "/$s$s/0s0o/1s1i/2s2a/4s4h",
    "/0s0o/1s1l/2s2a/3s3e/4s4a",
    "/$s$s/0s0o/1s1l/2s2a/4s4a",
    "/0s0o/1s1i/2s2a/3s3e/4s4a",
    "/0s0o/1s1i/2s2a/3s3e/4s4h",
    "/$s$s/0s0o/1s1l/2s2a/3s3e",
    "/0s0o/1s1l/2s2a/3s3e/4s4h",
    "/$s$s/0s0o/1s1i/2s2a/3s3e",
    "/$s$s/0s0o/1s1l/2s2a/3s3e/4s4h",
    "/$s$s/0s0o/1s1l/2s2a/3s3e/4s4a",
    "/$s$s/0s0o/1s1i/2s2a/3s3e/4s4a",
    "/$s$s/0s0o/1s1i/2s2a/3s3e/4s4h",
    (char *) 0
};

static char *r_constructors[] = {
    ":",
    "r",
    "d",
    "f",
    "dr",
    "fr",
    "rf",
    (char *) 0
};

static int GTry __P((char *, char *));
static char * FascistGecos __P((QDIR, char *));
static char * FascistLook __P((PWDICT *, QDIR, char *));

static int
GTry(rawtext, password)
    register char *rawtext;
    register char *password;
{
    register int i;
    register char *mp;
    int len;

    /* use destructors to turn password into rawtext */
    /* note use of Reverse() to save duplicating all rules */

    len = strlen(password);

    for (i = 0; r_destructors[i]; i++)
    {
	if (!(mp = Mangle(password, r_destructors[i])))
	{
	    continue;
	}

#ifdef DEBUG
	printf("d1->\t%s vs %s\n", mp, rawtext);
#endif
	if (!strncmp(mp, rawtext, len))
	{
	    return (1);
	}
#ifdef DEBUG
	printf("d2->\tr(%s) vs %s\n", mp, rawtext);
#endif
	if (!strncmp(Reverse(mp), rawtext, len))
	{
	    return (1);
	}
    }

    /* use constructors to turn rawtext into password */

    for (i = 0; r_constructors[i]; i++)
    {
	if (!(mp = Mangle(rawtext, r_constructors[i])))
	{
	    continue;
	}
#ifdef DEBUG
	printf("c->\t%s vs %s\n", mp, password);
#endif
	if (!strncmp(mp, password, len))
	{
	    return (1);
	}
    }

    return (0);
}

static char *
FascistGecos(dir, password)
    QDIR dir;
    char *password;
{
    int i;
    int j;
    int wc;
    char gbuffer[STRINGSIZE];
    char *uwords[STRINGSIZE];
    char buffer[STRINGSIZE];
    register char *ptr = 
        "\101\154\145\143\127\141\163\110\145\162\145";

    if (GTry(FINDVALUE(dir, F_ALIAS), password))
    {
	return ("it is based on your ph alias");
    }

    strcpy(gbuffer, FINDVALUE(dir, F_NAME));
    strcat(gbuffer, " ");
    strcat(gbuffer, FINDVALUE(dir, F_PHONE));
    strcat(gbuffer, " ");
    strcat(gbuffer, FINDVALUE(dir, F_HOMEPHONE));
    strcat(gbuffer, " ");
    strcat(gbuffer, FINDVALUE(dir, F_UNIVID));

    wc = 0;
    ptr = gbuffer;

    while (*ptr)
    {
	while (*ptr && ISSKIP(*ptr))
	{
	    ptr++;
	}

	if (ptr != gbuffer)
	{
	    ptr[-1] = '\0';
	}

	uwords[wc++] = ptr;

	if (wc == STRINGSIZE)
	{
	    uwords[--wc] = (char *) 0;	/* to hell with it */
	    break;
	} else
	{
	    uwords[wc] = (char *) 0;
	}

	while (*ptr && !ISSKIP(*ptr))
	{
	    ptr++;
	}

	if (*ptr)
	{
	    *(ptr++) = '\0';
	}
    }
#ifdef DEBUG
    for (i = 0; uwords[i]; i++)
    {
    	printf("u:\t%s\n", uwords[i]);
    }
#endif
    for (i = 0; uwords[i]; i++)
    {
	if (GTry(uwords[i], password))
	{
	    return ("it is based upon your ph entry");
	}
    }

    for (j = 1; uwords[j]; j++)
    {
	for (i = 0; i < j; i++)
	{
	    strcpy(buffer, uwords[i]);
	    strcat(buffer, uwords[j]);
	    if (GTry(buffer, password))
	    {
		return ("it is derived from your ph entry");
	    }

	    strcpy(buffer, uwords[j]);
	    strcat(buffer, uwords[i]);
	    if (GTry(buffer, password))
	    {
		return ("it's derived from your ph entry");
	    }

	    buffer[0] = uwords[i][0];
	    buffer[1] = '\0';
	    strcat(buffer, uwords[j]);
	    if (GTry(buffer, password))
	    {
		return ("it is derivable from your ph entry");
	    }

	    buffer[0] = uwords[j][0];
	    buffer[1] = '\0';
	    strcat(buffer, uwords[i]);
	    if (GTry(buffer, password))
	    {
		return ("it's derivable from your ph entry");
	    }
	}
    }

    return ((char *) 0);
}

static char *
FascistLook(pwp, dir, instring)
    PWDICT *pwp;
    QDIR dir;
    char *instring;
{
    register int i;
    register char *ptr;
    char junk[STRINGSIZE];
    register char *jptr;
    char rpassword[STRINGSIZE];
    register char *password;
    int32 notfound;

    notfound = PW_WORDS(pwp);
    password = rpassword;

    strncpy(password, instring, STRINGSIZE);

    password[STRINGSIZE - 1] = '\0';

    if (strlen(password) < 4)
    {
	return ("it's WAY too short");
    }

    if (strlen(password) < MINLEN)
    {
	return ("it is too short");
    }

    jptr = junk;
    *jptr = '\0';

    for (i = 0; i < STRINGSIZE && password[i]; i++)
    {
	if (!strchr(junk, password[i]))
	{
	    *(jptr++) = password[i];
	    *jptr = '\0';
	}
    }

    if (strlen(junk) < MINDIFF)
    {
	return ("it does not contain enough DIFFERENT characters");
    }

    strcpy(password, Lowercase(password));

    Trim(password);

    while (*password && isspace(*password))
    {
	password++;
    }

    if (!*password)
    {
	return ("it is all whitespace");
    }

    i = 0;
    ptr = password;
    while (ptr[0] && ptr[1])
    {
	if ((ptr[1] == (ptr[0] + 1)) || (ptr[1] == (ptr[0] - 1)))
	{
	    i++;
	}
	ptr++;
    }

    if (i > MAXSTEP)
    {
	return ("it is too simplistic/systematic");
    }

    if (PMatch("aadddddda", password))	/* smirk */
    {
	return ("it looks like a National Insurance number.");
    }

    if (ptr = FascistGecos(dir, password))
    {
	return (ptr);
    }

    for (i = 0; r_destructors[i]; i++)
    {
	char *a;
	if (!(a = Mangle(password, r_destructors[i])))
	{
	    continue;
	}
	if (FindPW(pwp, a) != notfound)
	{
	    return ("it is based on a dictionary word");
	}
    }

    strcpy(password, Reverse(password));

    for (i = 0; r_destructors[i]; i++)
    {
	char *a;

	if (!(a = Mangle(password, r_destructors[i])))
	{
	    continue;
	}

	if (FindPW(pwp, a) != notfound)
	{
	    return ("it is based on a (reversed) dictionary word");
	}
    }

    return ((char *) 0);
}

char *
FascistCheck(dir, password, path)
    QDIR dir;
    char *password;
    char *path;
{
    static PWDICT *pwp;
    static char lastpath[STRINGSIZE];
    char *why;

    if (pwp && strncmp(lastpath, path, STRINGSIZE))
    {
    	PWClose(pwp);
    	pwp = (PWDICT *)0;
    }

    if (!pwp)
    {
	if (!(pwp = PWOpen(path, "r")))
	{
	    perror("PWOpen");
	    exit(-1);
	}
	strncpy(lastpath, path, STRINGSIZE);
    }

    if ((why = FascistLook(pwp, dir, password)) != NULL)
	IssueMessage(LOG_INFO, "%s's new password %s rejected as %s",
		FINDVALUE(dir, F_ALIAS), password, why);
    return (why);
}
#endif /* CRACKLIB */
