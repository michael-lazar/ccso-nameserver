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
static char RcsId[] = "@(#)$Id: commands.c,v 1.85 1995/06/28 20:32:53 p-pomes Exp $";
#endif /* !lint */

#include "protos.h"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef FWTK_AUTH
#include <firewall.h>			/* From the TIS Firewall Toolkit */
static Cfg	*cfp = NULL;
#endif /* FWTK_AUTH */

#ifdef KRB4_AUTH
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif /* KRB4_AUTH */

#ifdef KRB5_AUTH
#include <krb5/krb5.h>
#include <krb5/ext-proto.h>
#endif /* KRB5_AUTH */

static ARG *s_args = NULL;

static char *RandomString __P((int));
static int UserMatch __P((char *));
static void DoDelete __P((ARG *));
static void DoFields __P((ARG *));
static void DoId __P((ARG *));
static void DoInfo __P((ARG *));
static void DoLogin __P((ARG *));
static void DoAnswer __P((ARG *));
static void DoLogout __P((ARG *));
static void DoXLogin __P((ARG *));
static void DoFwtkLogin __P((char *));
static void DoGssLogin(char *);
static void DoKrbLogin __P((int, char *));
static void DoStatus __P((ARG *));
static void DumpOutput();
static void ListAllFields();
static void ListField __P((FDESC *));
static void NotImplemented __P((ARG *));
static int OkByEmail __P((QDIR, char *));
static int OkFields __P((ARG *));
static int OpenTempOut();
static void PrintCommand __P((int, ARG *));
static void SleepTil __P((INT32));

#ifdef NO_STRSTR
char *strstr __P((char *, char *));
#endif /* NO_STRSTR */

extern FILE *Input;		/* qi.c */
extern FILE *Output;		/* qi.c */
extern int InputType;		/* qi.c */
extern char Foreign[80];	/* qi.c */
extern char CommandText[];	/* language.l */
extern char *CommandSpot;	/* language.l */
extern int Daemon;		/* qi.c */
extern int Quiet;		/* qi.c */
extern int LocalUser;		/* qi.c */
extern char *DBState;		/* qi.c */
extern char Revision[];		/* version.c */

#ifdef EMAIL_AUTH
extern struct hostent TrustHp;	/* qi.c */
#endif /* EMAIL_AUTH */

FILE *TempOutput = NULL;
static char *TFile = NULL;

int  DiscardIt;
void (*CommandTable[]) __P((ARG *)) =
{
    DoQuery,
    DoChange,
    DoLogin,
    DoAnswer,
    DoLogout,
    DoFields,
    DoAdd,
    DoDelete,
    DoSet,
    DoQuit,
    DoStatus,
    DoId,
    DoHelp,
    DoAnswer,
    DoInfo,
    DoAnswer,
    DoXLogin,
    (void (*)__P((ARG *))) 0
};

QDIR User = NULL;		/* the currently logged in user */
int  AmHero = 0;		/* is currently logged in user the Hero? */
int  AmOpr = 0;			/* is currently logged in user the Operator? */
char *UserAlias = NULL;		/* the alias of the currently logged in user */
int  UserEnt = 0;		/* entry number of current user */
char *ClaimUser = NULL;		/* the identity the user has claimed, but not
				 * yet verified */
char *Challenge = NULL;		/* challenge string */
int  OkResponse = 0;		/* acceptable responses to a challenge */
int  State = S_IDLE;

/*
 * Add a value to an argument list
 */
void
AddValue(value, ttype)
    char *value;
    int  ttype;
{
    static ARG *LastArg = NULL;

    if (value == NULL)
	value = "";
    if (!s_args) {
	LastArg = s_args = FreshArg();
	LastArg->aFirst = strdup(value);
	LastArg->aType = ttype;
    }
    else {
	if (LastArg->aType == VALUE && ttype & EQUAL)
	    LastArg->aType |= ttype;
	else if (LastArg->aType & EQUAL && !(LastArg->aType & VALUE2)) {
	    LastArg->aType |= VALUE2;
	    LastArg->aSecond = strdup(value);
	}
	else {
	    LastArg->aNext = FreshArg();
	    LastArg = LastArg->aNext;
	    LastArg->aType = ttype;
	    LastArg->aFirst = strdup(value);
	}
    }
}

/*
 * Complain about extraneous junk
 */
void
Unknown(junk)
    char *junk;
{
    fprintf(Output, "%d:%s:%s\n", LR_NOCMD, junk, "Command not recognized.");
}

/*
 * Execute a command
 */
void
DoCommand(cmd)
    int  cmd;
{
#define MAX_SYSLOG 100
    char c;
    int  which = cmd == C_CLEAR ? 6 : MAX_SYSLOG;

    c = CommandText[which];
    CommandText[which] = '\0';
    *CommandSpot = '\0';
    if (*CommandText && strstr(CommandText, "password=") == NULL)
	IssueMessage(LOG_INFO, "%s", CommandText);
    CommandText[which] = c;
    if (!Quiet && (InputType == IT_FILE || InputType == IT_PIPE || OP_VALUE(ECHO_OP)))
	PrintCommand(cmd, s_args);	/* echo the cmd */

    if (Daemon && GetState()) {
	fprintf(Output, "555:Database shut off (%s).\n", DBState);
	exit(0);
    }
    if (TempOutput == NULL && !OpenTempOut()) {
	fprintf(Output, "%d:couldn't open temp file.\n", LR_INTERNAL);
	exit(0);
    }
    if (State == S_E_PENDING) {
	if (DiscardIt || cmd != C_ANSWER && cmd != C_CLEAR && cmd != C_EMAIL) {
	    DoReply(-LR_NOANSWER, "Expecting answer, clear, or email; login discarded.");
	    State = S_IDLE;
	    free(ClaimUser);
	    ClaimUser = NULL;
	    free(Challenge);
	    Challenge = NULL;
	}
    }
    if (DiscardIt)
	DoReply(LR_SYNTAX, "Command not understood.");
    else {
	/* (*CommandTable[cmd - 1]) (s_args); */
	void (*cmdP) __P((ARG *));
	
	if ((cmdP = *CommandTable[cmd - 1]) != NULL)
	    (*cmdP) (s_args);
	else
	    DoReply(LR_SYNTAX, "Command not understood.");
    }

    DumpOutput();

  done:
    FreeArgs(s_args);
    s_args = NULL;
    DiscardIt = 0;
}

/*
 * Free the argument list
 */
void
FreeArgs(arg)
    ARG *arg;
{
    ARG *narg;

    if (arg)
	for (narg = arg->aNext; arg; arg = narg) {
	    narg = arg->aNext;
	    if (arg->aFirst)
		free(arg->aFirst);
	    if (arg->aSecond)
		free(arg->aSecond);
	    free((char *) arg);
	    arg = NULL;
	}
}

/*
 * create a fresh argument structure
 */
ARG *
FreshArg()
{
    ARG *arg;

    arg = (ARG *) malloc(sizeof (ARG));
    memset((void *) arg, (char) 0, sizeof (ARG));
    return (arg);
}

/*
 * status--give the database status
 */
/*ARGSUSED */
static void
DoStatus(arg)
    ARG *arg;
{
    char banner[MAXPATHLEN], vstr[MAXSTR];
    FILE *bfp;

    (void) sprintf(banner, "%s.bnr", Database);
    if (bfp = fopen(banner, "r")) {
	(void) sprintf(vstr, "Qi server %s", Revision);
	DoReply(LR_PROGRESS, vstr);
	while (fgets(banner, sizeof (banner) - 1, bfp)) {
	    char *nl = strchr(banner, '\n');

	    if (nl)
		*nl = '\0';
	    DoReply(LR_PROGRESS, banner);
	}
	(void) fclose(bfp);
    }
    if (ReadOnly)
	DoReply(LR_RONLY, "Database ready, read only (%s).", DBState);
    else
	DoReply(LR_OK, "Database ready.");
}

/*
 * id--this command is a no-op; the client issues it only to put
 * the name of the calling user into the nameserver's logfiles.
 */
/*ARGSUSED */
static void
DoId(arg)
    ARG *arg;
{
    DoReply(LR_OK, "Thanks.");
}

/*
 * quit
 */
/*ARGSUSED */
void
DoQuit(arg)
    ARG *arg;
{
    fprintf(Output, "%d:%s\n", LR_OK, "Bye!");
    fflush(Output);
    IssueMessage(LOG_INFO, "Done 0");
#ifdef KDB
    (void) krb5_db_fini();
#endif /* KDB */
    closelog();
    exit(0);
}

/*
 * info,  N.B., the ordering of the #define's for the authenticate section
 * is the order that the server prefers the clients to use.  If a different
 * order is preferred, the code sections must be moved around.
 */
/*ARGSUSED */
static void
DoInfo(arg)
    ARG *arg;
{
    short n = 0;
    char sbuf[8], authmethods[MAXSTR];

#ifdef MAILDOMAIN
    DoReply(-LR_OK, "%d:maildomain:%s", ++n, MAILDOMAIN);
#endif /* MAILDOMAIN */
    DoReply(-LR_OK, "%d:mailfield:%s", ++n, MAILFIELD);
    DoReply(-LR_OK, "%d:administrator:%s", ++n, ADMIN);
    DoReply(-LR_OK, "%d:passwords:%s", ++n, PASSW);
    DoReply(-LR_OK, "%d:mailbox:%s", ++n, MAILBOX);
#ifdef CHARSET
    DoReply(-LR_OK, "%d:charset:%s", ++n, CHARSET);
#endif /* CHARSET */
    strcpy(authmethods, "authenticate");
#ifdef KRB5_AUTH
    (void) sprintf(sbuf, ":%d", LQ_KRB5);
    strcat(authmethods, sbuf);
#endif /* KRB5_AUTH */
#ifdef KRB4_AUTH
    (void) sprintf(sbuf, ":%d", LQ_KRB4);
    strcat(authmethods, sbuf);
#endif /* KRB4_AUTH */
#ifdef GSS_AUTH
    (void) sprintf(sbuf, ":%d", LQ_GSS);
    strcat(authmethods, sbuf);
#endif /* GSS_AUTH */
#ifdef PASS_AUTH
    (void) sprintf(sbuf, ":%d", LQ_PASSWORD);
    strcat(authmethods, sbuf);
#endif /* PASS_AUTH */
#ifdef EMAIL_AUTH
    (void) sprintf(sbuf, ":%d", LQ_EMAIL);
    strcat(authmethods, sbuf);
#endif /* EMAIL_AUTH */
#ifdef FWTK_AUTH
    (void) sprintf(sbuf, ":%d", LQ_FWTK);
    strcat(authmethods, sbuf);
#endif /* FWTK_AUTH */
#ifdef CLEAR_AUTH
    (void) sprintf(sbuf, ":%d", LQ_CLEAR);
    strcat(authmethods, sbuf);
#endif /* CLEAR_AUTH */
    DoReply(-LR_OK, "%d:%s", ++n, authmethods);
    DoReply(LR_OK, "Ok.");
}

/*
 * Not implemented
 */
static void
NotImplemented(arg)
    ARG *arg;
{
    DoReply(500, "%s:command not implemented.", arg->aFirst);
}

/*
 * make a reply to a command
 */
/*VARARGS2 */
void
#ifdef __STDC__
DoReply(int code, char *fmt,...)
#else /* !__STDC__ */
DoReply(code, fmt, va_alist)
    int  code;
    char *fmt;
    va_dcl

#endif /* __STDC__ */
{
    char scratchFormat[256];
    char buf[4096];
    va_list args;

    (void) sprintf(scratchFormat, "%d:%s\n", code, fmt);

#ifdef __STDC__
    va_start(args, fmt);
#else /* !__STDC__ */
    va_start(args);
#endif /* __STDC__ */
    vsprintf(buf, scratchFormat, args);
    va_end(args);
    if (TempOutput == NULL && !OpenTempOut()) {
	fprintf(Output, "%d:couldn't open temp file.\n", LR_INTERNAL);
	return;
    }
    if (fprintf(TempOutput, "%s", buf) == EOF) {
	IssueMessage(LOG_ERR, "DoReply: fprintf: %s", strerror(errno));
	fprintf(Output, "%d:couldn't write to temp file.\n", LR_INTERNAL);
	exit(0);
    }
    /* IssueMessage(LOG_DEBUG, "%s", buf); */
}

/*
 * Select which authentication method to use.
 */
static void
DoXLogin(arg)
    ARG *arg;
{
    int  code;
    char *me;
    char *alias = NULL;

    me = arg->aFirst;
    if (!stricmp(me, "klogin"))	/* backward compatibility */
	code = LQ_KRB4;
    else {
	if (!arg->aNext) {
	    DoReply(LR_SYNTAX, "%s: No authentication method selected.", me);
	    return;
	}
	arg = arg->aNext;
	code = atoi(arg->aFirst);
	if (!arg->aNext) {
	    DoReply(LR_SYNTAX, "%s: No login alias specified.", me);
	    return;
	}
	arg = arg->aNext;
	alias = strdup(arg->aFirst);
	if (arg->aNext) {
	    DoReply(LR_SYNTAX, "%s: Too many arguments.", me);
	    return;
	}
    }

    switch (code) {
#ifdef FWTK_AUTH
    case LQ_FWTK:
	DoFwtkLogin(alias);
	break;
#endif /* FWTK_AUTH */

#ifdef KRB5_AUTH
    case LQ_KRB5:
	DoKrbLogin(code, alias);
	break;
#endif /* KRB5_AUTH */

#ifdef GSS_AUTH
    case LQ_GSS:
	DoGssLogin(alias);
	break;
#endif /* GSS_AUTH */

#ifdef KRB4_AUTH
    case LQ_KRB4:
	DoKrbLogin(code, alias);
	break;
#endif /* KRB4_AUTH */

    default:
	DoReply(LR_NOAUTH, "%s:Authentication method %d unavailable.",
		me, code);
	break;
    }
    if (alias)
	free(alias);
}

/*
 * Identify user, using TIS Firewall Toolkit authentication server.
 */
#ifdef FWTK_AUTH
static void
DoFwtkLogin(alias)
    char *alias;
{
    char buf[MAXSTR];

    if(cfp == NULL && (cfp = cfg_read("authenticate")) == (Cfg *)-1) {
	IssueMessage(LOG_ERR, "DoFwtkLogin: cfg_read(): Cannot read config.");
	DoReply(LR_INTERNAL, "DoFwtkLogin: cfg_read(): Cannot read config.");
	return;
    }

    /* open connection to auth server */
    if (auth_open(cfp)) {
	IssueMessage(LOG_ERR, "DoFwtkLogin: auth_open(): Cannot connect.");
	DoReply(LR_TEMP, "DoFwtkLogin: auth_open(): Cannot connect.");
	return;
    }

    /* get welcome message from auth server */
    if(auth_recv(buf, sizeof(buf)))
	goto lostconn;

    if (strncmp(buf, "Authsrv ready", 13)) {
	IssueMessage(LOG_ERR, "DoFwtkLogin: auth_recv(): Unexpected greeting: %s.", buf);
	DoReply(LR_INTERNAL, "DoFwtkLogin: auth_recv(): Unexpected greeting: %s.", buf);
	auth_close();
	return;
    }

    (void) sprintf(buf, "authorize %s", alias);
    if (auth_send(buf))
	goto lostconn;
    if (auth_recv(buf, sizeof(buf)))
	goto lostconn;

    ClaimUser = strdup(alias);
    AmHero = 0;
    AmOpr = 0;
    State = S_E_PENDING;
    OkResponse = LQ_PASSWORD|LQ_FWTK;
    if (User)
	FreeDir(&User);

    if (!strncmp(buf, "challenge ", 10))
	fprintf(Output, "%d:%s\n", LR_XLOGIN, buf+10);
    else
	fprintf(Output, "%d:P!L0.C4XDW])0/0GI^2DZ>\"F]<T69*P.;B*_V;_E[N\n", LR_LOGIN);
    fflush(Output);
    return;

lostconn:
    auth_close();
    IssueMessage(LOG_ERR, "DoFwtkLogin: Lost connection to auth server.");
    DoReply(LR_TEMP, "DoFwtkLogin: Lost connection to auth server.");
    return;
}
#endif /* FWTK_AUTH */

/*
 * Identify user, using GSS-API (v5 Kerberos) authentication.
 */
#ifdef GSS_AUTH
static void
DoGssLogin()
{
    struct sockaddr_in c_sock, s_sock;
    int  namelen = sizeof (c_sock);

    DoReply(LR_NOAUTH, "%s:Authentication method %d unavailable.",
	    me, LQ_GSS);
}
#endif /* GSS_AUTH */

/*
 * Identify user, using v4 or v5 Kerberos authentication.
 */
#if defined(KRB4_AUTH) || defined(KRB5_AUTH)
static void
DoKrbLogin(select, alias)
    int  select;
    char *alias;
{
    struct sockaddr_in s_sock;		/* server's address */
    struct sockaddr_in c_sock;		/* client's address */
    int  namelen;
    int  retval;
    char myhost[MAXHOSTNAMELEN];
    char *pname = NULL, *iname = NULL;

#ifdef KRB4_AUTH
    INT32 authopts;
    AUTH_DAT auth_data;
    KTEXT_ST clt_ticket;
    Key_schedule sched;
    char instance[INST_SZ];
    char realm[REALM_SZ];
    char version[9];
    char srvtab[sizeof(KRB4SRVTAB)+1];
#endif /* KRB4_AUTH */

#ifdef KRB5_AUTH
    krb5_data *comp, packet;
    unsigned char pktbuf[BUFSIZ];
    krb5_principal sprinc, cprinc;
    krb5_ticket *ticket;
    krb5_authenticator *authent;
    krb5_tkt_authent *ad = NULL;
    krb5_address peeraddr;
    char *cp = NULL;
    char *def_realm = NULL;
    int realm_length;
    char v5srvtab[sizeof(KRB5SRVTAB)+1];
    int sock = 0;
#endif /* KRB5_AUTH */

    fprintf(Output, "%d:DoKrbLogin started; send Kerberos mutual authenticator.\n", LR_LOGIN);
    fflush(Output);

    /*
     * To verify authenticity, we need to know the address of the
     * client.
     */
    namelen = sizeof (c_sock);
    if (getpeername(fileno(stdin), (struct sockaddr *) &c_sock, &namelen) < 0) {
	IssueMessage(LOG_ERR, "DoKrbLogin:getpeername(): %s.",
		     strerror(errno));
	DoReply(LR_INTERNAL, "DoKrbLogin:getpeername(): %s",
		strerror(errno));
	return;
    }

    /* for mutual authentication, we need to know our address */
    namelen = sizeof (s_sock);
    if (getsockname(0, (struct sockaddr *) &s_sock, &namelen) < 0 &&
      !QiAuthDebug) {
	IssueMessage(LOG_ERR, "DoKrbLogin:getsockname(): %s.",
		     strerror(errno));
	DoReply(LR_INTERNAL, "DoKrbLogin:getsockname(): %s",
		strerror(errno));
	return;
    }
    (void) gethostname(myhost, MAXHOSTNAMELEN);

#ifdef KRB4_AUTH
    if (select == LQ_KRB4) {
	/*
	 * Read the authenticator and decode it.  Since we don't care
	 * what the instance is, we use "*" so that krb_rd_req
	 * will fill it in from the authenticator.
	 */
	(void) strcpy(instance, "*");

	/* we want mutual authentication */
	authopts = KOPT_DO_MUTUAL;
	(void) strcpy(srvtab, KRB4SRVTAB);
	retval = krb_recvauth(authopts, 0, &clt_ticket, KRB4SRV, instance,
		     &c_sock, &s_sock, &auth_data, srvtab, sched, version);
	if (retval != KSUCCESS) {
	    IssueMessage(LOG_INFO, "DoKrbLogin:krb_recvauth(): %s.",
			 krb_err_txt[retval]);
	    DoReply(LR_ERROR, "DoKrbLogin:krb_recvauth(): %s.",
		    krb_err_txt[retval]);
	    return;
	}
	/* Check the version string (8 chars) */
	if (strncmp(version, "VERSION9", 8)) {
	    /*
	     * didn't match the expected version.
	     * could do something different, but we just
	     * log an error and continue.
	     */
	    version[8] = '\0';	/* make null term */
	    IssueMessage(LOG_ERR, "DoKrbLogin:Version mismatch: '%s' isn't 'VERSION9'",
			 version);
	}

	/* Check to make sure it's in our local realm */
	krb_get_lrealm(realm, 1);
	if (strcmp(auth_data.prealm, realm)) {	/* if not equal */
	    DoReply(LR_ERROR, "DoKrbLogin:Login failed (wrong realm).");
	    IssueMessage(LOG_INFO, "DoKrbLogin:Realm %s not local realm.",
			 auth_data.prealm);
	    return;
	}
	pname = strdup(auth_data.pname);
	iname = strdup(auth_data.pinst);
	if (QiAuthDebug)
	    fprintf(stderr, "KRB4: pname %s, iname %s\n",
		    pname, (iname)?iname:"(nil)");
    }
    else
#endif /* KRB4_AUTH */
#ifdef KRB5_AUTH
    if (select == LQ_KRB5) {
	(void) strcpy(v5srvtab, KRB5SRVTAB);
	if (retval = krb5_kt_default_name(v5srvtab, strlen(v5srvtab))) {
	    DoReply(LR_INTERNAL, "DoKrbLogin:krb5_kt_default_name(%s): %s.",
		v5srvtab, error_message(retval));
	    IssueMessage(LOG_ERR, "DoKrbLogin:krb5_kt_default_name(%s): %s.",
		 v5srvtab, error_message(retval));
	    return;
	}
	if (retval = krb5_sname_to_principal(myhost, KRB5SRV, KRB5_NT_SRV_HST,
					     &sprinc)) {
	    DoReply(LR_INTERNAL, "DoKrbLogin:krb5_sname_to_principal(%s,%s): %s.",
		myhost, KRB5SRV, error_message(retval));
	    IssueMessage(LOG_ERR, "DoKrbLogin:krb5_sname_to_principal(%s,%s): %s.",
		 myhost, KRB5SRV, error_message(retval));
	    return;
	}

	/* Check authentication info */
	peeraddr.addrtype = c_sock.sin_family;
	peeraddr.length = sizeof(c_sock.sin_addr);
	peeraddr.contents = (krb5_octet *)&c_sock.sin_addr;

	if (retval = krb5_recvauth((krb5_pointer)&sock,
			       KQI_VERSION, sprinc, &peeraddr,
			       0, 0, 0,	/* no fetchfrom, keyproc or arg */
			       0,	/* default rc type */
			       0,	/* no flags */
			       0,	/* don't need seq number */
			       &cprinc,
			       0,	/* don't need &ticket */
			       0	/* don't need &authent */
			       )) {
	    DoReply(LR_INTERNAL, "DoKrbLogin:krb5_recvauth(): %s.",
		    error_message(retval));
	    IssueMessage(LOG_ERR, "DoKrbLogin:krb5_recvauth(): %s.",
			 error_message(retval));
	    return;
	}
	krb5_free_principal(sprinc);
	if (retval = krb5_unparse_name(cprinc, &cp)) {
	    DoReply(LR_INTERNAL, "DoKrbLogin:krb5_unparse_name(): %s.",
		    error_message(retval));
	    IssueMessage(LOG_ERR, "DoKrbLogin:krb5_unparse_name(): %s.",
			 error_message(retval));
	    return;
	}
	comp = krb5_princ_component(cprinc, 0);
	pname = malloc(comp->length+1);
	(void) strncpy(pname, comp->data, comp->length);
	comp = krb5_princ_component(cprinc, 1);
	iname = malloc(comp->length+1);
	(void) strncpy(iname, comp->data, comp->length);

	realm_length = krb5_princ_realm(cprinc)->length;
	if (retval = krb5_get_default_realm(&def_realm)) {
	    DoReply(LR_INTERNAL, "DoKrbLogin:krb5_get_default_realm(): %s.",
		    error_message(retval));
	    IssueMessage(LOG_ERR, "DoKrbLogin:krb5_get_default_realm(): %s.",
			 error_message(retval));
	    return;
	}

	if ((realm_length != strlen(def_realm)) ||
	    (memcmp(def_realm, krb5_princ_realm(cprinc)->data, realm_length))) {
	    DoReply(LR_ERROR, "DoKrbLogin:Login failed (wrong realm).");
	    IssueMessage(LOG_INFO, "DoKrb5Login:Realm %*s not local realm.",
			 realm_length, krb5_princ_realm(cprinc)->data);
	    free(def_realm);
	    return;
	}	
	free(def_realm);
	if (QiAuthDebug)
	    fprintf(stderr, "KRB5:pname %s, iname %s\n", pname, (iname)?iname:"(nil)");
    }
    else
#endif /* KRB5_AUTH */
    {
	DoReply(LR_INTERNAL, "DoKrbLogin:Unknown Kerberos method %d.", select);
	IssueMessage(LOG_ERR, "DoKrbLogin:Unknown Kerberos method %d.", select);
	return;
    }
    /*
     * The user has successfully authenticated, so check for matching alias,
     * log in and send reply.
     *
     * Kerberos principal == our alias is pname
     * Kerberos instance iname says whether it's a ph hero.
     *
     * Make sure the instance is a reasonable one (null or KRBHERO).
     * Dissallow other instances, since who knows what level of
     * privilege a particular instance means at a given site?
     */
    if (*iname && strcmp(iname, KRBHERO)) {
	/* not null or phhero */
	DoReply(LR_ERROR, "DoKrbLogin:Login failed (%s/%s: invalid instance).",
	    pname, iname);
	IssueMessage(LOG_INFO, "DoKrbLogin:%s/%s not accepted",
	    pname, iname);
	return;
    }
    if (User)
	FreeDir(&User);
#define WAIT_SECS	1
    if (!GonnaRead("DoKrbLogin"))
	/* Lock routines give their own errors */ ;
    else {
	INT32 xtime = time((INT32 *) NULL);

	AmHero = 0;
	UserAlias = NULL;
	User = GetAliasDir(pname);
	Unlock("DoKrbLogin");
	if (!User) {
	    SleepTil(xtime + WAIT_SECS);
	    DoReply(LR_ERROR, "DoKrbLogin:Login failed (non-existent alias %s).", pname);
	    IssueMessage(LOG_INFO, "DoKrbLogin:Alias %s does not exist", pname);
	}
	else {
	    SleepTil(xtime + WAIT_SECS);
	    DoReply(LR_OK, "%s:Hi how are you?", pname);
	    LocalUser = 1;
	    AmHero = !strcmp(iname, KRBHERO) &&
	      *FINDVALUE(User, F_HERO);
	    UserAlias = FINDVALUE(User, F_ALIAS);
	    UserEnt = CurrentIndex();
	    IssueMessage(LOG_INFO, "%s logged in", UserAlias);
	}
    }
}
#endif /* KRB4_AUTH || KRB5_AUTH */

/*
 * Identify user the old-fashioned way.
 */
static void
DoLogin(arg)
    ARG *arg;
{
    char *me;

    me = arg->aFirst;
    arg = arg->aNext;		/* skip the command name */
    if (!arg)
	DoReply(LR_SYNTAX, "%s:no name given.", me);
    else if (arg->aType != VALUE)
	DoReply(LR_SYNTAX, "%s:argument invalid.", me);
    else if (arg->aNext)
	DoReply(LR_SYNTAX, "%s:extra arguments.", me);
    else {
	Challenge = strdup(RandomString(42));
	DoReply(LR_LOGIN, "%s", Challenge);
	ClaimUser = strdup(arg->aFirst);
	AmHero = 0;
	AmOpr = 0;
	State = S_E_PENDING;
	OkResponse = 0;
#ifdef PASS_AUTH
	OkResponse |= LQ_PASSWORD;
#endif /* PASS_AUTH */
#ifdef EMAIL_AUTH
	OkResponse |= LQ_EMAIL;
#endif /* EMAIL_AUTH */
#ifdef CLEAR_AUTH
	OkResponse |= LQ_CLEAR;
#endif /* CLEAR_AUTH */
	if (User)
	    FreeDir(&User);
    }
}

/*
 * handle the answer to a challenge
 */
#define WAIT_SECS 1
static void
DoAnswer(arg)
    ARG *arg;
{
    char buf[MAXSTR], *me, *pass;
    int method;
    INT32 xtime;

    me = arg->aFirst;
    arg = arg->aNext;

    if (!stricmp(me, "answer"))
	method = LQ_PASSWORD;
    else if (!stricmp(me, "email"))
	method = LQ_EMAIL;
    else if (!stricmp(me, "clear"))
	method = LQ_CLEAR;
    else {
	DoReply(LR_SYNTAX, "%s:Unknown verb.", me);
	goto badfw;
    }
    if (!ClaimUser)
	DoReply(LR_SYNTAX, "%s:there is no outstanding login.", me);
    else if (!arg)
	DoReply(LR_SYNTAX, "%s:no argument given.", me);
    else if (arg->aType != VALUE)
	DoReply(LR_SYNTAX, "%s:invalid argument type.", me);
    else if (! (OkResponse & method))
	DoReply(LR_SYNTAX, "%s:Not allowed for challenge type(s) 0x%x.",
	    me, OkResponse);
    else {
	if (!GonnaRead("DoAnswer"))
	    /* Lock routines give their own errors */
	    goto badfw;

	AmHero = 0;
	AmOpr = 0;
	UserAlias = NULL;
	xtime = time((INT32 *) NULL);
	if (User = GetAliasDir(ClaimUser))
	    pass = PasswordOf(User);
	Unlock("DoAnswer");
	if (!User) {
	    SleepTil(xtime + WAIT_SECS);
	    DoReply(LR_ERROR, "Login failed.");
	    IssueMessage(LOG_INFO, "login: alias %s does not exist", ClaimUser);
	    goto badfw;
	}
#ifdef FWTK_AUTH
	if ((method & OkResponse) && (OkResponse & LQ_FWTK)) {
	    if (strlen(arg->aFirst) > 64) {
		auth_close();
		DoReply(LR_SYNTAX, "Password response too long.");
		goto badfw;
	    }

	    /* send response */
	    sprintf(buf, "response '%s'", arg->aFirst);
	    if (auth_send(buf)) {
		auth_close();
		IssueMessage(LOG_ERR, "DoAnswer: Lost connection to auth server.");
		DoReply(LR_TEMP, "DoAnswer: Lost connection to auth server.");
		goto badfw;
	    }
	    if (auth_recv(buf, sizeof(buf))) {
		auth_close();
		IssueMessage(LOG_ERR, "DoAnswer: Lost connection to auth server.");
		DoReply(LR_TEMP, "DoAnswer: Lost connection to auth server.");
	        goto badfw;
	    }
	    auth_close();

	    /* failed */
	    if (strncmp(buf, "ok", 2)) {
		DoReply(LR_ERROR, "Login failed.");
	        goto badfw;
	    }
	}
	else
#endif /* FWTK_AUTH */
#ifdef PASS_AUTH
	if (OkResponse & method & LQ_PASSWORD) {
	    if (!UserMatch(arg->aFirst)) {
		SleepTil(xtime + WAIT_SECS);
		DoReply(LR_ERROR, "Login failed.");
		IssueMessage(LOG_INFO, "Password incorrect for %s", ClaimUser);
		goto badfw;
	    }
	}
	else
#endif /* PASS_AUTH */
#ifdef EMAIL_AUTH
	if (OkResponse & method & LQ_EMAIL) {
	    if (!OkByEmail(User, arg->aFirst)) {
		IssueMessage(LOG_INFO, "Email incorrect for %s", ClaimUser);
		SleepTil(xtime + WAIT_SECS);
		goto badfw;
	    }
	}
	else
#endif /* EMAIL_AUTH */
#ifdef CLEAR_AUTH
	if (OkResponse & method & LQ_CLEAR) {
# ifdef PRE_ENCRYPT
	    if (pass == NULL ||
		strncmp(crypt(arg->aFirst, arg->aFirst), pass, 13))
# else /* !PRE_ENCRYPT */
	    if (pass == NULL || strcmp(arg->aFirst, pass))
# endif /* PRE_ENCRYPT */
	    {
		SleepTil(xtime + WAIT_SECS);
		DoReply(LR_ERROR, "Login failed.");
		IssueMessage(LOG_INFO, "Clear password incorrect for %s", ClaimUser);
		goto badfw;
	    }
	}
	else
#endif /* CLEAR_AUTH */
	{
	    SleepTil(xtime + WAIT_SECS);
	    DoReply(LR_INTERNAL, "No login action! (method 0x%x, OkResponse 0x%x)", method, OkResponse);
	    IssueMessage(LOG_ERR, "Fell through all login checks!");
	    goto badfw;
	}
	{
	    char *tpnt = FINDVALUE(User, F_HERO);

	    SleepTil(xtime + WAIT_SECS);
	    DoReply(LR_OK, "%s:Hi how are you?", ClaimUser);
	    if (*tpnt != '\0') {
		if (stricmp(tpnt, "opr") == 0 ||
		    stricmp(tpnt, "oper") == 0 ||
		    stricmp(tpnt, "operator") == 0)
		    AmOpr = 1;
		else
		    AmHero = 1;
	    }
	    LocalUser = 1;
	    UserAlias = FINDVALUE(User, F_ALIAS);
	    UserEnt = CurrentIndex();
	    IssueMessage(LOG_INFO, "%s logged in", UserAlias);
	}
    }
badfw:
    State = S_IDLE;
    if (!UserAlias && User) {
	free(User);
	User = NULL;
    }
    if (ClaimUser) {
	free(ClaimUser);
	ClaimUser = NULL;
    }
    if (Challenge) {
	free(Challenge);
	Challenge = NULL;
    }
}

/*
 * sleep til a given time
 */
static void
SleepTil(xtime)
    INT32 xtime;
{
    unsigned span;

    span = xtime - time((INT32 *) 0);
    if (0 < span && span < 10000)
	sleep(span);
}

/*
 * return the dir entry of the requested alias
 */
QDIR
GetAliasDir(fname)
    char *fname;
{
    ARG *Alist;
    ARG *arg;
    INT32 *entry;
    QDIR dirp;

    arg = Alist = FreshArg();

    arg->aType = COMMAND;
    arg->aFirst = strdup("query");
    arg->aNext = FreshArg();
    arg = arg->aNext;
    arg->aType = VALUE | EQUAL | VALUE2;
    arg->aFirst = strdup("alias");	/* should be alias */
    arg->aSecond = strdup(fname);
    (void) ValidQuery(Alist, C_QUERY);

    if ((entry = DoLookup(Alist)) != NULL && length(entry) == 1 &&
	next_ent(*entry))
	getdata(&dirp);
    else
	dirp = NULL;

    if (entry)
	free((char *) entry);
    FreeArgs(Alist);

    return (dirp);
}

/*
 * de-identify the current user
 */
static void
DoLogout(arg)
    ARG *arg;
{
    if (arg->aNext)
	DoReply(LR_SYNTAX, "argument given on logout command.");
    else if (!User)
	DoReply(LR_ERROR, "Not logged in.");
    else {
	FreeDir(&User);
	AmHero = 0;
	AmOpr = 0;
	UserAlias = NULL;
	UserEnt = 0;
	DoReply(LR_OK, "Ok.");
    }
}

/*
 * list fields
 */
static void
DoFields(arg)
    ARG *arg;
{
    if (arg->aNext == NULL) {
	ListAllFields();
	DoReply(LR_OK, "Ok.");
    }
    else if (OkFields(arg->aNext)) {
	for (arg = arg->aNext; arg; arg = arg->aNext)
	    if (arg->aFD)
		ListField(arg->aFD);
	DoReply(LR_OK, "Ok.");
    }
    else
	DoReply(LR_SYNTAX, "Invalid field request.");
}

/*
 * List a single field
 */
static void
ListField(fd)
    FDESC *fd;
{
    char scratch[MAX_LEN];
    char *cp;

    (void) sprintf(scratch, "%d:%s:max %d", fd->fdId, fd->fdName, fd->fdMax);
    cp = scratch + strlen(scratch);
    if (fd->fdIndexed)
	cp += strcpc(cp, " Indexed");
    if (fd->fdLookup)
	cp += strcpc(cp, " Lookup");
    if (fd->fdNoMeta)
	cp += strcpc(cp, " NoMeta");
    if (fd->fdPublic)
	cp += strcpc(cp, " Public");
    if (fd->fdLocalPub)
	cp += strcpc(cp, " LocalPub");
    if (fd->fdDefault)
	cp += strcpc(cp, " Default");
    if (fd->fdAlways)
	cp += strcpc(cp, " Always");
    if (fd->fdAny)
	cp += strcpc(cp, " Any");
    if (fd->fdChange)
	cp += strcpc(cp, " Change");
    if (fd->fdSacred)
	cp += strcpc(cp, " Sacred");
    if (fd->fdTurn)
	cp += strcpc(cp, " Turn");
    if (fd->fdEncrypt)
	cp += strcpc(cp, " Encrypt");
    if (fd->fdNoPeople)
	cp += strcpc(cp, " NoPeople");
    *cp = '\0';
    DoReply(-LR_OK, scratch);
    strcpy(scratch, fd->fdHelp);
    for (cp = strtok(scratch, "\n"); cp; cp = strtok((char *) NULL, "\n"))
	DoReply(-LR_OK, "%d:%s:%s", fd->fdId, fd->fdName, cp);
}

/*
 * list all fields
 */
static void
ListAllFields()
{
    FDESC **fd;

    for (fd = FieldDescriptors; *fd; fd++) {
	if ((*fd)->fdLocalPub && !LocalUser)
	    continue;
	ListField(*fd);
    }
}

/*
 * validate arguments for field names
 */
static int
OkFields(arg)
    ARG *arg;
{
    int  bad = 0;
    int  count = 0;
    FDESC *fd;

    for (; arg; arg = arg->aNext) {
	count++;
	if (arg->aType != VALUE) {
	    DoReply(-LR_SYNTAX, "argument %d:is not a field name.", count);
	    bad = 1;
	}
	else if (!(fd = FindFD(arg->aFirst)) || (!LocalUser && fd->fdLocalPub)) {
	    DoReply(-LR_FIELD, "%s:unknown field.", arg->aFirst);
	    bad = 1;
	}
	else
	    arg->aFD = fd;
    }
    return (!bad);
}

/*
 * delete entries
 */
static void
DoDelete(arg)
    ARG *arg;
{
    INT32 *entries, *entp;
    int  haveError = 0;
    int  count;
    int  done;
    QDIR dirp;

    if (!AmHero && !User) {
	DoReply(LR_NOTLOG, "Must be logged in to delete.");
	return;
    }
    else if (!UserCanDelete()) {
	DoReply(LR_ERROR, "You may not delete entries.");
	IssueMessage(LOG_INFO, "%s is not authorized to delete entries", UserAlias);
	return;
    }
    if (!ValidQuery(arg, C_DELETE)) {
	DoReply(LR_SYNTAX, "Delete command not understood.");
	return;
    }
    if (!GonnaWrite("DoDelete")) {
	/* GonnaWrite will issue an error message */ ;
	return;
    }
    if ((entries = DoLookup(arg)) == NULL) {
	Unlock("DoDelete");
	DoReply(LR_NOMATCH, "No entries matched specifications.");
	return;
    }
    for (count = 1, done = 0, entp = entries; *entp; count++, entp++) {
	if (!next_ent(*entp)) {
	    DoReply(-LR_TEMP, "Internal error.");
	    haveError = 1;
	    continue;
	}
	getdata(&dirp);
	if (!CanDelete(dirp)) {
	    DoReply(-LR_ERROR, "%d:%s: you may not delete this entry.",
		    count, FINDVALUE(dirp, F_ALIAS));
	    haveError = 1;
	    IssueMessage(LOG_INFO, "%s may not delete %s",
			 UserAlias, FINDVALUE(dirp, F_ALIAS));
	    FreeDir(&dirp);	/* XXX */
	    continue;
	}
#ifdef KDB
	if (*FINDVALUE(dirp, F_ALIAS))
	    kdb_del_entry(FINDVALUE(dirp, F_ALIAS));
#endif /* KDB */
	/* delete the index entries */
	MakeLookup(dirp, *entp, unmake_lookup);
	FreeDir(&dirp);

	/* mark it as dead and put it out to pasture */
	SetDeleteMark();
	set_date(1);
	store_ent();
	done++;
    }

    free((char *) entries);
    Unlock("DoDelete");

    if (haveError)
	DoReply(LR_ERROR, "%d errors, %d successes on delete command.",
		count - done, done);
    else
	DoReply(LR_OK, "%d entries deleted.", done);
}

/*
 * open a temp file for output
 */
static int
OpenTempOut()
{
    if (TFile == NULL) {
	TFile = strdup(TEMPFILE);
	mktemp(TFile);
    }
    if ((TempOutput = fopen(TFile, "w+")) == NULL) {
	IssueMessage(LOG_ERR, "OpenTempOut: fopen(%s) failed: %s",
		     TFile, strerror(errno));
	free(TFile);
	TFile = NULL;
	return (0);
    }
    unlink(TFile);

    return (1);
}

/*
 * Dump a the stuff in TFile to output
 */
static void
DumpOutput()
{
    int  c;

    rewind(TempOutput);		/* back to the beginning */
    {
	while ((c = getc(TempOutput)) != EOF)
	    putc(c, Output);
    }
    fclose(TempOutput);		/* close; already unlinked */
    TempOutput = NULL;
    fflush(Output);
}

/*
 * print the current command
 */
static void
PrintCommand(cmd, arg)
    int  cmd;
    ARG *arg;
{
    fprintf(Output, "%d: %s", LR_ECHO, arg->aFirst);
    for (arg = arg->aNext;
	 arg;
	 arg = arg->aNext) {
	putc(' ', Output);
	if (arg->aType == RETURN)
	    fputs(cmd == C_QUERY ? "return" : "make", Output);
	else {
	    if (arg->aType & VALUE)
		fputs(arg->aFirst, Output);
#ifdef DO_TILDE
	    if (arg->aType & TILD_E)
		putc('~', Output);
	    else
#endif
	    if (arg->aType & EQUAL)
		putc('=', Output);
	    if (arg->aType & VALUE2)
		fputs(arg->aSecond, Output);
	}
    }
    putc('\n', Output);
}

/*
 * was the returned string encrypted with the appropriate password?
 */
static int
UserMatch(string)
    char *string;
{
    char decrypted[MAXSTR];
    char *pw = PasswordOf(User);

    if (!*pw)
	return (0);
#ifdef PRE_ENCRYPT
    crypt_start(crypt(pw,pw));
#else
    crypt_start(pw);
#endif
    (void) decrypt(decrypted, string);
    return (!strcmp(decrypted, Challenge));
}

/*
 * generate a random string
 */
static char *
RandomString(byteCount)
    int  byteCount;
{
    static char string[MAXSTR];
    char *cp;
    static int seeded = 0;

    if (!seeded) {
	seeded = 1;
	srand((int) time((INT32 *) NULL) ^ getpid());
    }
    for (cp = string; byteCount; cp++, byteCount--)
	*cp = (rand() & 0x3f) + 0x21;

    return (string);
}

/*
 * extract the password from a dir
 */
char *
PasswordOf(User)
    QDIR User;
{
    int  len;
    char *password;

    /* find the user's password */
    if (!*(password = FINDVALUE(User, F_PASSWORD))) {
#ifdef ID_FALLBACK
	if (*(password = FINDVALUE(User, F_UNIVID))) {
	    len = strlen(password);
	    if (len > 8)
		password += len - 8;
	}
	else
#endif
	    password = "";
    }
    return (password);
}

#ifdef EMAIL_AUTH
/*
 * figure out if a user is ok by his email address
 */
static int
OkByEmail(User, username)
    QDIR User;
    char *username;
{
    char buf[256];
    char *email = FINDVALUE(User, F_EMAIL);
    char *new, *spnt, *epnt;
    int  result;

    /*
     * Fix up email field by omitting leading whitespace and
     * terminating it at the first white space.
     */
    new = spnt = strdup(email);
    while (isspace(*spnt))
	spnt++;
    epnt = spnt;
    while (*epnt && !isspace(*epnt))
	epnt++;
    *epnt = '\0';
    if (!TrustHp.h_name || !*spnt)
	result = 1;
    else {
	(void) sprintf(buf, "%s@%s", username, TrustHp.h_name);
	result = stricmp(spnt, buf);
    }
    free(new);
    if (result)
	DoReply(LR_NOEMAIL, "You can't login that way.");
    return (!result);
}

#endif

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strstr.c	5.1 (Berkeley) 5/15/90";

#endif /* LIBC_SCCS and not lint */

#ifdef NO_STRSTR

/*
 * Find the first occurrence of find in s.
 */
char *
strstr(s, find)
    register char *s, *find;
{
    register char c, sc;
    register int len;

    if ((c = *find++) != 0) {
	len = strlen(find);
	do {
	    do {
		if ((sc = *s++) == 0)
		    return ((char *) 0);
	    } while (sc != c);
	} while (strncmp(s, find, len) != 0);
	s--;
    }
    return ((char *) s);
}

#endif
