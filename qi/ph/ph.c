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
static char rcsid[] = "@(#)$Id: ph.c,v 7.6 1995/06/27 17:36:18 p-pomes Exp $";
static char rcsrev[] = "100:Ph client $Revision: 7.6 $";
#endif

#ifdef OSF1
# define INT32 int
#else /* !OSF1 */
# define INT32 long
#endif /* OSF1 */

/*
 * This is a client program for CSO's nameserver.  It attempts to contact
 * the nameserver running on ns.uiuc.edu, and query it about entries.
 * It's *HIGHLY* recommended that the DNS be used to resolve ns.uiuc.edu
 * to a specific host.  The following entry in /etc/services will help:
 *
 * /etc/services:
 * csnet-ns		105/tcp     ns		# CCSO nameserver
 */
#ifdef VMS
/*
            P H   for   V A X / V M S
  
  
  Ported to VAX/VMS Version 4.4 using VAXC 2.2 and Wollongong WIN/TCP 3.1
  by Mark Sandrock, UIUC School of Chemical Sciences Computing Services.
  
  VMS 4.4 implementation notes:
  
  1) VAXCRTL does not supply the following routines used by PH:
  
    a) fork: SYS$CREPRC or LIB$SPAWN should be used instead.
    b) execlp: VAXCRTL does provide execl, but it is too limited.
    c) popen/pclose: VAXCRTL does provide "pipe" instead.
    d) index/rindex: VAXCRTL "strchr/strrchr" functions are equivalent.
    e) getpass: implemented in this file.
    f) unlink: VAXCRTL does provide "delete" function.
  
  2) VAX/VMS does not provide the following utilities used by PH:
  
    a) /usr/ucb/more: TYPE/PAGE should be used instead.
    b) /usr/ucb/vi: (callable) EDT should be used instead.
  
  3) The VAXCRTL "getenv" function does not recognize the following
     environment names. SYS$TRNLNM could be used instead, if need be:
  
    a) PAGER: specifies "pager" other than the default (TYPE/PAGE).
    b) EDITOR: specifies editor other than the default (EDT).
  
  4) The SOCKET INTERFACE implemented by Wollongong WIN/TCP 3.1 returns
     a channel number rather than a standard file descriptor, and thus
     is not compatible with the UNIX-style i/o functions such as fdopen,
     read and write. Instead WIN/TCP provides special versions of read/
     write called netread/netwrite for network i/o.
  
  5) The VMS VAXC include files are used wherever possible, with several
     exceptions as noted in the WIN/TCP Programmer's Guide. The following
     include files do not exist under VMS VAXC 2.2 nor under WIN/TCP 3.1,
     and were simply copied over from uxc (4.3 bsd):
  
    a) #include <sgtty.h>
    b) #include <syslog.h>
  
  Change log:
  
  05-May-1988   12:09   MTS   Initial port of ph.c,v 2.15 to vms_ph.c.
                Initial port of cryptit.c to vms_cryptit.c.
  
***************************************************************************/

/*#module vms_ph "2.15"*/
static char *rcsid = "VAX/VMS: ph.c,v 2.15 88/05/05 15:06:50 dorner Locked $";

#include stdio
#include signal
#include <types.h>
#include <socket.h>
#include file
#include <in.h>
#include <netdb.h>
#include ctype
#include <strings.h>
#include descrip		/* VMS descriptor structures defs */
#include iodef			/* VMS I/O function definitions */
#include ssdef			/* VMS System Status definitions */
#include ttdef			/* VMS terminal characteristics */
#include "termdefs.h"		/* VMS defs for setterm() function */

#else /* !VMS */

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
#include <fcntl.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/param.h>
#include <errno.h>
#endif /* VMS */

#ifdef KRB4_AUTH
#include <sys/param.h>
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif /* KRB4_AUTH */

#ifdef OS2
#include <process.h>
#endif   /* OS2 */

extern int QiAuthDebug;

#include <qiapi.h>

/*
 * vital defines
 */
#define MAXSTR		2048	/* max string length */
#define MAXVAL		14000	/* max value length */
#define DELIM		" \t\n"	/* command delimiters */
#define MAXARGS		20	/* maximum # of arguments in PH environ var. */
#define CLIENT		"ph"
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  64
#endif
#ifndef PAGER
# define PAGER		"more"
#endif

#ifdef VMS
char *strchr();			/* VMS equivalent of "index" */
char *strrchr();		/* VMS equivalent of "rindex" */
#endif /* VMS */

#define GetQValue(x)	(strchr(strchr(x,':')+1,':')+1)

/*
 * declarations for the functions in this file
 */
char *GetValue __P((char *, char *));
char *issub __P((char *, char *));
int  ContactQI();
int  DoId();
int  PrintResponse __P((int));

#ifdef VMS
int  GetGood __P((char *, int, int));
#else /* !VMS */
int  GetGood __P((char *, int, FILE *));
#endif /* VMS */

void EnvOptions __P((char *));
void ComplainAboutService();
void ComplainAboutHost __P((char *));
void Interactive();
int  DoCommand __P((char *));
int  DoOtherWPage __P((char *));
int  DoOther __P((char *));

#ifdef MACC
int  DoFields __P((char *));
#endif

int  DoHelp __P((char *));
int  DoQuery __P((char *));
int  DoLogin __P((char *));
int  DoQuit __P((char *));
int  DoEdit __P((char *));
int  DoMake __P((char *));
int  DoRegister __P((char *));
int  EditValue __P((char *));
int  UpdateValue __P((char *, char *, char *));
int  DoFields __P((char *));
int  DoMe __P((char *));
int  DoLogout __P((char *));
int  DoPassword __P((char *));
void VetPassword __P((char *));
int  AllDigits __P((char *));
int  PrintQResponse __P((int, int));
void DoAutoLogin();
void SkipMacdef __P((FILE *));
void EmailLine __P((char *, char *));
void NotRegisteredLine __P((char *, FILE *));
FILE *OpenPager __P((int));
int  DoSwitch __P((char *));

#ifdef __STDC__
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
#else /* !__STDC__ */
# include <strings.h>
char *malloc();
char *getenv();
char *strtok();
#endif /* __STDC__ */

char *getpass __P((const char *));
char *mktemp __P((char *));

#ifndef MAILDOMAIN
# define MAILDOMAIN	NULL
#endif /* !MAILDOMAIN */

/*
 * These are external for convenience' sake
 */
#ifdef MACC_ECHO
int  maccecho = 0;
#endif

#ifdef VMS
int  ToQI;			/* write to this to tell the nameserver stuff */
int  FromQI;			/* read nameserver responses from here */
char ToQIBuf[MAXSTR];
int  ToQILen;
#define qprintf		\	/*this is fairly sneaky... */
{
	\
	   char    *ToQI = ToQIBuf;

	\
	   sprintf
#define qflush(foobar)		\	/*compound sneakiness */
	   ToQILen = strlen(ToQIBuf);
	\
} \

netwrite(ToQI, ToQIBuf, ToQILen)
#else
FILE *ToQI;			/* write to this to tell the nameserver stuff */
FILE *FromQI;			/* read nameserver responses from here */
#define qprintf fprintf
#define qflush fflush
#endif

char MyAlias[MAXSTR];		/* currently logged-in alias */
char *Me;			/* the name of this program */
char *MyPassword = NULL;	/* password read from .netrc (if any) */
char *MailDomain = MAILDOMAIN;	/* mail domain */
int  LocalPort = 0;		/* local port in use */
int  Debug = 0;			/* print debug info if set */
unsigned INT32 Dot2Addr();

/*
 * switches
 */
int  NoNetrc = 0;		/*-n don't read .netrc */
char *UseHost = 0;		/*-s use server on what machine */
int  UsePort = 0;		/*-p use port # */
int  NoReformat = 0;		/*-i don't reformat email fields */
int  NoPager = 0;		/*-m don't use pager */
int  NoBeautify = 0;		/*-b don't beautify output */
int  NoLabels = 0;		/*-l don't use labels */
int  Confirm = 0;		/*-c confirm Edit */
int  Quiet = 0;			/*-q act like a pipe, no extraneous output */
char *DefType = 0;		/*-t prepend this type to queries */
char *ReturnFields = NULL;	/*-f give list of fields */
int  JustHelp = 0;		/*-h give me help */

/*
 * and the fun begins...
 */
int
main(argc, argv)
    int  argc;
    char **argv;
{
    int  code = LR_ERROR;
    int  optionsCount;
    char buffer[4096];

#ifdef MACC_ECHO
    int  margc = 0;
#endif
#ifdef VMS
    char *temps;		/* temp strings */
#endif
    /* figure out what this program is called */
#ifdef VMS
    Me = " ph";
#else
    Me = (char *) strrchr(*argv, '/');
#endif
    if (Me)
	Me++;
    else
	Me = *argv;
    EnvOptions(CLIENT);
#ifndef OS2
    if (strcmp(CLIENT, Me)) {
	sprintf(buffer, "-t %s", Me);
	(void) OptionLine(buffer);
	EnvOptions(Me);
    }
#endif /* OS2 */
    optionsCount = ProcessOptions(--argc, ++argv);
    argc -= optionsCount;
    argv += optionsCount;

    if (!ContactQI()) {
	fputs("Sorry--phone book not available now.\n", stderr);
#ifdef VMS
	exit(SS$_CONNECFAIL);
#else
	exit(1);
#endif
    }
    /* identify ourselves */
    if ((code = DoId()) >= 400)
	exit(code / 100);
    if (!MailDomain && (code = GetMailDomain()) >= 400)
	exit(code / 100);
    if (argc == 0 && !JustHelp)
	Interactive();		/* no arguments--interactive mode */
    else {
	/* make a query out of the arguments */
#ifdef VMS
	temps = JustHelp ? "help ph " : "query ";
	netwrite(ToQI, temps, strlen(temps));
	for (; argc; argc--, argv++) {
	    netwrite(ToQI, *argv, strlen(*argv));
	    if (argc > 1)
		netwrite(ToQI, " ", 1);
	}
	temps = "\nquit\n";
	netwrite(ToQI, temps, strlen(temps));
#else
	strcpy(buffer, JustHelp ? "help " : "query ");
	for (; argc; argc--, argv++) {
	    strcat(buffer, *argv);
	    if (argc > 1)
		strcat(buffer, " ");
	}
	strcat(buffer, "\n");
	code = DoCommand(buffer);
	if (Debug)
	    fprintf(stderr, "sent=quit\n");
	qprintf(ToQI, "quit\n");
	qflush(ToQI);
#endif
    }
#ifdef VMS
    exit(SS$_NORMAL);
#else
    exit(code > 299 ? code / 100 : 0);
#endif
}

/*
 * contact the central nameserver
 */
int
ContactQI()
{
    int  sock;			/* our socket */
    static struct sockaddr_in QI;	/* the address of the nameserver */
    struct servent *sp;		/* nameserver service entry */
    static struct hostent *hp;	/* host entry for nameserver */
    char host[80];
    char *baseHost;
    int  backupNum = 0;
    int  mightBackup;
    int  result = 0;
    int  err;

    QI.sin_family = AF_INET;

    /* give up privs if using anything other than default port and host */
    if (UsePort || (UseHost && *UseHost)) {
	if (Debug)
	    fprintf(stderr, "giving up privs (UsePort || UseHost)\n");
	(void) setgid(getgid());
	(void) setuid(getuid());
    }

    /* find the proper port */
    if (UsePort)
	QI.sin_port = htons(UsePort);
    else if (sp = getservbyname(NSSERVICE, "tcp")) {
	QI.sin_port = sp -> s_port;
    }
    else {
	ComplainAboutService();
	QI.sin_port = htons(atoi(FALLBACKPORT));
    }

    /* find the proper host */
    baseHost = UseHost ? UseHost : HOST;
    if (mightBackup = (*baseHost == '.'))
	sprintf(host, "%s%s", NSSERVICE, baseHost);
    else
	strcpy(host, baseHost);

    if (!geteuid())
	LocalPort = (IPPORT_RESERVED - 1);
    for (;;) {
	/* create the socket */
#ifdef OS2
	sock = socket(PF_INET, SOCK_STREAM, 0);
#else
	sock = LocalPort ? rresvport(&LocalPort) : socket(PF_INET, SOCK_STREAM, 0);
#endif /* OS2 */
	if (sock < 0) {
	    perror("socket");
	    goto done;
	}
	QI.sin_family = AF_INET;
	if (hp = gethostbyname(host)) {
#ifdef _CRAY
	    memmove((char *) &QI.sin_addr, hp -> h_addr, 4);
#else
	    memmove((char *) &QI.sin_addr.s_addr, hp -> h_addr, 4);
#endif
	}
	else if (!backupNum) {
	    ComplainAboutHost(host);
	    QI.sin_addr.s_addr = Dot2Addr(FALLBACKADDR);
	}
	else {
	    fprintf(stderr, "No more backups to try.\n");
	    goto done;
	}

	/* connect to the nameserver */
	if (connect(sock, (struct sockaddr *) & QI, sizeof(QI)) < 0) {
	    if (errno == EADDRINUSE) {
		if (LocalPort)
		    LocalPort = LocalPort - 1;
		continue;
	    }
	    perror(host);
	    if (mightBackup) {
		backupNum++;
		sprintf(host, "%s%d%s", NSSERVICE, backupNum, baseHost);
	    }
	    else
		goto done;
	}
	else
	    break;
    }

    if (backupNum)
	fprintf(stderr, "WARNING--backup host %s; information may be out of date.\n", host);
    /* open path to nameserver */
#ifdef VMS
    ToQI = sock;		/* copy socket channel for netwrite calls */
    FromQI = sock;		/* ditto for netread calls */
#else
    if ((ToQI = fdopen(sock, "w")) == NULL) {
	perror("to qi");
	goto done;
    }
    /* open path from nameserver */
    if ((FromQI = fdopen(sock, "r")) == NULL) {
	perror("from qi");
	goto done;
    }
#endif
    if (UseHost)
	free(UseHost);
    UseHost = strdup(hp ? hp -> h_name : inet_ntoa(QI.sin_addr));
    UsePort = ntohs(QI.sin_port);
    result = 1;

done:
    setgid(getgid());
    setuid(getuid());
    return (result);
}

/*
 * identify ourselves to the nameserver
 */
int
DoId()
{
    int  code;
    char *cpnt = (char *) getlogin();
    struct passwd *pw;

    /* Who is doing this? */
    if (cpnt == NULL || (pw = getpwnam(cpnt)) == NULL) {
	if ((pw = getpwuid(getuid())) == NULL) {
	    perror("getpwuid(getuid())");
	    exit(1);
	}
    }
    if (Debug)
	fprintf(stderr, "sent=id %d\n", pw -> pw_uid);
    qprintf(ToQI, "id %d\n", pw -> pw_uid);
    qflush(ToQI);
    code = PrintResponse(-1);
    return (code);
}

/*
 * get the mail domain from a foreign server
 */
int
GetMailDomain()
{
    char scratch[MAXSTR];
    char *lastc, *s2lastc;
    short code = 0;

    if (Debug)
	fprintf(stderr, "sent=siteinfo\n");
    qprintf(ToQI, "siteinfo\n");
    qflush(ToQI);
    while (GetGood(scratch, MAXSTR, FromQI)) {	/* read it */
	code = atoi(scratch);
	if (code == -200) {
	    if ((lastc = (char *) strrchr(scratch, ':')) && lastc > scratch) {
		*lastc++ = 0;
		if (s2lastc = (char *) strrchr(scratch, ':')) {
		    s2lastc++;
		    if (!strcmp("maildomain", s2lastc)) {
			lastc[strlen(lastc) - 1] = 0;
			MailDomain = strdup(lastc);
		    }
		}
	    }
	}
	else if (code >= LR_OK)
	    break;
    }
    return (code ? 200 : 500);	/* only fail if the cnxn broke */
}

/*
 * print what the QI (Query Interpreter; nameserver) says
 * read replies from nameserver until code indicates a completed
 * command.  This routine does not beautify the responses in any way.
 * if pager is 1, the pager will be used.
 * if pager is 0, no pager will be used.
 * if pager is -1, the response will not be printed at all.
 */
int
PrintResponse(pager)
    int  pager;			/* use the pager? */
{
    char scratch[MAXSTR];	/* some space */
    int  code = LR_ERROR;	/* the reply code */
    FILE *out;

    out = OpenPager(pager);
    while (GetGood(scratch, MAXSTR, FromQI)) {	/* read it */
	code = atoi(scratch);
	if (pager != -1 || code >= 400)
	    fputs(scratch, out);/* echo it */
	if (code >= LR_OK)
	    break;
    }
#ifdef VMS
#else
    if (out != stdout)
	pclose(out);
#endif

    return (code);		/* all done.  return final code */
}


/*
 * complain that there isn't an entry for ns in /etc/services
 */
void
ComplainAboutService()
{
    fprintf(stderr, "Warning--there is no entry for ``%s'' in /etc/services;\n",
	    NSSERVICE);
    fputs("please have your systems administrator add one.\n", stderr);
    fprintf(stderr, "I'm going to use port %s in the meantime.\n", FALLBACKPORT);
}

/*
 * complain that there isn't an entry for HOST in /etc/hosts
 */
void
ComplainAboutHost(name)
    char *name;
{
    fprintf(stderr, "Warning--unable to find address for ``%s''.\n",
	    name);
    fprintf(stderr, "I'm going to use address %s in the meantime.\n",
	    FALLBACKADDR);
}

/*
 * the interactive portion of the client
 */
typedef struct command CMD;

struct command {
    char *cName;		/* the name of the command */
    int  cLog;			/* must be logged in to use? */
    int  (*cFunc) ();		/* function to call for command */
};

CMD  CommandTable[] =
{
 "help", 0, DoHelp,
 "?", 0, DoHelp,
 "query", 0, DoQuery,
#ifndef MACC
 CLIENT, 0, DoQuery,
 "me", 1, DoMe,
 "edit", 1, DoEdit,
 "make", 1, DoMake,
 "register", 1, DoRegister,
 "password", 1, DoPassword,
 "passwd", 1, DoPassword,
 "login", 0, DoLogin,
 "logout", 1, DoLogout,
#endif
#ifdef MACC
 "fields", 0, DoFields,
#else
 "fields", 0, DoOtherWPage,
 "switch", 0, DoSwitch,
#endif
#ifndef MACC
 "add", 1, DoOther,
 "delete", 1, DoOther,
 "set", 0, DoOther,
#endif
 "quit", 0, DoQuit,
 "bye", 0, DoQuit,
 "exit", 0, DoQuit,
 0, 0, 0
};

/*
 * the main loop
 */
int  LastCode = 0;		/* the response from the previous command */

void
Interactive()
{
    char in_line[MAXSTR];	/* space for an input line */
    char *spot;

    *MyAlias = 0;		/* nobody logged in yet... */
    if (!Quiet)
	puts(rcsrev);

    /* print database status */
    if (!Quiet)
	LastCode = DoOther("status\n");

    /* autologin if possible */
#ifndef MACC
    if (!NoNetrc)
	DoAutoLogin();
#endif

    if (!Quiet)
	puts("");

    while (1) {
	(void) signal(SIGPIPE, SIG_IGN);
	if (!Quiet) {
#ifdef MACC_ECHO
	    if (!maccecho)
		printf("%s> ", Me);
#else
	    printf("%s> ", Me);
#endif
	    (void) fflush(stdout);	/* prompt */
	}
	spot = in_line;
	do {
	    if (!fgets(spot, MAXSTR - (spot - in_line), stdin))
		return;		/* read line */
	    spot = in_line + strlen(in_line) - 2;
	    if (*spot == '\\')
		*spot++ = ' ';
	    else
		spot = in_line - 1;
	}
	while (spot >= in_line);
#ifdef MACC_ECHO
	if (maccecho && !Quiet)
	    printf("%s> %s", Me, in_line);
#endif

	if (!(LastCode = DoCommand(in_line)))	/* is it a command we know? */
	    LastCode = DoOther(in_line);	/* unrecognized command */
    }
}

/*
 * look at input line, and if we have a specific command for it, do it
 */
int
DoCommand(in_line)
    char *in_line;		/* the input line */
{
    char scratch[MAXSTR];	/* some space */
    char *token;		/* a token from the command line */
    CMD *cmd;			/* the command name */
    CMD *doMe;
    int  len;

    /* make a safe copy of the input line, so we can play with it */
    strcpy(scratch, in_line);

    if (!(token = strtok(scratch, DELIM)))
	return (LR_ERROR);	/* blank line */

    /* search command table linearly */
    doMe = NULL;
    len = strlen(token);
    for (cmd = CommandTable; cmd -> cName; cmd++)
	if (!strncmp(cmd -> cName, token, len)) {
	    if (doMe) {		/* we found 2 commands that match (bad) */
		printf("%s is ambiguous.\n", token);
		return (LR_ERROR);
	    }
	    doMe = cmd;		/* we found a command that matches */
	}
    if (doMe) {			/* found one and only one command */
	/* expand command name */
	token = strtok((char *) 0, "\n");
	sprintf(in_line, "%s %s\n", doMe -> cName, token ? token : "");

	/* execute command */
	if (doMe -> cLog && !*MyAlias)
	    printf("You must be logged in to use %s.\n", doMe -> cName);
	else
	    return ((*doMe -> cFunc) (in_line));
	return (LR_ERROR);
    }
    return (0);			/* didn't find it */
}

/*
 * execute a command for which we do nothing special; use the pager
 */
int
DoOtherWPage(in_line)
    char *in_line;
{
    if (Debug)
	fprintf(stderr, "sent=%s", in_line);	/* send command */
    qprintf(ToQI, "%s", in_line);	/* send command */
    qflush(ToQI);
    return (PrintResponse(1));	/* get response */
}

/*
 * execute a command for which we do nothing special; don't use pager
 */
int
DoOther(in_line)
    char *in_line;
{
    if (Debug)
	fprintf(stderr, "sent=%s", in_line);	/* send command */
    qprintf(ToQI, "%s", in_line);	/* send command */
    qflush(ToQI);
    return (PrintResponse(0));	/* get response */
}

#ifdef MACC
int
DoFields(in_line)
    char *in_line;
{
    printf("Field Name     Description\n");
    printf("------------------------------\n");
    printf("name          Person Name\n");
    printf("email         Electronic Mail Address if exists\n");
    printf("phone         Telephone Number One\n");
    printf("phone2        Telephone Number Two\n");
    printf("address       Street Address of the Building\n");
    printf("building      Building Name\n");
    printf("department    Department Number One of Person\n");
    printf("department2   Department number Two of Person\n");
    printf("appointment   Appointment Classification Code Number One \n");
    printf("appointment2  Appointment Classification Code Number Two\n");
    printf("title         Title One\n");
    printf("title2        Title Two\n");
    printf("alias         Unique Name built from First Letter of First Name, Last\n");
    printf("              Name and a Number\n\n");
    return (LR_OK);
}

#endif

/*
 * execute a query request
 */
int
DoQuery(in_line)
    char *in_line;
{
    char scratch[4096];
    char *args;
    int  noReformatWas = NoReformat;
    int  code;

    if (ReturnFields && !issub(in_line, "return")) {
	args = in_line + strlen(in_line) - 1;
	sprintf(args, " return %s\n", ReturnFields);
	for (; *args; args++)
	    if (*args == ',')
		*args = ' ';
    }
    if (!NoBeautify && !NoReformat) {
	char *ret = issub(in_line, "return");

	if (ret)
	    NoReformat = !issub(ret, "email");
    }
    if (!DefType || issub(in_line, "type=")) {
	if (Debug)
	    fprintf(stderr, "sent=%s", in_line);	/* send command */
	qprintf(ToQI, "%s", in_line);	/* send command */
    }
    else {
	strcpy(scratch, in_line);
	args = strtok(scratch, " \t");
	args = strtok(0, "\n");
	if (args) {
	    if (Debug)
		fprintf(stderr, "sent=query type=\"%s\" %s\n", DefType, args);	/* send command */
	    qprintf(ToQI, "query type=\"%s\" %s\n", DefType, args);	/* send command */
	}
	else {
	    if (Debug)
		fprintf(stderr, "sent=%s", in_line);
	    qprintf(ToQI, "%s", in_line);
	}
    }

    qflush(ToQI);
    code = (NoBeautify ? PrintResponse(1) : PrintQResponse(1, 0));
    NoReformat = noReformatWas;
    return (code);
}

/*
 * execute a login request, using qiapi.
 */
int
DoLogin(in_line)
    char *in_line;
{
    char scratch[4096];
    char *uname = NULL, *upass = NULL, *r;
    int  opts;

    if (QiAuthDebug)
	fprintf(stderr, "DoLogin(%s)\n", in_line);
    strcpy(scratch, in_line);
    (void) strtok(scratch, DELIM);	/* the login part of the command */
    uname = strtok(NULL, DELIM);/* the username, if any  */
    if (uname)
	upass = strtok(NULL, DELIM);	/* the password, if any  */
    opts = LQ_ALL | LQ_INTERACTIVE | ((uname) ? 0 : LQ_AUTO);
    if ((r = LoginQi(UseHost, ToQI, FromQI, opts, uname, upass)) != NULL) {
	strcpy(MyAlias, r);
	return LR_OK;
    }
    else {
	*MyAlias = '\0';
	return LR_ERROR;
    }
}

/*
 * execute a quit request
 */
int
DoQuit(in_line)
    char *in_line;
{
    DoOther("quit\n");
#ifdef VMS
    exit(SS$_NORMAL);
#else
    exit(LastCode < LR_OK || LastCode >= LR_MORE ? LastCode / 100 : 0);
#endif
}

/*
 * edit a field
 */
int
DoEdit(in_line)
    char *in_line;
{
    char *field;
    char *alias;
    char *value;
    int  code = LR_OK;
    char confirm[10];

    (void) strtok(in_line, DELIM);	/* skip ``edit'' */
    if (!(field = strtok((char *) 0, DELIM))) {
	(void) DoHelp("help edit\n");
	return (LR_ERROR);
    }
    if (!(alias = strtok((char *) 0, DELIM)))
	alias = MyAlias;

    if ((value = GetValue(alias, field)) && EditValue(value)) {
	for (code = UpdateValue(alias, field, value);
	     400 <= code && code <= 499;
	     code = UpdateValue(alias, field, value)) {
	    if (!isatty(0))
		break;
	    printf("Shall I try again [y/n]? ");
	    fgets(confirm, sizeof(confirm), stdin);
	    if (*confirm != 'y' && *confirm != 'Y')
		break;
	}
	if (code < 300 && !strcmp(field, "alias"))
	    strcpy(MyAlias, value);
    }
    return (code);
}

/*
 * get the value of a field from the nameserver
 */
char *
GetValue(alias, field)
    char *alias, *field;
{
    static char value[MAXVAL];	/* will hold the value */
    char *vSpot;
    char scratch[MAXSTR];
    int  code;

    if (!strcmp(field, "password")) {
	puts("Use the ``password'' command, not edit.");
	return (NULL);
    }
    /* do the query */
    if (Debug)
	fprintf(stderr, "sent=query alias=%s return %s\n", alias, field);
    qprintf(ToQI, "query alias=%s return %s\n", alias, field);
    qflush(ToQI);

    *value = '\0';

    /* read qi response lines, concatenating the responses into one value */
    for (vSpot = value;; vSpot += strlen(vSpot)) {
	if (!GetGood(scratch, MAXSTR, FromQI)) {
	    fprintf(stderr, "Ding-dong the server's dead!\n");
	    exit(0);
	}
	if ((code = atoi(scratch)) == -LR_OK)
	    strcpy(vSpot, strchr(GetQValue(scratch), ':') + 2);	/* part of value */
	else if (code >= LR_OK)
	    break;		/* final response */
	else
	    fputs(scratch, stdout);	/* ??? */
    }

    if (code != LR_OK)		/* error */
	fputs(scratch, stdout);

    return (code == LR_OK ? value : NULL);
}

/*
 * Edit a value
 */
int
EditValue(value)
    char *value;		/* the value to edit */
{
    char *fname;		/* name of temp file to use */

#ifdef VMS
    struct dsc$descriptor_s cli_input;
    char template[28], f1[28], f2[28], edit_command[64];
    int  istat;

#else
    char template[20];

#endif
    int  fd;			/* file descriptor for temp file */
    static char nvalue[MAXVAL];	/* new value */
    char *editor;		/* editor program to use */
    int  bytes;			/* number of bytes in file */
    char *from, *to;
    int  badc;			/* did we find a bad character? */

#ifdef WAIT_INT
    int  junk;

#else
    union wait junk;

#endif
    char scratch[80];

    /* put the value into a temp file */
#ifdef VMS
    strcpy(template, "SYS$SCRATCH:PHXXXXXX.TMP");
    fname = mktemp(template);
    strcpy(f1, fname);
    strcpy(f2, fname);
    strcat(f1, ";1");		/* versions needed for delete function */
    strcat(f2, ";2");
    if ((fd = creat(fname, 0)) < 0)
#else /* !VMS */
# ifdef OS2
    strcpy(template, "c:\\temp\\phXXXXXX");
# else /* !OS2 */
    strcpy(template, "/tmp/phXXXXXX");
# endif /* OS2 */
    fname = mktemp(template);
    if ((fd = open(fname, O_RDWR | O_CREAT, 0777)) < 0)
#endif /* VMS */
    {
	perror(fname);
	return (0);
    }
    if (write(fd, value, strlen(value)) < 0) {
	perror(fname);
	(void) close(fd);
	return (0);
    }
    (void) close(fd);

    /* run an editor on the temp file */
#ifdef VMS
    if (!(editor = getenv("EDITOR")))
	editor = "EDIT/EDT";

    strcpy(edit_command, editor);
    strcat(edit_command, " ");
    strcat(edit_command, fname);
    cli_input.dsc$w_length = strlen(edit_command);	/* descriptor for spawn */
    cli_input.dsc$a_pointer = edit_command;
    cli_input.dsc$b_class = DSC$K_CLASS_S;
    cli_input.dsc$b_dtype = DSC$K_DTYPE_T;

    if ((istat = LIB$SPAWN(&cli_input)) != SS$_NORMAL) {
	(void) delete(f1);
	exit(istat);
    }
#else /* !VMS */
# ifdef OS2
    {
	int rc, mode=P_WAIT;

	if (!(editor = getenv("EDITOR"))) {
	    editor = "epm.exe";
	    mode = P_PM;
	}
	rc = spawnlp( mode, editor, editor, fname, NULL);
	if (rc == -1)
	    fprintf(stderr, "Whoops!  Failed to exec %s\n", editor);
	if (mode == P_PM)
	    (void) wait(&junk);
    }
# else /* !OS2 */
    if (!(editor = getenv("EDITOR")))
	editor = "vi";
    if (fork())
	(void) wait(&junk);
    else {
	(void) execlp(editor, editor, fname, NULL);
	fprintf(stderr, "Whoops!  Failed to exec %s\n", editor);
	exit(1);
    }
# endif /* OS2 */
#endif /* VMS */

    /* does the user want the value? */
    if (Confirm) {
	do {
	    printf("Change the value [y]? ");
	    gets(scratch);
	}
	while (*scratch && !strchr("yYnN", *scratch));
    }
    /* read the value back out */
    if ((fd = open(fname, 0)) < 0) {
	perror(fname);
#ifdef VMS
#else
	(void) unlink(fname);
#endif
	return (0);
    }
#ifdef VMS
#else
    (void) unlink(fname);
#endif

    if ((bytes = read(fd, nvalue, MAXSTR - 1)) < 0) {
	perror(fname);
	(void) close(fd);
#ifdef VMS
	(void) delete(f1);	/* delete 1st temp file */
	(void) delete(f2);	/* delete 2nd temp file */
#endif
	return (0);
    }
    (void) close(fd);
#ifdef VMS
    (void) delete(f1);		/* delete 1st temp file */
    (void) delete(f2);		/* delete 2nd temp file */
#endif
    nvalue[bytes] = 0;

    /* did the value change? */
    if (Confirm && *scratch && *scratch != 'y' && *scratch != 'Y' ||
	!strcmp(nvalue, value))
	return (0);

    /* copy new value into old, stripping bad characters */
    badc = 0;
    for (to = value, from = nvalue; *from; from++)
	if (*from == '"') {
	    *to++ = '\\';
	    *to++ = '"';
	}
	else if (*from >= ' ' && *from <= '~')
	    *to++ = *from;
	else if (*from == '\t') {
	    *to++ = '\\';
	    *to++ = 't';
	}
	else if (*from == '\n') {
	    if (*(from + 1)) {	/* skip terminating newline from vi */
		*to++ = '\\';
		*to++ = 'n';
	    }
	}
	else
	    badc = 1;

    *to = 0;

    if (badc) {			/* complain if we found bad characters */
	fputs("Illegal characters were found in your value.\n", stderr);
	fputs("Please use only printable characters, newlines, and tabs.\n", stderr);
	fputs("The offending characters were removed.\n", stderr);
    }
    return (1);
}

/*
 * update a nameserver field with a new value
 */
int
UpdateValue(alias, field, value)
    char *alias, *field, *value;
{
    if (Debug)
	fprintf(stderr, "sent=change alias=%s make %s=\"%s\"\n", alias, field, value);
    qprintf(ToQI, "change alias=%s make %s=\"%s\"\n", alias, field, value);
    qflush(ToQI);

    return (PrintResponse(0));
}

/*
 * print info on current user
 */
/*ARGSUSED*/
int
DoMe(in_line)
    char *in_line;
{
    if (!*MyAlias) {
	return (DoHelp("help me"));
    }
    if (Debug)
	fprintf(stderr, "sent=query alias=%s return all\n", MyAlias);
    qprintf(ToQI, "query alias=%s return all\n", MyAlias);
    qflush(ToQI);

    return (NoBeautify ? PrintResponse(0) : PrintQResponse(0, 0));
}

/*
 * set command-line switches
 */
int
DoSwitch(in_line)
    char *in_line;
{
    in_line = strtok(in_line, DELIM);
    if (!OptionLine(strtok(0, "\n"))) {
	printf("The following things can be changed with \"switch\":\n\n");
	printf("  Paging is %s; use \"switch -%c\" to turn it %s.\n",
	       NoPager ? "OFF" : "ON",
	       NoPager ? 'M' : 'm',
	       NoPager ? "on" : "off");
	printf("  Email reformatting is %s; use \"switch -%c\" to turn it %s.\n",
	       NoReformat ? "OFF" : "ON",
	       NoReformat ? 'R' : 'r',
	       NoReformat ? "on" : "off");
	printf("  Query beautification is %s; use \"switch -%c\" to turn it %s.\n",
	       NoBeautify ? "OFF" : "ON",
	       NoBeautify ? 'B' : 'b',
	       NoBeautify ? "on" : "off");
	printf("  Label printing is %s; use \"switch -%c\" to turn it %s.\n",
	       NoLabels ? "OFF" : "ON",
	       NoLabels ? 'L' : 'l',
	       NoLabels ? "on" : "off");
	printf("  Edit confirmation is %s; use \"switch -%c\" to turn it %s.\n",
	       Confirm ? "ON" : "OFF",
	       Confirm ? 'c' : 'C',
	       Confirm ? "off" : "on");
	printf("  Default entry type is %s; use \"switch -%c%s\" to %s %s.\n",
	       DefType ? DefType : "OFF",
	       DefType ? 'T' : 't',
	       DefType ? "" : " name-of-type",
	       DefType ? "turn it" : "set it to",
	       DefType ? "off" : "\"name-of-type\"");
	printf("  Default field list is %s; use \"switch -%c%s\" to %s to %s.\n",
	       ReturnFields ? ReturnFields : "default",
	       ReturnFields ? 'F' : 'f',
	       ReturnFields ? "" : " field1,field2,... ",
	       ReturnFields ? "revert" : "set it",
	       ReturnFields ? "default" : "\"field1,field2,...\"");
	printf("\nThe following things cannot be changed with \"switch\":\n\n");
	printf("  Connected to server %s at port %d\n", UseHost, UsePort);
	printf("  The .netrc file was %sread.\n", NoNetrc ? "not " : "");
	printf("  The -h switch is meaningless in interactive mode.\n");
    }
    return (LR_OK);
}

/*
 * change a field value from the command line
 */
int
DoMake(in_line)
    char *in_line;
{
    int  code = LR_ERROR;
    char *token;

    if (!*MyAlias)
	DoHelp("help make");
    else {
	char scratch[MAXSTR];

	(void) strcpy(scratch, in_line);
	for (token = strtok(scratch, " \n"); token;
	     token = strtok(0, " \n")) {
	    if (!strncmp(token, "password=", 9)) {
		printf("Use the ``password'' command, not make.\n");
		return (LR_OK);
	    }
	}
	if (Debug)
	    fprintf(stderr, "sent=change alias=%s %s", MyAlias, in_line);
	qprintf(ToQI, "change alias=%s %s", MyAlias, in_line);
	qflush(ToQI);
	code = PrintResponse(0);
	if (code < 300)
	    for (token = strtok(in_line, " \n"); token; token = strtok(0, " \n"))
		if (!strncmp(token, "alias=", 6)) {
		    strcpy(MyAlias, token + 6);
		    break;
		}
    }
    return (code);
}

/*
 * register the current account
 */
int
DoRegister(in_line)
    char *in_line;
{
    int  code = LR_ERROR;
    struct passwd *pw;
    char hostname[MAXHOSTNAMELEN];

    if (!*MyAlias)
	DoHelp("help register");
    else if ((pw = getpwuid(getuid())) && !gethostname(hostname, sizeof(hostname))
	     && strcmp(pw -> pw_name, "phones")) {
	if (Debug)
	    fprintf(stderr, "sent=change alias=%s make email=%s@%s\n",
		    MyAlias, pw -> pw_name, hostname);
	qprintf(ToQI, "change alias=%s make email=%s@%s\n",
		MyAlias, pw -> pw_name, hostname);
	qflush(ToQI);
	code = PrintResponse(0);
    }
    return (code);
}

/*
 * change password
 */
int
DoPassword(in_line)
    char *in_line;
{
    char password[80];
    char *confirm;
    char *alias;
    int  code = LR_ERROR;

    if (!*MyAlias) {
	return (DoHelp("help password"));
    }
    /* which alias to use? */
    (void) strtok(in_line, DELIM);
    if (!(alias = strtok((char *) 0, DELIM)))
	alias = MyAlias;

    /* get the password */
    strcpy(password, getpass("Enter new password: "));
    if (!*password)
	return (LR_ERROR);
    confirm = getpass("Type it again: ");
    if (strlen(password) > PH_PW_LEN)
	*(password + PH_PW_LEN) = '\0';
    if (strlen(confirm) > PH_PW_LEN)
	*(confirm + PH_PW_LEN) = '\0';
    if (strcmp(confirm, password)) {
	fprintf(stderr, "Sorry--passwords didn't match.\n");
	return (code);
    }
    VetPassword(confirm);	/* complain if we don't like the password */

    /* encrypt and send the password */
    if (!LocalPort)
	password[encryptit(password, confirm)] = '\0';
    if (Debug)
	fprintf(stderr, "sent=change alias=%s %s password=%s\n", alias,
		LocalPort ? "force" : "make", password);
    qprintf(ToQI, "change alias=%s %s password=%s\n", alias,
	    LocalPort ? "force" : "make", password);
    qflush(ToQI);

    /* see what the nameserver says */
    if ((code = PrintResponse(0)) == LR_OK && !strcmp(alias, MyAlias))
#ifdef PRE_ENCRYPT
	crypt_start(crypt(confirm, confirm));
#else
	crypt_start(confirm);
#endif
    return (code);
}

/*
 * complain about passwords we don't like
 */
void
VetPassword(pass)
    char *pass;
{
    if (strlen(pass) < 5 ||	/* too short */
	AllDigits(pass))	/* digits only */
	fputs("That is an insecure password; please change it.\n", stderr);
}

/*
 * is a string all digits
 */
int
AllDigits(str)
    char *str;
{
    for (; *str; str++)
	if (!isdigit(*str))
	    return (0);
    return (1);
}

/*
 * log out the current user
 */
int
DoLogout(in_line)
    char *in_line;
{
    *MyAlias = '\0';
    return (LogoutQi(ToQI, FromQI));
}

/*
 * print the response to a query
 * this strips out all the nameserver reply codes.
 */
int
PrintQResponse(ref_email, help)
    int  ref_email;

    int  help;
{
    char line[MAXSTR];
    int  code = LR_ERROR;
    int  CurPerson = 0;
    int  person;
    char *cp;
    FILE *out;
    char alias[MAXSTR];
    char email[MAXSTR];
    int  copiedEmail = 0;

    out = OpenPager(1);

    *alias = *email = 0;	/* haven't found an alias yet */
    if (NoReformat || !MailDomain)
	ref_email = 0;
    /* get the response */
    while (GetGood(line, MAXSTR, FromQI)) {
	code = atoi(line);
	if (code == LR_NUMRET) {
#ifdef MACC
	    cp = strchr(line, ':');
	    if (cp != 0)
		fprintf(out, "\n%s\n", cp + 1);	/* strchr returns pointer to :
						 * then add one */
#endif				/* MACC */
	}
	else if (code == -LR_OK || code == -LR_AINFO || code == -LR_ABSENT
		 || code == -LR_ISCRYPT) {
	    person = atoi(strchr(line, ':') + 1);
	    /* output a delimiter */
	    if (person != CurPerson) {
		if (*alias && !*email)
		    NotRegisteredLine(alias, out);
		else if (*email) {
		    EmailLine(email, alias);
		    fputs(GetQValue(email), out);
		    *email = 0;
		}
		fputs("----------------------------------------\n", out);
		CurPerson = person;
		copiedEmail = 0;
	    }
	    if (ref_email) {
		cp = GetQValue(line);
		while (*cp && *cp == ' ')
		    cp++;
		if (!strncmp("alias", cp, 5)) {
		    copiedEmail = 0;
		    strcpy(alias, line);
		    continue;
		}
		else if (!strncmp("email", cp, 5)) {
		    strcpy(email, line);
		    copiedEmail = 1;
		    continue;
		}
		else if (*cp == ':' && copiedEmail)
		    continue;
		else
		    copiedEmail = 0;
	    }
	    /* output the line */
	    if (NoLabels && !help)
		fputs(strchr(GetQValue(line), ':') + 2, out);
	    else
		fputs(GetQValue(line), out);
	}
	else if (code != LR_OK)
	    fputs(line, out);	/* error */

	if (code >= LR_OK) {
	    if (*alias && !*email)
		NotRegisteredLine(alias, out);
	    else if (*email) {
		EmailLine(email, alias);
		/* output the line */
		if (NoLabels && !help)
		    fputs(strchr(GetQValue(email), ':') + 2, out);
		else
		    fputs(GetQValue(email), out);
	    }
	    break;
	}
    }

    /* final "delimiter" */
    if (CurPerson)
	fputs("----------------------------------------\n", out);

#ifdef VMS
#else
    if (out != stdout)
	(void) pclose(out);
#endif

    return (code);
}

#ifndef HASSTRTOK
/*
 * break a string into tokens.  this code is NOT lifted from sysV, but
 * was written from scratch.
 */
/*
 * function:   strtok purpose:	to break a string into tokens parameters:
 * s1 string to be tokenized or 0 (which will use last string) s2 delimiters
 * returns:  pointer to first token.  Puts a null after the token. returns
 * NULL if no tokens remain.
 */
char *
strtok(s1, s2)
    char *s1;

    const char *s2;
{
    static char *old = 0;
    char *p1, *p2;

    if (!(s1 || old))
	return (NULL);
    p1 = (s1 ? s1 : old);
    while (*p1 && (strchr(s2, *p1) != NULL))
	p1++;
    if (*p1) {
	p2 = p1;
	while (*p2 && (strchr(s2, *p2) == NULL))
	    p2++;
	if (*p2) {
	    *p2 = '\0';
	    old = ++p2;
	}
	else
	    old = 0;
	return (p1);
    }
    else
	return (NULL);
}

#endif
#ifdef VMS
/*	setterm.c
 *
 *    module in termlib
 *
 *    contains routines to set terminal mode
 *
 *    V1.0 19-jul-84  P. Schleifer  Initial draft
 */

setterm(characteristic, state)
    INT32 *characteristic, *state;
{
    int  status;
    INT32 efn;
    INT32 new_state;
    short term_chan;
    struct char_buff mode;
    struct mode_iosb term_iosb;

    $DESCRIPTOR(term_desc, "TT:");

    /* get event flag */
    status = lib$get_ef(&efn);
    if (status != SS$_NORMAL)
	return (status);

    /* get channel to terminal */
    status = sys$assign(&term_desc, &term_chan, 0, 0);
    if (status != SS$_NORMAL)
	return (status);

    /* if characteristic is BROADCAST, ECHO, or TYPEAHEAD, state must be
     * toggled */
    if (*characteristic == BROADCAST || *characteristic == ECHO || *characteristic == TYPEAHEAD)
	new_state = !(*state);
    else
	new_state = *state;

    /* get current mode */
    status = sys$qiow(efn, term_chan, IO$_SENSEMODE, &term_iosb, 0, 0, &mode, 12, 0, 0, 0, 0);
    if (status != SS$_NORMAL || term_iosb.stat != SS$_NORMAL) {
	sys$dassgn(term_chan);
	return (status);
    }
    /* change characteristics buffer */
    if (new_state == ON)
	mode.basic_char |= *characteristic;
    else
	mode.basic_char &= ~(*characteristic);

    /* $ SET TERM/...  and then deassign channel */
    status = sys$qiow(efn, term_chan, IO$_SETMODE, &term_iosb, 0, 0, &mode, 12, 0, 0, 0, 0);

    sys$dassgn(term_chan);
    lib$free_ef(&efn);

    if (status != SS$_NORMAL)
	return (status);
    else
	return (term_iosb.stat);
}

/*
 * get password from stdin
 *
 * implement for VMS, since VAXCRTL lacks getpass() function.
 */
char *
getpass(prompt)
    char *prompt;
{

    static char line[12];
    static int echo =
    {ECHO}, off =
    {OFF}, on =
    {ON};

    printf(prompt);
    (void) fflush(stdout);	/* prompt */
    setterm(&echo, &off);
    gets(line);
    setterm(&echo, &on);
    puts("");
    return (line);
}

#endif

/*
 * use .netrc to login to nameserver, if possible
 */
void
DoAutoLogin()
{
    char *r;
    int  opts = LQ_ALL | LQ_AUTO;

    if ((r = LoginQi(UseHost, ToQI, FromQI, opts, NULL, NULL)) != NULL)
	strcpy(MyAlias, r);
    else
	*MyAlias = '\0';
}

/*
 * execute a help request
 */
int
DoHelp(in_line)
    char *in_line;
{
    char scratch[256];
    char *token;

    if (*in_line == '?') {
	/* avoid bug for lone ? */
	sprintf(scratch, "help %s", in_line + 1);
	strcpy(in_line, scratch);
    }
    else
	strcpy(scratch, in_line);
    token = strtok(scratch + 4, DELIM);	/* the word after help */
    if (token && !strcmp(token, "native"))	/* looking for native help */
	strcpy(scratch, in_line);	/* leave the command alone */
    else
	sprintf(scratch, "help %s %s", CLIENT, in_line + 4);	/* insert identifier */

    if (Debug)
	fprintf(stderr, "sent=%s", scratch);	/* send command */
    qprintf(ToQI, "%s", scratch);	/* send command */
    qflush(ToQI);
    return (NoBeautify ? PrintResponse(0) : PrintQResponse(0, 1));
}

/*
 * reformat an email line to include an alias address
 * this is kind of a hack since we're working on an already-formatted line
 */
void
EmailLine(email, alias)
    char *email, *alias;
{
    char scratch[MAXSTR];
    char *emSpot;		/* beginning of email account */
    char *alSpot;		/* beginning of nameserver alias */

    if (*alias) {
	emSpot = (char *) strchr(GetQValue(email), ':') + 2;
	alSpot = (char *) strchr(GetQValue(alias), ':') + 2;
	*(char *) strchr(alSpot, '\n') = 0;
	*(char *) strchr(emSpot, '\n') = 0;
	/* overwrite the email label */
	strcpy(alSpot - 2 - strlen("email to"), "email to");
	alSpot[-2] = ':';	/* strcpy clobbered the colon; repair */
	sprintf(scratch, "@%s (%s)\n", MailDomain, emSpot);
	strcat(alias, scratch);
	strcpy(email, alias);	/* leave it in the "email" line */
	*alias = 0;		/* we're done with the alias */
    }
}

/*
 * put out a ``not registered'' line with alias
 */
void
NotRegisteredLine(alias, out)
    char *alias;

    FILE *out;
{
    char scratch[MAXSTR];
    char *cp;

    strcpy(scratch, alias);
    cp = (char *) strchr(GetQValue(scratch), ':');
    strcpy(cp - 7, "email");
    cp[-2] = ':';
    strcpy(cp, "no account registered\n");
    EmailLine(scratch, alias);
    /* output the line */
    if (NoLabels)
	fputs(strchr(GetQValue(scratch), ':') + 2, out);
    else
	fputs(GetQValue(scratch), out);
    *alias = 0;			/* done with alias */
}

/*
 * process a set of options
 */
int
ProcessOptions(argc, argv)
    int  argc;

    char **argv;
{
    int  count = 0;

    /* options processing */
    for (; argc && **argv == '-'; argc--, argv++, count++) {
	for ((*argv)++; **argv; (*argv)++) {
	    switch (**argv) {
	      case 'q':
		Quiet = 1;
		break;
	      case 'r':
		NoReformat = 1;
		break;
	      case 'R':
		NoReformat = 0;
		break;
	      case 'n':
		NoNetrc = 1;
		break;
	      case 'N':
		NoNetrc = 0;
		break;
	      case 'm':
		NoPager = 1;
		break;
	      case 'M':
		NoPager = 0;
		break;
	      case 'b':
		NoBeautify = 1;
		break;
	      case 'B':
		NoBeautify = 0;
		break;
	      case 'D':
		QiDebug++;
		Debug++;
		QiAuthDebug++;
		break;
	      case 'l':
		NoLabels = 1;
		break;
	      case 'L':
		NoLabels = 0;
		break;
	      case 'C':
		Confirm = 1;
		break;
	      case 'c':
		Confirm = 0;
		break;
	      case 'H':
		JustHelp = 0;
		break;
	      case 'h':
		JustHelp = 1;
		break;
	      case 's':
		if (argv[0][1]) {
		    if (UseHost)
			free(UseHost);
		    UseHost = strdup(*argv + 1);
		    MailDomain = NULL;
		    goto whilebottom;
		}
		else if (argc > 1) {
		    if (UseHost)
			free(UseHost);
		    UseHost = strdup(argv[1]);
		    argc--, argv++, count++;
		    MailDomain = NULL;
		    goto whilebottom;
		}
		else
		    fprintf(stderr, "-%c option given without server hostname.\n", **argv);
		break;
	      case 't':
		if (argv[0][1]) {
		    if (DefType)
			free(DefType);
		    DefType = strdup(*argv + 1);
		    goto whilebottom;
		}
		else if (argc > 1) {
		    if (DefType)
			free(DefType);
		    DefType = strdup(argv[1]);
		    argc--, argv++, count++;
		    goto whilebottom;
		}
		else
		    fprintf(stderr, "-%t option given without entry type.\n", **argv);
		break;
	      case 'f':
		if (argv[0][1]) {
		    if (ReturnFields)
			free(ReturnFields);
		    ReturnFields = strdup(*argv + 1);
		    goto whilebottom;
		}
		else if (argc > 1) {
		    if (ReturnFields)
			free(ReturnFields);
		    ReturnFields = strdup(argv[1]);
		    argc--, argv++, count++;
		    goto whilebottom;
		}
		else
		    fprintf(stderr, "-%t option given without field list.\n", **argv);
		break;
	      case 'F':
		if (ReturnFields)
		    free(ReturnFields);
		ReturnFields = 0;
		break;
	      case 'T':
		if (DefType)
		    free(DefType);
		DefType = 0;
		break;
	      case 'p':
		if (isdigit(argv[0][1])) {
		    UsePort = atoi(*argv + 1);
		    goto whilebottom;
		}
		else if (argc > 1 && isdigit(*argv[1])) {
		    UsePort = atoi(argv[1]);
		    argc--, argv++, count++;
		    goto whilebottom;
		}
		else
		    fprintf(stderr, "-%c option given without port number.\n", **argv);
		break;
	      default:
		fprintf(stderr, "Unknown option: -%c.\n", **argv);
	    }
	}
whilebottom:;
    }
    return (count);
}

/*
 * Process a lineful of options
 */
int
OptionLine(line)
    char *line;
{
    int  argc;
    char *argv[MAXARGS];
    char *token;

    if (!line || !*line)
	return (0);

    for (argc = 0, token = strtok(line, DELIM); token; argc++, token = strtok(0, DELIM))
	argv[argc] = token;
    argv[argc] = 0;

    return (ProcessOptions(argc, argv));
}

/*
 * OpenPager - open the user's chosen pager
 */
FILE *
OpenPager(pager)
    int  pager;
{
    char *pname;
    FILE *out;

#ifdef VMS
    return (stdout);		/* simpler to skip paging for right now */
#else
    if (NoPager || pager != 1)
	return (stdout);
    else {
	if ((pname = getenv("PAGER")) == NULL)
	    pname = PAGER;
	if ((out = popen(pname, "w")) == NULL)
	    out = stdout;
	return (out);
#endif
    }
}

/*
 * issub - is one string a substring of another?
 */
char *
issub(string, sub)
    char *string, *sub;
{
    int  len;

    len = strlen(sub);
    for (; *string; string++)
	if (!strncmp(string, sub, len))
	    return (string);
    return (0);
}

/*
 * EnvOptions - grab some options from the environment
 */
void
EnvOptions(name)
    char *name;
{
    char buffer[80];
    char *np, *bp;

    for (np = name, bp = buffer; *np; np++, bp++)
	*bp = islower(*np) ? toupper(*np) : *np;
    *bp = 0;
    (void) OptionLine(getenv(buffer));
}

/*
 * Dot2Addr - turn a dotted decimal address into an inet address
 * ---Assumes 4 octets---
 */
unsigned INT32
Dot2Addr(dot)
    char *dot;
{
    unsigned INT32 addr = 0;

    do {
	addr <<= 8;
	addr |= atoi(dot);
	while (isdigit(*dot))
	    dot++;
	if (*dot)
	    dot++;
    }
    while (*dot);
    return ((unsigned INT32) htonl(addr));
}
