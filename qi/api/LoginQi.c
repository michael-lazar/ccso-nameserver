/*
 * Copyright (c) 1985 Corporation for Research and Educational Networking
 * Copyright (c) 1988 University of Illinois Board of Trustees, Steven
 *              Dorner, and Paul Pomes
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
 *      This product includes software developed by the Corporation for
 *      Research and Educational Networking (CREN), the University of
 *      Illinois at Urbana, and their contributors.
 * 4. Neither the name of CREN, the University nor the names of its
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
static char RcsId[] = "@(#)$Id: LoginQi.c,v 1.29 1995/06/27 02:20:33 p-pomes Exp p-pomes $";
#endif

/*
 * Login and Logout functions, using the many flavors of password
 *  protocols (original recipe, Kerberos, email etc.)
 *
 *
 * LoginQi - Login to QI server, optionally prompting for username/password.
 *
 *   Parameters:
 *           UseHost - name of Qi server host
 *           ToQI - stream descriptor to write to
 *           FromQI - stream descriptor to read from
 *           Options - see qiapi.h/LQ_* defines
 *           Username - pointer to name to login as (alias) or NULL
 *           Password - pointer to password or NULL
 *
 *   Returns:
 *           alias logged in as or NULL.
 *
 *   Side Effects:
 *           possibly obtains and caches Kerberos tickets.
 *           username/password prompts are written/read to/from stdin/out,
 *            iff Options&LQ_INTERACTIVE.
 *
 * (most of this code lifted out of ph 6.5)
 */

#include <syslog.h>

#ifdef __STDC__
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#else /* !__STDC__ */
#include <strings.h>
char *malloc();
char *getenv();
char *strtok();

#endif /* __STDC__ */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#ifdef SYSV
#include <fcntl.h>
#endif
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/param.h>
#include <errno.h>
#include "conf.h"
#include "qiapi.h"
#ifdef KRB4_AUTH
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif /* KRB4_AUTH */

#ifdef KRB5_AUTH
#include <krb5/krb5.h>
#include <krb5/krb5_err.h>
#include <krb5/ccache.h>
#endif /* KRB5_AUTH */

char *getpass __P((const char *));

#ifndef NAMEPROMPT
#define NAMEPROMPT "Enter nameserver alias: "
#endif
#ifndef PASSPROMPT
#define PASSPROMPT "Enter nameserver password: "
#endif
#ifndef CLIENT
#define CLIENT "ph"
#endif

#define MAXSTR		255	/*max string length */

int  QiDebug = 0;
int  QiAuthDebug = 0;
static char MsgBuf[MAXSTR];	/*messages from qi */
char *AuthMethods = NULL;

#ifdef FWTK_AUTH
static int LoginFwtk __P((const char *, FILE *, FILE *, int, char *, char **, char **));
#endif /* FWTK_AUTH */

#ifdef KRB4_AUTH
static int LoginKrb4 __P((const char *, FILE *, FILE *, int, char *, char **, char **));
#endif /* KRB4_AUTH */

#ifdef KRB5_AUTH
static int LoginKrb5 __P((const char *, FILE *, FILE *, int, char *, char **, char **));
#endif /* KRB5_AUTH */

#ifdef GSS_AUTH
static int LoginGss __P((const char *, FILE *, FILE *, int, char *, char **, char **));
#endif /* GSS_AUTH */

static int LoginEmail __P((const char *, FILE *, FILE *, int, char *, char **, char **));
static int LoginOriginal __P((const char *, FILE *, FILE *, int, char *, char **, char **));
static void GetAutoLogin __P((char **, char **));
static void SkipMacdef __P((FILE *));
static int CheckAuth __P((FILE *, FILE *));

/*
 * try each kind of login protocol in turn 'til one succeeds or we run
 * out of choices.  To avoid reprompting for Username or Password, it
 * is the responsiblity of each routine to malloc up the result of
 * obtaining the username/password, and the responsibility of this
 * routine to clean them up -- unless they were passed in from caller of
 * course.
 */
char *
LoginQi(UseHost, ToQI, FromQI, Options, Username, Password)
    const char *UseHost;
    FILE *ToQI, *FromQI;
    int  Options;
    const char *Username, *Password;
{
    char *U = (char *) Username, *P = (char *) Password;
    static char MyAlias[MAXSTR];
    int LoggedIn, code;
    char *pnt;

    if (!AuthMethods && (code = CheckAuth(ToQI, FromQI)) != LR_OK)
	return (NULL);
    if (QiAuthDebug) {
	fprintf(stderr, "Options %#x, Username %s, Password %s\n", Options,
	    (Username)?Username:"(nil)", (Password)?Password:"(nil)");
	fprintf(stderr, "AuthMethods: %s\n", AuthMethods);
    }
    memset(MyAlias, 0, sizeof MyAlias);
    for (LoggedIn = 0, pnt = AuthMethods; pnt && *pnt && !LoggedIn; ) {
	if (QiAuthDebug)
	    fprintf(stderr, "LoginQi:Trying method %d\n", atoi(pnt));
	switch (code = atoi(pnt)) {
	  case LQ_FWTK:
#if defined(FWTK_AUTH)
	    if (Options & LQ_FWTK &&
		LoginFwtk(UseHost, ToQI, FromQI, Options, MyAlias, &U, &P)
		== LR_OK)
		    LoggedIn++;
#endif /* FWTK_AUTH */
	    break;

	  case LQ_KRB5:
#if defined(KRB5_AUTH)
	    if (Options & LQ_KRB5 &&
		LoginKrb5(UseHost, ToQI, FromQI, Options, MyAlias, &U, &P)
		== LR_OK)
		    LoggedIn++;
#endif /* KRB5_AUTH */
	    break;

	  case LQ_GSS:
#ifdef GSS_AUTH
	    if (Options & LQ_GSS &&
		LoginGss(UseHost, ToQI, FromQI, Options, MyAlias, &U, &P) == LR_OK)
		    LoggedIn++;
#endif /* GSS_AUTH */
	    break;

	  case LQ_KRB4:
#if defined(KRB4_AUTH)
	    if (Options & LQ_KRB4 &&
		LoginKrb4(UseHost, ToQI, FromQI, Options, MyAlias, &U, &P)
		== LR_OK)
		    LoggedIn++;
#endif /* KRB4_AUTH */
	    break;

	  case LQ_PASSWORD:
	    if (Options & LQ_PASSWORD &&
		LoginOriginal(UseHost, ToQI, FromQI, Options, MyAlias, &U, &P) == LR_OK)
		    LoggedIn++;
	    break;

	  case LQ_EMAIL:
	    if (Options & LQ_EMAIL &&
		LoginEmail(UseHost, ToQI, FromQI, Options, MyAlias, &U, &P) == LR_OK)
		    LoggedIn++;
	    break;

	  case LQ_CLEAR:
	    break;

	  default:
	    syslog(LOG_ERR, "LoginQi:Unknown authentication method %d ignored", code);
	    fprintf(stderr, "LoginQi:Unknown authentication method %d ignored\n", code);
	    break;
	}
	pnt = strchr(pnt, ':');
	if (pnt && *pnt)
	    pnt++;
    }
    if (!Username && U)		/* username was not passed in */
	free(U);		/* so free malloc'd string */
    if (!Password && P) {	/* ditto for password */
	memset(P, 0, strlen(P));
	free(P);
    }
    fputs(MsgBuf, stdout);
    return ((*MyAlias) ? MyAlias : NULL);
}


/*
 * Original recipe login, based on shared secret (password) between QI
 * server and user.  If autologin is selected, .netrc is tried first.
 * If LoginQiEmailAuth is true, then email auth is attempted.
 */
static int LoginQiEmailAuth = 0;	/* a dirty little secret */

static int
LoginOriginal(UseHost, ToQI, FromQI, Options, MyAlias, Up, Pp)
    const char *UseHost;
    FILE *ToQI, *FromQI;
    int  Options;
    char *MyAlias, **Up, **Pp;
{
    int  code;
    char *pnt, scratch[MAXSTR];

    /*
     * If LQ_AUTO option selection and a username is not supplied,
     * try getting the login info from .netrc
     */
    if (Options & LQ_AUTO && !*Up) {	/* try autologin w/.netrc */
	GetAutoLogin(Up, Pp);
	if (QiAuthDebug)
	    fprintf(stderr, "autologin: .netrc user=%s, pass=%s\n",
		    (*Up) ? *Up : "(none)", (*Pp) ? *Pp : "(none)");
    }
    if (!*Up) {			/* username not supplied */
	if (!(Options & LQ_INTERACTIVE))	/* sorry, I can't ask you. */
	    return (LR_ERROR);
	printf(NAMEPROMPT);	/* ask for missing alias */
	fgets(scratch, sizeof (scratch), stdin);
	scratch[strlen(scratch) - 1] = '\0';	/* zap the \n */
	if (!*scratch)
	    return (LR_ERROR);
	*Up = strdup(scratch);
    }
    if (*Pp && **Pp == '\0')
	*Pp = NULL;
    if (!*Pp && !LoginQiEmailAuth && !(Options & LQ_INTERACTIVE))
	return (LR_ERROR);	/* I can't ask your password */
    if (QiAuthDebug)
	fprintf(stderr, "sent=login %s\n", *Up);	/*send login request */
    if (fprintf(ToQI, "login %s\n", *Up) == EOF) {
	syslog(LOG_ERR, "LoginOriginal: fprintf: %m");
	fprintf(stderr, "LoginOriginal: Whoops--the nameserver died.\n");
	return LR_ERROR;
    }
    fflush(ToQI);

    for (;;) {			/*read the response */
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginOriginal: Whoops--the nameserver died.\n");
	    return LR_ERROR;
	}
	code = atoi(MsgBuf);

	/* intermediate or strange response */
	if (code != LR_LOGIN && code != LR_XLOGIN)
	    fputs(MsgBuf, stdout);
	if (code >= LR_OK)	/*final response */
	    break;
    }

    if (code == LR_LOGIN || code == LR_XLOGIN) {
	if (LoginQiEmailAuth) {	/* try email login */
	    pnt = getpwuid(getuid())->pw_name;

	    if (QiAuthDebug)
		fprintf(stderr, "sent=email %s\n", pnt);
	    fprintf(ToQI, "email %s\n", pnt);
	}
	else
	{
	    if (!*Pp) {		/* password not supplied */
		char *newp;

		if (code == LR_XLOGIN)
		    pnt = strchr(MsgBuf, ':') + 1;
		else
		    pnt = PASSPROMPT;
		newp = getpass(pnt);
		if (newp && *newp)
		    *Pp = strdup(newp);
	    }
	    if (strlen(*Pp) > PH_PW_LEN) {
		char *cp = &(*Pp)[PH_PW_LEN];

		while (*cp)
		    *cp++ = '\0';	/* null out *all* the extras */
	    }
#ifdef PRE_ENCRYPT
	    crypt_start(crypt(*Pp,*Pp));
#else
	    crypt_start(*Pp);
#endif

	    /*encrypt the challenge with the password */
	    MsgBuf[strlen(MsgBuf) - 1] = '\0';	/*strip linefeed */
	    scratch[encryptit(scratch, (char *) strchr(MsgBuf, ':') + 1)] = '\0';

	    /*send the encrypted text to qi */
	    if (QiAuthDebug)
		fprintf(stderr, "sent=answer %s\n", scratch);
	    fprintf(ToQI, "answer %s\n", scratch);
	}
    }
    fflush(ToQI);

    /*get the final response */
    for (;;) {
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginOriginal: Whoops--the nameserver died.\n");
	    return LR_ERROR;
	}
	code = atoi(MsgBuf);
	if (code >= LR_OK)	/*final response */
	    break;
    }

    if (code == LR_OK) {	/*logged in */
	strcpy(MyAlias, (char *) strchr(MsgBuf, ':') + 1);
	*(char *) strchr(MyAlias, ':') = '\0';
    }
    else
	*MyAlias = '\0';
    return (code);
}
/*
 * check .netrc to for username and password to try to login with.
 */
static void
GetAutoLogin(alias, pw)
    char **alias, **pw;		/* filled in from .netrc */
{
    FILE *netrc;		/*the .netrc file */
    char path[1024];		/*pathname of .netrc file */
    struct stat statbuf;	/*permissions, etc. of .netrc file */
    char key[80], val[80];	/*line from the .netrc file */
    char *token;		/*token (word) from the line from the .netrc file */

    /*
     * manufacture the pathname of the user's .netrc file
     */
    sprintf(path, "%s/.netrc", getenv("HOME"));

    /*
     * make sure its permissions are ok
     */
    if (stat(path, &statbuf) < 0)
	return;
    if (statbuf.st_mode & 077)
	return;			/*refuse insecure files */

    /*
     * try to open it
     */
    if (!(netrc = fopen(path, "r")))
	return;

    /*
     * look for a ``machine'' named ``ph''
     */
    while (2 == fscanf(netrc, "%s %s", key, val)) {
	if (!strcmp(key, "machine") && !strcmp(val, CLIENT)) {
	    /*
	     * found an entry for ph.  look now for other items
	     */
	    while (2 == fscanf(netrc, "%s %s", key, val)) {
		if (!strcmp(key, "machine"))	/*new machine */
		    goto out;
		else if (!strcmp(key, "login"))
		    *alias = strdup(val);
		else if (!strcmp(key, "password"))
		    *pw = strdup(val);
		else if (!strcmp(key, "macdef"))
		    SkipMacdef(netrc);
	    }
	}
	else if (!strcmp(key, "macdef"))
	    SkipMacdef(netrc);
    }
  out:
    return;
}

/*
 * skip a macdef in the .netrc file
 */
static void
SkipMacdef(netrc)
    FILE *netrc;
{
    int  c, wasNl;

    for (wasNl = 0; (c = getc(netrc)) != EOF; wasNl = (c == '\n'))
	if (wasNl && c == '\n')
	    break;
}

#ifdef FWTK_AUTH
/*
 * Use the authentication server from the TIS Firewall Toolkit.  Properly
 * built it provides SNK/4, SecureId, S/Key, and other methods.
 */
static int
LoginFwtk(UseHost, ToQI, FromQI, Options, MyAlias, Up, Pp)
    const char *UseHost;
    FILE *ToQI, *FromQI;
    int  Options;
    char *MyAlias, **Up, **Pp;
{
    int  code;
    char *pnt, *newp;
    char scratch[MAXSTR];

    /* if LQ_AUTO option selection and a username is not supplied,
     * try getting the login info from .netrc
     */
    if (!(Options & LQ_INTERACTIVE))	/* only interactive use is possible */
	return (LR_ERROR);

    if (!*Up) {			/* username not supplied */
	printf(NAMEPROMPT);	/* ask for missing alias */
	fgets(scratch, sizeof (scratch), stdin);
	scratch[strlen(scratch) - 1] = '\0';	/* zap the \n */
	if (!*scratch)
	    return (LR_ERROR);
	*Up = strdup(scratch);
    }
    if (QiAuthDebug)
	fprintf(stderr, "sent=xlogin %d %s\n", LQ_FWTK, *Up);
    if (fprintf(ToQI, "xlogin %d %s\n", LQ_FWTK, *Up) == EOF) {
	syslog(LOG_ERR, "LoginFwtk: fprintf: %m");
	fprintf(stderr, "LoginFwtk: Whoops--the nameserver died.\n");
	return LR_ERROR;
    }

    fflush(ToQI);

    for (;;) {			/*read the response */
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginFwtk: Whoops--the nameserver died.\n");
	    return LR_ERROR;
	}
	code = atoi(MsgBuf);

	/* intermediate or strange response */
	if (code != LR_LOGIN && code != LR_XLOGIN)
	    fputs(MsgBuf, stdout);
	if (code >= LR_OK)	/*final response */
	    break;
    }

    /*
     * Ignore passed in password because SNK/4, SecureId, S/Key all require
     * a password calculated from a challenge.  Well, that's not exactly
     * true with S/Key, however S/Key doesn't use reuseable passwords.
     */
    if (code == LR_XLOGIN) {
	if ((pnt = strchr(MsgBuf, '\n')) != NULL)
	    *pnt = '\0';
	pnt = strchr(MsgBuf, ':') + 1;
    }
    else
	pnt = PASSPROMPT;
    newp = getpass(pnt);

    /* send the response to qi */
    if (QiAuthDebug)
	fprintf(stderr, "sent=answer %s\n", newp);
    fprintf(ToQI, "answer %s\n", newp);
    fflush(ToQI);

    /*get the final response */
    for (;;) {
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginFwtk: Whoops--the nameserver died.\n");
	    return LR_ERROR;
	}
	code = atoi(MsgBuf);
	if (code >= LR_OK)	/*final response */
	    break;
    }

    if (code == LR_OK) {	/*logged in */
	strcpy(MyAlias, (char *) strchr(MsgBuf, ':') + 1);
	*(char *) strchr(MyAlias, ':') = '\0';
    }
    else
	*MyAlias = '\0';
    return (code);
}
#endif /* FWTK_AUTH */

#ifdef KRB4_AUTH
/*
 * Extra-Krispy recipe, using a trusted third party with a strange name.
 */
static int
LoginKrb4(UseHost, ToQI, FromQI, Options, MyAlias, Up, Pp)
    const char *UseHost;
    FILE *ToQI, *FromQI;
    int  Options;
    char *MyAlias, **Up, **Pp;
{
    struct sockaddr_in sin, lsin;
    int  sock = fileno(ToQI);
    int  namelen;
    int  code, retval;
    char SrvHost[MAXSTR];
    static char scratch[MAXSTR];
    static char kpass[MAXSTR];
    KTEXT_ST ticket;
    INT32 authopts;
    MSG_DAT msg_data;
    CREDENTIALS cred;
    Key_schedule sched;
    char principal[ANAME_SZ];
    char instance[INST_SZ];
    char realm[REALM_SZ], *hrealm, *pnt;
    char krbtkfile[MAXPATHLEN];
    char okrbtkfile[MAXPATHLEN];

    *principal = *instance = *realm = '\0';
    (void) strcpy(SrvHost, UseHost);
    if (hrealm = strchr(SrvHost, '.'))
	*hrealm = '\0';

    /* find out who I am */
    namelen = sizeof (lsin);
    if (getsockname(sock, (struct sockaddr *) &lsin, &namelen) < 0) {
	return (LR_ERROR);
    }

    /* find out who the other side is */
    namelen = sizeof (sin);
    if (getpeername(sock, (struct sockaddr *) &sin, &namelen) < 0) {
	return (LR_ERROR);
    }

    /*
     * Did the user specify a username?  Has autologin been requested?
     * If not, and if we're not logged in to Kerberos, prompt for one.
     */
    if (!*Up) {
	struct stat dummy;

	if (!(Options & LQ_AUTO))	/* no user, no autologin */
	    return (LR_ERROR);	/* no deal */
	if (stat(TKT_FILE, &dummy)) {	/* no ticket cache */
	    if (!(Options & LQ_INTERACTIVE))	/* can't ask */
		return (LR_ERROR);
	    printf(NAMEPROMPT);
	    fgets(scratch, sizeof (scratch), stdin);
	    if (!*scratch)
		return (LR_ERROR);
	    else {
		/* zap newline */
		scratch[strlen(scratch) - 1] = 0;
		*Up = strdup(scratch);
	    }
	}
    }
    /* If we're not already logged in with Kerberos then do so (get a TGT).
     * (NULL username at this point implies we already have a TGT).
     */
    if (*Up) {
	if ((pnt = strchr(*Up, '/')) != NULL)
	    *pnt = '.';		/* convert V5 principal/instance to V4 format */
	retval = kname_parse(principal, instance, realm, *Up);
	if (pnt && *pnt)
	    *pnt = '/';
	if (retval != KSUCCESS) {
	    fprintf(stderr, "LoginKrb4: %s\n", krb_err_txt[retval]);
	    return LR_ERROR;
	}
	if (!*realm && krb_get_lrealm(realm, 1)) {
	    fprintf(stderr, "LoginKrb4: Unable to get realm.\n");
	    return LR_ERROR;
	}
	/* set tkt file we'll use */
	strcpy(okrbtkfile, TKT_FILE);
	sprintf(krbtkfile, "/tmp/tkt_ph4_%d", getpid());
	krb_set_tkt_string(krbtkfile);

	if (*Pp && **Pp == '\0')
	    *Pp = NULL;
	if (!*Pp) {		/* no password supplied */
	    if (!(Options & LQ_INTERACTIVE))	/* I can't ask */
		return LR_ERROR;

	    /* Read the password string, krb_get_pw_in_tkt() will convert to
	     * key.
	     */
	    if (des_read_pw_string(kpass, sizeof (kpass),
				    "Enter kerberos password: ", 0) != 0) {
		fprintf(stderr, "LoginKrb4: Unable to read password.\n");
		return LR_ERROR;
	    }
	    if (*kpass)
		*Pp = strdup(kpass);
	    memset(kpass, 0, sizeof(kpass));
	    (void) sprintf(kpass, "%ld", time(0));
	}
	/* login */
	retval = krb_get_pw_in_tkt(principal, instance, realm,
				   "krbtgt", realm, 96,
				   (*Pp == NULL || **Pp == '\0') ? kpass : *Pp);
	if (QiAuthDebug)
	    fprintf(stderr, "%s getting V4 Kerberos TGT for %s.%s@%s.\n",
		    (retval == KSUCCESS) ? "Success" : "Failure",
		    principal, (instance) ? instance : "(nil)", realm);
	if (retval != KSUCCESS) {
	    if (*Up) {
		krb_set_tkt_string(okrbtkfile);
	    }
	    return LR_ERROR;
	}
    }

    /* Read principal name from ticket cache if needed */
    if (!*principal) {
	if ((retval = tf_init(TKT_FILE, R_TKT_FIL)) != KSUCCESS) {
	    syslog(LOG_ERR, "LoginKrb4: tf_init(%s): %s",
		    TKT_FILE, krb_err_txt[retval]);
	    fprintf(stderr, "LoginKrb4: tf_init(%s): %s",
		    TKT_FILE, krb_err_txt[retval]);
	    return LR_ERROR;
	}
	if ((retval = tf_get_pname(principal)) != KSUCCESS) {
	    syslog(LOG_ERR, "LoginKrb4: tf_get_pname(): %s",
		    krb_err_txt[retval]);
	    fprintf(stderr, "LoginKrb4: tf_get_pname(): %s",
		    krb_err_txt[retval]);
	    return LR_ERROR;
	}
    }
    if (QiAuthDebug)
	fprintf(stderr, "sent=xlogin %d %s\n", LQ_KRB4, principal);
    if (fprintf(ToQI, "xlogin %d %s\n", LQ_KRB4, principal) == EOF) {
	syslog(LOG_ERR, "LoginKrb4: fprintf: %m");
	fprintf(stderr, "LoginKrb4: Whoops--the nameserver died.\n");
	return LR_ERROR;
    }
    fflush(ToQI);

    for (;;) {			/* read the response */
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginKrb4: Whoops--the nameserver died.\n");
	    if (*Up)
		dest_tkt();	/* destroy temp tickets for
				 * specified username */
	    return (LR_ERROR);
	}
	code = atoi(MsgBuf);

	/* intermediate or strange response */
	if (code != LR_LOGIN && code != LR_XLOGIN)
	    fputs(MsgBuf, stdout);
	if (code >= LR_OK)	/* final response */
	    break;
    }

    if (code == LR_LOGIN || code == LR_XLOGIN) {
	/*
	 * call Kerberos library routine to obtain an authenticator,
	 * pass it over the socket to the server, and obtain mutual
	 * authentication.
	 */

#ifdef KRBNSREALM
	hrealm = KRBNSREALM;
#else
	hrealm = krb_realmofhost(UseHost);
#endif
	authopts = KOPT_DO_MUTUAL;
	retval = krb_sendauth(authopts, sock, &ticket,
			      KRB4SRV, SrvHost, hrealm,
			      0, &msg_data, &cred,
			      sched, &lsin, &sin, "VERSION9");
	if (QiAuthDebug)
	    fprintf(stderr, "%s doing V4 Kerberos mutual authentication of %s.%s@%s with %s.%s@%s\n",
		(retval == KSUCCESS) ? "Success" : "Failure",
		cred.pname, (*cred.pinst) ? cred.pinst : "(nil)", cred.realm,
		KRB4SRV, SrvHost, hrealm);
	if (*Up)		/* ???? */
	    dest_tkt();		/* destroy special tickets as soon as
				 * possible */

	/* get the final response (even if mutual failed) */
	for (;;) {
	    if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
		fprintf(stderr, "LoginKrb4: Whoops--the nameserver died.\n");
		return LR_ERROR;
	    }
	    code = atoi(MsgBuf);
	    if (code >= LR_OK)	/* final response */
		break;
	}

	if (*Up) {
	    krb_set_tkt_string(okrbtkfile);
	}

	if (retval == KSUCCESS && code == LR_OK) {	/* logged in */
	    strcpy(MyAlias, (char *) strchr(MsgBuf, ':') + 1);
	    *(char *) strchr(MyAlias, ':') = '\0';
	}
	else
	    *MyAlias = '\0';
	return (code);
    }
}
#endif /* KRB4_AUTH */

#ifdef KRB5_AUTH

static krb5_data tgtname = {
    0,
    KRB5_TGS_NAME_SIZE,
    KRB5_TGS_NAME
};

/*
 * Try no preauthentication first; then try the encrypted timestamp
 */
int preauth_search_list[] = {
    0,
    KRB5_PADATA_ENC_UNIX_TIME,
    -1
};


/*
 * Extra-Krispy recipe, using a trusted third party with a strange name.
 */
static int
LoginKrb5(UseHost, ToQI, FromQI, Options, MyAlias, Up, Pp)
    const char *UseHost;
    FILE *ToQI, *FromQI;
    int  Options;
    char *MyAlias, **Up, **Pp;
{
    int  code, retval;
    int  sock = fileno(ToQI);
    char scratch[MAXSTR];
    char cname[MAXSTR], pname[MAXSTR];
    static char kpass[MAXSTR];
    int kpasslen = sizeof(kpass);
    char cache_name[MAXSTR];
    krb5_ccache cache = NULL;
    krb5_creds my_creds;
    krb5_principal me;
    krb5_error *err_ret;
    krb5_ap_rep_enc_part *rep_ret;
    krb5_address **my_addresses;
    int i, options = 0;
    krb5_timestamp now;
    char *pnt, *princ = NULL, realm[MAXSTR];

    *cname = *pname = '\0';

    if (retval = krb5_cc_default(&cache)) {
	syslog(LOG_ERR, "LoginKrb5: krb5_cc_default(): %s",
		error_message(retval));
	fprintf(stderr, "LoginKrb5: krb5_cc_default(): %s\n",
		error_message(retval));
	return (LR_ERROR);
    }

    memset ((char*)&my_creds, 0, sizeof(my_creds));
    if (retval = krb5_sname_to_principal(UseHost, KRB5SRV, KRB5_NT_SRV_HST,
					&my_creds.server)) {
	syslog(LOG_ERR, "LoginKrb5:krb5_sname_to_principal(%s,%s): %s.",
	    UseHost, KRB5SRV, error_message(retval));
	fprintf(stderr, "LoginKrb5:krb5_sname_to_principal(%s,%s): %s.\n",
	     UseHost, KRB5SRV, error_message(retval));
	return (LR_ERROR);
    }
    (void) krb5_unparse_name(my_creds.server, &pnt);
    (void) strcpy(cname, pnt);
    free(pnt);

    /*
     * Strategy: determine if ticket cache exists.  If it does and the
     * tickets are valid, use them to log in.  If ticket cache doesn't
     * exist, create a temporary cache, prompt for username and password if
     * need be, and send Kerberos authentication.
     */

#ifdef KRBNSREALM
    (void) strcpy(realm, KRBNSREALM);
#else
    if (retval = krb5_get_default_realm(&pnt)) {
	syslog(LOG_ERR, "LoginKrb5: krb5_get_default_realm(): %s",
		error_message(retval));
	fprintf(stderr, "LoginKrb5:  krb5_get_default_realm(): %s\n",
		error_message(retval));
	krb5_free_principal(my_creds.server);
	return (LR_ERROR);
    }
    (void) strncpy(realm, pnt, MAXSTR-1);
    free(pnt);
#endif

    /*
     * If user has previously done a kinit, then krb5_cc_get_principal()
     * will succeed.  The ticket obtained may have timed out so be prepared
     * to handle that after  krb5_get_credentials().
     */
    if (retval = krb5_cc_get_principal(cache, &my_creds.client)) {

	/* No credentials cache. */
	if (!*Up && !(Options & LQ_INTERACTIVE)) {	/* can't ask */
		krb5_free_principal(my_creds.server);
		return (LR_ERROR);
	}
	if (!*Up) {
		printf(NAMEPROMPT);
		fgets(scratch, sizeof (scratch), stdin);
		if (!*scratch) {
		    krb5_free_principal(my_creds.server);
		    return (LR_ERROR);
		}
		/* zap newline */
		scratch[strlen(scratch) - 1] = 0;
		*Up = strdup(scratch);
	}

	/* convert V4 principal/instance to V5 format */
	if ((pnt = strchr(*Up, '.')) != NULL) {
	    char *at = strchr(*Up, '@');

	    if (at) {
		if (at > pnt)
		    *pnt = '/';
		else
		    pnt = NULL;
	    }
	    else
		*pnt = '/';
	}

	/* create the principal to ask for */
	if (retval = krb5_parse_name (*Up, &me)) {
	    syslog(LOG_ERR, "LoginKrb5: krb5_parse_name(%s): %s",
		*Up, error_message(retval));
	    fprintf(stderr, "LoginKrb5:  krb5_parse_name(%s): %s\n",
		*Up, error_message(retval));
	    krb5_free_principal(my_creds.server);
	    return (LR_ERROR);
	}
	if (pnt && *pnt)
	    *pnt = '.';
	(void) krb5_unparse_name(me, &pnt);
	(void) strcpy(pname, pnt);
	free(pnt);

	/* Determine address(es) of client host */
	if (retval = krb5_os_localaddr(&my_addresses)) {
	    syslog(LOG_ERR, "LoginKrb5: krb5_os_localaddr(): %s",
		    error_message(retval));
	    fprintf(stderr, "LoginKrb5:  krb5_os_localaddr(): %s\n",
		    error_message(retval));
	    krb5_free_principal(my_creds.server);
	    krb5_free_principal(me);
	    return (LR_ERROR);
	}

	/* What time is it? */
	if (retval = krb5_timeofday(&now)) {
	    syslog(LOG_ERR, "LoginKrb5:  krb5_timeofday(): %s",
		    error_message(retval));
	    fprintf(stderr, "LoginKrb5:   krb5_timeofday(): %s\n",
		    error_message(retval));
	    krb5_free_principal(my_creds.server);
	    krb5_free_principal(me);
	    krb5_free_addresses(my_addresses);
	    return (LR_ERROR);
	}

	/* create a temporary credentials cache */
	(void) sprintf(cache_name, "FILE:/tmp/tkt_ph5_%d", getpid());
	if ((retval = krb5_cc_resolve(cache_name, &cache))) {
	    syslog(LOG_ERR, "LoginKrb5: krb5_cc_resolve(%s): %s",
		    cache_name, error_message(retval));
	    fprintf(stderr, "LoginKrb5:  krb5_cc_resolve(%s): %s\n",
		    cache_name, error_message(retval));
	    krb5_free_principal(my_creds.server);
	    krb5_free_principal(me);
	    krb5_free_addresses(my_addresses);
	    return (LR_ERROR);
	}

	/* make the principal the primary cache entry */
	if (retval = krb5_cc_initialize(cache, me)) {
	    syslog(LOG_ERR, "LoginKrb5: krb5_cc_initialize(%s): %s",
		pname, error_message(retval));
	    fprintf(stderr, "LoginKrb5: krb5_cc_initialize(%s): %s\n",
		pname, error_message(retval));
	    krb5_free_principal(my_creds.server);
	    krb5_free_principal(me);
	    krb5_free_addresses(my_addresses);
	    return (LR_ERROR);
	}

	my_creds.client = me;
	my_creds.times.starttime = 0;
	my_creds.times.endtime = now + ((QiAuthDebug) ? 3600 : 60);
	my_creds.times.renew_till = 0;

	if (*Pp && **Pp == '\0')
	    *Pp = NULL;
	if (!*Pp) {		/* no password supplied */
	    if (!(Options & LQ_INTERACTIVE)) {	/* I can't ask */
		krb5_free_principal(my_creds.server);
		krb5_free_principal(me);
		krb5_free_addresses(my_addresses);
		return LR_ERROR;
	    }
	    if (krb5_read_password("Enter Kerberos password: ", 0,
				    kpass, &kpasslen) != 0) {
		fprintf(stderr, "Unable to read password.\n");
		krb5_free_principal(my_creds.server);
		krb5_free_principal(me);
		krb5_free_addresses(my_addresses);
		return LR_ERROR;
	    }
	    if (*kpass)
		*Pp = strdup(kpass);
	    memset(kpass, 0, sizeof(kpass));
	    (void) sprintf(kpass, "%d", my_creds.times.endtime);
	}

	/* Iterate through the pre-auth methods until we succeed or fail */
	for (i=0; preauth_search_list[i] >= 0; i++) {
	    retval = krb5_get_in_tkt_with_password(options, my_addresses,
						   preauth_search_list[i],
                                                   ETYPE_DES_CBC_CRC,
                                                   KEYTYPE_DES,
				   (*Pp == NULL || **Pp == '\0') ? kpass : *Pp,
                                                   cache,
                                                   &my_creds, 0);
	    if (retval != KRB5KDC_ERR_PREAUTH_FAILED &&
                  code != KRB5KRB_ERR_GENERIC)
		break;
	}
	if (!QiAuthDebug)
	    (void) krb5_cc_destroy(cache);
	cache = NULL;
	krb5_free_addresses(my_addresses);
	if (QiAuthDebug) {
	    fprintf(stderr, "%s obtaining V5 Kerberos ticket for %s to use %s.\n",
		    (retval == 0) ? "Success" : "Failure", pname, cname);
	}
	if (retval) {
	    krb5_free_principal(my_creds.server);
	    krb5_free_principal(me);
	    if (retval != KRB5KRB_AP_ERR_BAD_INTEGRITY)
		fprintf(stderr, "LoginKrb5: krb5_get_in_tkt_with_password(): %s\n",
		    error_message(retval));
	    return (LR_ERROR);
	}
    }

    /* Get service ticket from cache or use TGT with KDC */
    else if (retval = krb5_get_credentials(0, cache, &my_creds)) {
	fprintf(stderr, "LoginKrb5: krb5_get_credentials(): %s\n",
	    error_message(retval));
	krb5_free_principal(my_creds.server);
	krb5_free_principal(me);
	return (LR_ERROR);
    }

    if (retval = krb5_unparse_name(my_creds.client, &princ)) {
	syslog(LOG_ERR, "LoginKrb5: krb5_unparse_name(): %s",
	    error_message(retval));
	fprintf(stderr, "LoginKrb5:  krb5_unparse_name(): %s\n",
	    error_message(retval));
	krb5_free_principal(my_creds.server);
	krb5_free_principal(me);
	memset ((char*)&my_creds, 0, sizeof(my_creds));
	return (LR_ERROR);
    }
    (void) strcpy(pname, princ);

    if (!*Up && princ) {
	*Up = princ;
	if (pnt = strchr(*Up, '@'))
	    *pnt = '\0';
    }

    if (QiAuthDebug)
	fprintf(stderr, "sent=xlogin %d %s\n", LQ_KRB5, *Up);
    if (fprintf(ToQI, "xlogin %d %s\n", LQ_KRB5, *Up) == EOF) {
	syslog(LOG_ERR, "LoginKrb5: fprintf: %m");
	fprintf(stderr, "LoginKrb5: Whoops--the nameserver died.\n");
	krb5_free_principal(my_creds.server);
	krb5_free_principal(me);
	memset ((char*)&my_creds, 0, sizeof(my_creds));
	return LR_ERROR;
    }
    fflush(ToQI);

    for (;;) {			/* read the response */
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginKrb5: Whoops--the nameserver died.\n");
	    krb5_free_principal(my_creds.server);
	    krb5_free_principal(me);
	    memset ((char*)&my_creds, 0, sizeof(my_creds));
	    return (LR_ERROR);
	}
	code = atoi(MsgBuf);

	/* intermediate or strange response */
	if (code != LR_LOGIN && code != LR_XLOGIN)
	    fputs(MsgBuf, stdout);
	if (code >= LR_OK)	/* final response */
	    break;
    }

    if (code == LR_LOGIN || code == LR_XLOGIN) {
	/*
	 * call Kerberos library routine to obtain an authenticator,
	 * pass it over the socket to the server, and obtain mutual
	 * authentication.
	 */

	retval = krb5_sendauth((krb5_pointer) &sock,
				KQI_VERSION,
				my_creds.client,
				my_creds.server,
				AP_OPTS_MUTUAL_REQUIRED,
				0,
				&my_creds,
				cache,
				0, 0,		/* don't need seqno or subkey */
				&err_ret,
				&rep_ret);
	if (QiAuthDebug) {
	    fprintf(stderr, "%s doing V5 Kerberos mutual authentication of %s with %s.\n",
		    (retval == 0) ? "Success" : "Failure", pname, cname);
	}
	/* krb5_free_principal(me); */	/* can't do if already had TGT */
	krb5_free_principal(my_creds.server);
	memset ((char*)&my_creds, 0, sizeof(my_creds));
	if (retval && err_ret)
	    fprintf(stderr, "LoginKrb5: %s\n", error_message(retval));

	/* get the final response (even if mutual failed) */
	for (;;) {
	    if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
		fprintf(stderr, "LoginKrb5: Whoops--the nameserver died.\n");
		return LR_ERROR;
	    }
	    code = atoi(MsgBuf);
	    if (code >= LR_OK)	/* final response */
		break;
	}

	if (retval == KSUCCESS && code == LR_OK) {	/* logged in */
	    strcpy(MyAlias, (char *) strchr(MsgBuf, ':') + 1);
	    *(char *) strchr(MyAlias, ':') = '\0';
	}
	else
	    *MyAlias = '\0';
	return (code);
    }
}
#endif /* KRB5_AUTH */

/*
 * Bean sprout recipe, using Berkeley r-command ingredients.
 * (actually just calls LoginOriginal since I stole this code
 *  out of ph....)
 */
static int
LoginEmail(UseHost, ToQI, FromQI, Options, MyAlias, Up, Pp)
    const char *UseHost;
    FILE *ToQI, *FromQI;
    int  Options;
    char *MyAlias, **Up, **Pp;
{
    int  rc;

    LoginQiEmailAuth = 1;	/* set our secret internal flag */
    if (QiAuthDebug)
	fprintf(stderr, "attempting email login.\n");
    rc = LoginOriginal(UseHost, ToQI, FromQI, Options, MyAlias, Up, Pp);
    LoginQiEmailAuth = 0;
    return rc;
}

/*
 * LogoutQi - Logout from QI server.
 *
 *   Parameters:
 *           ToQI - stream descriptor to write to
 *           FromQI - stream descriptor to read from
 *
 *   Returns:
 *           success(LR_OK) or failure indication
 *
 */

int
LogoutQi(ToQI, FromQI)
    FILE *ToQI, *FromQI;
{
    QIR *r;
    int  n;

    fprintf(ToQI, "logout\n");
    fflush(ToQI);
    if ((r = ReadQi(FromQI, &n)) == NULL)
	return LR_ERROR;
    n = r->code;

    /* Accept the memory leak to simplify standalone compilation of ph */
    /* FreeQIR(r); */
    return n;
}

static int
CheckAuth(ToQI, FromQI)
    FILE *ToQI, *FromQI;
{
    int code;
    char *pnt, sbuf[10], buf[MAXSTR];

    /* See if the server has preferences for authentication methods */
    if (QiAuthDebug)
	fprintf(stderr, "sent=siteinfo\n");
    if (fprintf(ToQI, "siteinfo\n") == EOF) {
	syslog(LOG_ERR, "LoginQi: fprintf: %m");
	fprintf(stderr, "LoginQi: Whoops--the nameserver died.\n");
	return LR_ERROR;
    }
    fflush(ToQI);

    for (;;) {			/*read the response */
	if (!GetGood(MsgBuf, MAXSTR, FromQI)) {
	    fprintf(stderr, "LoginOriginal: Whoops--the nameserver died.\n");
	    return LR_ERROR;
	}
	code = atoi(MsgBuf);
	if (pnt = strstr(MsgBuf, "authenticate")) {
	    /* skip to next ':' */
	    if (pnt = strchr(pnt, ':')) {
		if (pnt && *++pnt)
		    AuthMethods = strdup(pnt);
	    }
	}
	if (code >= LR_OK)	/*final response */
	    break;
    }
    if (AuthMethods || code != LR_OK)
	return (code);

    /*
     * If siteinfo was uninformative, build our own based on what we were
     * compiled with.  N.B., ordering here reflects policy of which login
     * methods are preferred at each site.
     */
    *buf = '\0';
#ifdef KRB5_AUTH
    (void) sprintf(sbuf, ":%d", LQ_KRB5);
    strcat(buf, sbuf);
#endif /* KRB5_AUTH */
#ifdef KRB4_AUTH
    (void) sprintf(sbuf, ":%d", LQ_KRB4);
    strcat(buf, sbuf);
#endif /* KRB4_AUTH */
#ifdef GSS_AUTH
    (void) sprintf(sbuf, ":%d", LQ_GSS);
    strcat(buf, sbuf);
#endif /* GSS_AUTH */
    (void) sprintf(sbuf, ":%d", LQ_PASSWORD);
    strcat(buf, sbuf);
    (void) sprintf(sbuf, ":%d", LQ_EMAIL);
    strcat(buf, sbuf);
#ifdef FWTK_AUTH
    (void) sprintf(sbuf, ":%d", LQ_FWTK);
    strcat(buf, sbuf);
#endif /* FWTK_AUTH */
    AuthMethods = strdup(buf+1);
    return (LR_OK);
}

/*
 * get a non-comment line from a stream
 * a comment is a line beginning with a # sign
 */
int
GetGood(str, maxc, fp)
    char *str;			/*space to put the chars */
    int  maxc;			/*max # of chars we want */

#ifdef VMS
    int  fp;			/*stream to read them from */
{
    static char Qbuf[MAXSTR + 4] = {'\0'};
    static int pos = {0},
	end = {0},
	len = {0};
    char *linp;

    for (;;) {
	if (pos >= len) {
	    len = netread(fp, Qbuf, maxc);
	    if (len <= 0)
		return (0);
	    Qbuf[len] = '\0';
	    pos = 0;
	}
	linp = strchr(Qbuf + pos, '\n');	/*find next newline char */
	if (linp == NULL)
	    end = len;		/*no newline chars left */
	else
	    end = linp - Qbuf;	/*convert pointer to index */

	strncpy(str, Qbuf + pos, end - pos + 1);
	*(str + end - pos + 1) = '\0';
	pos = end + 1;		/*save new position for next time */

	if (!*str)
#else
    FILE *fp;			/*stream to read them from */
{
    errno = 0;
    for (;;) {
	if (!fgets(str, maxc, fp))
#endif
	{
	    fputs("Oops; lost connection to server.\n", stderr);
	    exit(1);
	}
	else if (*str != '#') {
	    if (QiDebug)
		fprintf(stderr, "read =%s", str);
	    return (1);		/*not a comment; success! */
	}
    }
    /* NOTREACHED */
}

#ifdef NO_STRDUP
char *
strdup(str)
    const char *str;
{
    int  len;
    char *copy;

    len = strlen(str) + 1;
    if (!(copy = malloc((unsigned int) len)))
	return ((char *) NULL);
    memcpy(copy, str, len);
    return (copy);
}
#endif
