/*
 * Written by Paul Pomes, University of Illinois, Computing Services Office
 * Copyright (c) 1991 by Paul Pomes and the University of Illinois Board
 * of Trustees.
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
 *	This product includes software developed by the University of
 *	Illinois, Urbana and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 * Email:	Paul-Pomes@uiuc.edu	USMail:	Paul Pomes
 * ICBM:	40 06 47 N / 88 13 35 W		University of Illinois - CSO
 *						1304 West Springfield Avenue
 *						Urbana, Illinois,  61801-2910
 */

#ifndef lint
static char rcsid[] = "@(#)$Id: phquery.c,v 1.54 1995/01/17 23:33:07 p-pomes Exp $";
#endif /* lint */

#include "protos.h"
#ifdef __STDC__
# include <unistd.h>
# include <stdlib.h>
# include <string.h>
#else /* !__STDC__ */
# include <strings.h>
#endif /* __STDC__ */

#include <sysexits.h>
#include <sys/stat.h>
#include <assert.h>
#include <qiapi.h>
#include <qicode.h>
#include "phquery.h"
#include "messages.h"

#define		VERSION		"4.4"

/* How to print/log error messages */
#define		DANGER_WILL_ROBINSON(KateBush) \
	{ if (Debug) \
		perror (KateBush); \
	if (Log) { \
		char *xyzzy = malloc(strlen(KateBush)+4); \
		(void) strcpy(xyzzy, KateBush); (void) strcat(xyzzy, ": %m"); \
		syslog (LOG_ERR, xyzzy); free(xyzzy); } \
	finis (); }

/*
**  PHQUERY -- Resolve fuzzy addresses to specific a user@FQDN
**
**	FQDN := Fully Qualified Domain Name
**	Phquery is invoked as a mailer (not a final mailer!) by sendmail
**	to resolve addresses of the form user@some.domain where some.domain
**	is one of the m4 ALTERNATENAMES define used in building an IDA
**	sendmail.cf file.  At UIUC this would be user@uiuc.edu .
**
**	The user token is interpreted first as a QI alias, then as a full
**	name if that fails.  QI is the CSnet Query Interpreter.  At UIUC it
**	contains the entire campus phone directory plus the unit directory.
**	A user entry has about as many fields as ls has option letters.
**	The most important are alias, name, **	email, phone, department,
**	and curriculum.  In the simplest case, matching an alias (guaranteed
**	unique) returns the email address.
**
**	Since life is seldom simple, the alternate cases/actions are summarized
**
**	a) alias match, email found
**		write a X-PH-To: header with the email address found, copy the
**		rest of the message, and re-invoke sendmail
**	     OR
**		write a X-PH: VX.Y@<host> and re-invoke sendmail.  This is
**		useful for sites that don't wish to expand alias lists in the
**		header block.
**	b) alias match, no email field:
**		return public fields of ph entry and suggest phone usage
**	c) alias match, bogus email field:
**		sendmail catches this one.  The user will see the X-PH-To:
**		header.  Not the best so far.....
**	d) alias fail:
**		try name field
**	e) single name match, email present:
**		deliver as in a)
**	f) single name match, no email field:
**		handle as in b)
**	g) single name match, bogus email field:
**		handle as in c)
**	h) multiple (<5) name matches:
**		return alias, name, email, and dept fields of matches
**	i) multiple (>5) name matches:
**		return "too ambiguous" message
**
**	Phquery is also used to create return addresses of the form
**	ph-alias@ALTERNATENAMES.  This is implemented by adding the fields
**
**	Resent-From: postmaster@<host>
**	Reply-To: ph-alias@ALTERNATENAMES
**	Comment: Reply-To: added by phquery (Vx.y)
**
**	N.B., RFC-822, Section 4.4.1 requires that the From / Resent-From
**	fields be a single, authenticated machine address.
*/

FILE	*ToQi =		FILE_NULL;	/* write to the QI */
FILE	*FromQi =	FILE_NULL;	/* read from the QI */

extern int	errno;

/* Set to carbon-copy postmaster on error returns */
int	PostmasterCC =	0;

#ifdef REPLYTO
/* Set if the reply-to: field on outgoing mail is to inserted */
int	ReplyTo = 0;
#endif /* REPLYTO */

/* Hostname of this machine */
char	HostNameBuf[100];

/* How program was invoked (argv[0]) for error messages */
char	*MyName;

/* Exit status for finis() reporting to calling process */
int	ExitStat =	EX_TEMPFAIL;

/* Field values of email and alias from ph/qi server */
int	EmailVal, AliasVal;

/* Temporary message file */
char	TmpFile[] =	"/tmp/PhMailXXXXXXX";

/* Temporary file for creating error messages */
char	ErrorFile[] =	"/tmp/PhErrMailXXXXXXX";

/* Temporary file for rewriting messages */
char	NewFile[] =	"/tmp/PhNewMailXXXXXXX";

/*
 * How to report events: Debug set for stderr messages, Log for syslog.
 * Setting Debug disables fork/execve in ReMail.
 */
int	Debug =		0;
int	Log =		1;

/* From address supplied by caller */
char	*From =		CPNULL;

/* Name of qi server */
char	*QiHost = NULL;		/* Initial Qi server */

char	*usage[] = {
	"usage: %s [-d] [-p] [-l] [-R] [-i] [-s server] [-x service] [-f FromAddress] address1 [address2]",
	CPNULL
};

void ErrorReturn __P((NADD *, FILE *, char *[]));
void FindFrom __P((FILE *));
void ReMail __P((NADD *, FILE *, char *[]));
char * CodeString __P((int));
FILE * OpenTemp __P((const char *));
QIR * PickField  __P((QIR *, int));
void Query __P((NADD *));
int SendQuery __P((NADD *, const char *, const char *));
void RevQuery __P((NADD *));
char * Malloc __P((unsigned int));
void PrintMsg __P((FILE *, char *[]));
char * Realloc __P((char *, unsigned int));
void PrtUsage();
void finis();

main(argc, argv, envp)
int	argc;
char	*argv[], *envp[];
{
	extern	int	optind;		/* from getopt () */
	extern	char	*optarg;	/* from getopt () */
		int	option;		/* option "letter" */
		int	i;		/* good ol' i */
		char	*Service = CPNULL; /* ph alias from -x */
		FILE	*Msg;		/* stream pointer for temp file */
		NADD	*New, *NewP;	/* translated addresses */
		char	Buf[MAXSTR];
	extern	char	HostNameBuf[];

#ifdef SA_FULLDUMP
	/* AIX does not provide the data section in a core dump by default. */
	struct sigaction handlr;

	handlr.sa_handler = NULL;
	handlr.sa_flags = SA_FULLDUMP;
	sigaction(SIGSEGV, &handlr, NULL);
	sigaction(SIGIOT, &handlr, NULL);
#endif /* SA_FULLDUMP */

	(void) chdir ("/usr/tmp");

	MyName = ((MyName = strrchr (*argv, '/')) == CPNULL)
		? *argv : (MyName + 1);

	while ((option = getopt (argc, argv, "f:r:s:x:pRdli")) != EOF) {
		switch (option) {
		    case 'f':
			From = optarg;
			break;

		    case 's':
			QiHost = optarg;
			break;

		    case 'x':
			Service = optarg;
			break;

		    case 'R':
#ifdef REPLYTO
			/* Re-write outgoing address with Reply-To: field */
			ReplyTo++;
#endif /* REPLYTO */
			break;

		    case 'r':
			From = optarg;
			break;

		    case 'p':
			PostmasterCC++;
			break;

		    case 'l':
			Log++;
			break;

		    case 'd':
			Debug++;
			QiDebug = Debug - 1;
			Log = 0;
			break;

		    case 'i':
		    default:
			PrtUsage ();
			finis ();
			break;
		}
	}
	argc -= optind;			/* skip options */
	argv += optind;

	/* Fire up logging, or not, as the flags may be */
	if (Log)
#ifdef LOG_MAIL
# ifndef SYSLOG
#  define	SYSLOG		LOG_MAIL
# endif
		openlog(MyName, LOG_PID, SYSLOG);
#else
		openlog(MyName, LOG_PID);
#endif

	if (Log && From)
		syslog (LOG_DEBUG, "From %s", From);

	/* fetch our host name, some use will be found for it.... */
	if (gethostname (HostNameBuf, 100-1) != 0)
		DANGER_WILL_ROBINSON("gethostname")

	/* Is the qi server open for business? */
	if (!QiHost)
		QiHost = QI_HOST;
	if ((i = OpenQi(QiHost, &ToQi, &FromQi)) < 0) {
#ifdef QI_ALT
		i = OpenQi(QI_ALT, &ToQi, &FromQi);
#endif /* QI_ALT */
	}
	if (i < 0) {
		DANGER_WILL_ROBINSON("No qi servers available");
	}
	else {
		/* Nail down AliasVal and EmailVal */
		QIF *QFp;

		for (QFp = QiFields; QFp < (QiFields + QiHighKey + 1); QFp++) {
			if (QFp->value == NULL)
				continue;
			if (equal(QFp->value, "alias")) {
				AliasVal = QFp - QiFields;
				QFp->returnOK++;
			}
			else if (equal(QFp->value, "name"))
				QFp->returnOK++;
			else if (equal(QFp->value, "curriculum"))
				QFp->returnOK++;
			else if (equal(QFp->value, "phone"))
				QFp->returnOK++;
			else if (equal(QFp->value, "department"))
				QFp->returnOK++;
			else if (equal(QFp->value, "title"))
				QFp->returnOK++;
			else if (equal(QFp->value, "left_uiuc"))
				QFp->returnOK++;
			else if (equal(QFp->value, "email"))
				EmailVal = QFp - QiFields;
		}
	}

	/* Open the temp file, copy the message into it */
	Msg = OpenTemp (TmpFile);
	while ((i = fread (Buf, sizeof (char), MAXSTR, stdin)) != 0)
		if (fwrite (Buf, sizeof (char), i, Msg) != i)
			DANGER_WILL_ROBINSON("Msg copy")
	if (fflush(Msg) < 0)
		DANGER_WILL_ROBINSON("Msg fflush")

	/*
	 * Remaining arguments are addresses.  If From == CHNULL,
	 * then submission was done locally and return address has
	 * to be on the From: line.
	 */
	if (From == CPNULL || (From != CPNULL && From == CHNULL))
		FindFrom (Msg);

#ifdef REPLYTO
	if (ReplyTo) {

		/*
		 * Check with QI to see if this person has a email entry.
		 * If so add the Resent-From, Reply-To, and Comment fields.
		 * Then invoke ReMail with xyzzy appended to the From address
		 * so that sendmail won't send it back to us.  If a
		 * Reply-To: field is already present, handle as though no
		 * email field was found.
		 */

		/*
		 * Allocate NewAddress structs for from address, to addresses,
		 * plus 1 for terminal null.
		 */
		New = (NADD *) Malloc ((unsigned) ((argc+2) * sizeof (NADD)));
		(New + argc + 1)->original = CPNULL;
		NewP = New;
		RevQuery (NewP);
		assert (NewP->new != CPNULL);

		/* If a single alias was found, append the domain */
		if (abs (NewP->code) == LR_OK) {
			NewP->new =
			    Realloc (NewP->new, (unsigned) (strlen (NewP->new)
							+ strlen (DOMAIN) + 2));
			(void) strcat (NewP->new, "@");
			(void) strcat (NewP->new, DOMAIN);
		}

		/* Add To: addresses to NewP array */
		NewP++;
		while (argc > 0) {
			NewP->original = *argv;
			NewP->new = CPNULL;
			NewP++; argv++; argc--;
		}

		/* ReMail will add the new headers and call sendmail */
		ReMail (New, Msg, envp);

		/* We done good. */
		ExitStat = EX_OK;
		finis ();
	}
#endif /* REPLYTO */

	/*
	 * If not a ReplyTo ...
	 * Allocate NewAddress structs for addresses (or just one if this
	 * is a service forward).
	 */
	i = (Service == CPNULL) ? argc : 1;
	New = (NADD *) Malloc ((unsigned) ((i+1) * sizeof (NADD)));
	(New + i)->original = CPNULL;
	NewP = New;

	if (Service != CPNULL) {
		NewP->original = Service;
		NewP->new = CPNULL;
		Query (NewP);
		assert (NewP->new != CPNULL && NewP->field != -1);
		if (Debug)
			printf ("code %d, (%s) %s --> %s\n",
			    NewP->code, QiFields[NewP->field].value,
			    NewP->original, NewP->new);
		if (Log)
			syslog (LOG_INFO, "(%s) %s --> %s",
			    QiFields[NewP->field].value, NewP->original, NewP->new);
	}
	else
		/* Loop on addresses in argv building up translation table */
		while (argc > 0) {
			NewP->original = *argv;
			NewP->new = CPNULL;
			Query (NewP);
			assert (NewP->new != CPNULL && NewP->field != -1);
			if (Debug)
				printf ("code %d, (%s) %s --> %s\n",
				    NewP->code, QiFields[NewP->field].value,
				    NewP->original, NewP->new);
			if (Log)
				syslog (LOG_INFO, "(%s) %s --> %s",
				    QiFields[NewP->field].value, NewP->original,
				    NewP->new);
			NewP++; argv++; argc--;
		}

	/*
	 * Now re-invoke sendmail with the translated addresses.
	 * Make one pass for collecting error returns into one message.
	 */
	for (NewP = New; NewP->original != CPNULL; NewP++)
		if (abs (NewP->code) != LR_OK) {
			ErrorReturn (NewP, Msg, envp);
			break;
		}

	/* Any good addresses? */
	for (NewP = New; NewP->original != CPNULL; NewP++)
		if (abs (NewP->code) == LR_OK) {
			ReMail (NewP, Msg, envp);
			break;
		}

	/* exit */
	ExitStat = EX_OK;
	finis ();
}
/*
**  ErrorReturn -- Create and send informative mail messages
**
**	The envelope from address should be set to null as per RFC-821
**	in regard to notification messages (Section 3.6).
**
**	Parameters:
**		Addr -- pointer to NewAddress structure with addresses
**			and messages
**		Omsg -- stream pointer to original message
**		envp -- environment pointer for fork/execve
**
**	Returns:
**		Nothing
**
**	Side Effects:
**		None
*/

char	*ap[] = { "-sendmail", "-oi", "-f", "MAILER-DAEMON", "-t", 0};

void
ErrorReturn (Addr, Omsg, envp)
	NADD	*Addr;
	FILE	*Omsg;
	char	*envp[];
{
		int	i;			/* Good ol' i */
		char	Buf[MAXSTR];		/* Temp for copying msg test */
		FILE	*Emsg;			/* For creating the error msg */
		int	pid;			/* For fork() */
		int	flags = 0;		/* Controls printing of msgs */
		int	SubCode;		/* Printing control */
		NADD	*AddrP;			/* Loop variable */
		QIR	*QRp;			/* Another loop variable */
	extern	char	*ap[];

	/* Open the error file */
	Emsg = OpenTemp (ErrorFile);

	/* Insert the headers */
	if (fprintf (Emsg, "To: %s\n", From) < 0)
		DANGER_WILL_ROBINSON("ErrorReturn: To")
	if (PostmasterCC)
		if (fprintf (Emsg, "Cc: Postmaster\n") < 0)
			DANGER_WILL_ROBINSON("ErrorReturn: Cc")
	if (fprintf (Emsg, "Subject: Returned mail - nameserver error report\n\n") < 0)
		DANGER_WILL_ROBINSON("ErrorReturn: Subject")
	if (fprintf (Emsg, " --------Message not delivered to the following:\n\n") < 0)
		DANGER_WILL_ROBINSON("ErrorReturn: Message")
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) != LR_OK)
			if (fprintf (Emsg, " %15s    %s\n", AddrP->original, AddrP->new) < 0)
				DANGER_WILL_ROBINSON("ErrorReturn: addr")
	if (fprintf (Emsg, "\n --------Error Detail (phquery V%s):\n\n", VERSION) < 0)
		DANGER_WILL_ROBINSON("ErrorReturn: Detail")

	/* Loop again to insert messages */
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) == LR_NOMATCH) {
			if (! (flags & NO_MATCH_MSG)) {
				PrintMsg (Emsg, NoMatchMsg);
				flags |= NO_MATCH_MSG;
				break;
			}
		}
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) == LR_ERROR) {
			if (! (flags & HARD_MSG)) {
				PrintMsg (Emsg, HardMsg);
				flags |= HARD_MSG;
			}
			for (QRp = AddrP->QIalt; QRp->code < 0; QRp++)
				if (fprintf (Emsg, " %d: %s\n", QRp->code, QRp->message) < 0)
					DANGER_WILL_ROBINSON("ErrorReturn: hard")
			if (putc ('\n', Emsg) < 0)
				DANGER_WILL_ROBINSON("ErrorReturn: hard putc")
		}
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) == LR_ABSENT) {
			if (! (flags & ABSENT_MSG)) {
				PrintMsg (Emsg, AbsentMsg);
				flags |= ABSENT_MSG;
			}
			for (QRp = AddrP->QIalt; QRp->code < 0; QRp++)
				if (abs(QRp->code) == LR_OK && QRp->field > 0 &&
				    QiFields[QRp->field].returnOK)
					if (fprintf (Emsg, " %s: %s\n", QiFields[QRp->field].value, QRp->message) < 0)
						DANGER_WILL_ROBINSON("ErrorReturn: absent")
			if (putc ('\n', Emsg) < 0)
				DANGER_WILL_ROBINSON("ErrorReturn: absent putc")
		}
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) == LR_TOOMANY) {
			if (! (flags & TOO_MANY_MSG)) {
				PrintMsg (Emsg, TooManyMsg);
				flags |= TOO_MANY_MSG;
				break;
			}
		}
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) == LR_AMBIGUOUS) {
			if (! (flags & MULTI_MSG)) {
				PrintMsg (Emsg, MultiMsg);
				flags |= MULTI_MSG;
			}
			for (QRp = AddrP->QIalt, SubCode = QRp->subcode;
			    QRp->code < 0; QRp++) {
				if (QRp->subcode != SubCode) {
					SubCode = QRp->subcode;
					if (putc ('\n', Emsg) < 0)
						DANGER_WILL_ROBINSON("ErrorReturn: multi putc")
				}
				if (abs(QRp->code) == LR_OK && QRp->field > 0 &&
				    QiFields[QRp->field].returnOK)
					if (fprintf (Emsg, " %s: %s\n", QiFields[QRp->field].value, QRp->message) < 0)
						DANGER_WILL_ROBINSON("ErrorReturn: multi")
			}
			if (putc ('\n', Emsg) < 0)
				DANGER_WILL_ROBINSON("ErrorReturn: multi putc2")
		}
	if (fprintf (Emsg, "\n --------Unsent Message below:\n\n") < 0)
		DANGER_WILL_ROBINSON("ErrorReturn: unsent below")
	rewind (Omsg);
	while ((i = fread (Buf, sizeof (char), MAXSTR, Omsg)) != 0) {
		if (fwrite (Buf, sizeof (char), i, Emsg) != i)
			DANGER_WILL_ROBINSON("ErrorReturn: Emsg copy")
	}
	if (fprintf (Emsg, "\n --------End of Unsent Message\n") < 0)
		DANGER_WILL_ROBINSON("ErrorReturn: unsent end")
	(void) fflush (Emsg);
	(void) fclose (Emsg);
	if (freopen (ErrorFile, "r", stdin) == FILE_NULL)
		DANGER_WILL_ROBINSON("ErrorReturn: freopen")

	/* Zap file so it disappears automagically */
	if (! Debug)
		(void) unlink (ErrorFile);

	/*
	 * fork, then execve sendmail for delivery
	 */

	pid = 0;
	if (! Debug && (pid = fork ()) == -1)
		DANGER_WILL_ROBINSON("ErrorReturn: fork")
	if (pid) {
		(void) wait(0);
		return;
	}
	else if (! Debug)
		execve (SENDMAIL, ap, envp);
}
/*
**  FindFrom -- Find From: address in message headers
**
**	Parameters:
**		MsgFile -- stream pointer to message
**
**	Returns:
**		Nothing
**
**	Side Effects:
**		Global From pointer is adjusted to point at either a
**		malloc'ed area containing the address, or to the
**		constant string "Postmaster" if none is found.
*/

void
FindFrom (MsgFile)
	FILE	*MsgFile;
{
	char		*p1, *p2;
	extern char	*From;
	char		Buf[MAXSTR];

	rewind (MsgFile);
	while (fgets (Buf, MAXSTR, MsgFile) != CPNULL && *Buf != '\n') {
		if (strncasecmp (Buf, "From:", 5))
			continue;
		else {
			if ((p1 = strchr (Buf, '<')) != CPNULL) {
				p1++;
				if ((p2 = strchr (Buf, '>')) != CPNULL) {
					From = Malloc ((unsigned) ((p2-p1)+1));
					(void) strncpy (From, p1, (p2-p1));
				}
				else {
					if (Debug)
						fprintf (stderr, "Unbalanced <> in From: address\n");
					if (Log)
						syslog (LOG_ERR, "Unbalanced <> in From: address");
					From = "Postmaster";
				}
			}
			else {
				/*
				 * Mail from local users may not have the <>
				 * yet.  See what's there anyway.
				 */
				p1 = &Buf[5];
				while (*p1 && isspace(*p1))
					p1++;
				p2 = Buf + strlen(Buf);
				if (p2 > p1) {
					From = Malloc ((unsigned) ((p2-p1)+1));
					(void) strncpy (From, p1, (p2-p1));
				}
			}
			break;
		}
	}
	if (From == CPNULL || strlen(From) == 0) {
		if (Debug)
			fprintf (stderr, "No From: address in message\n");
		if (Log)
			syslog (LOG_ERR, "No From: address in message");
		From = "Postmaster";
	}
	if (Log)
		syslog (LOG_DEBUG, "From %s", From);
}
/*
**  ReMail -- Forward message to recipients after adding phquery headers
**
**	Parameters:
**		Addr -- pointer to NewAddress structure with addresses
**			and messages
**		Omsg -- stream pointer to original message
**		envp -- environment pointer for fork/execve
**
**	Returns:
**		Nothing
**
**	Side Effects:
**		None
*/

void
ReMail (Addr, Omsg, envp)
	NADD	*Addr;
	FILE	*Omsg;
	char	*envp[];
{
		int	i = 10;
		char	Buf[MAXSTR];
		NADD	*AddrP;
		FILE	*Nmsg;
		int	pid = 0;
		char	**nap, **np, nFrom[100];
	extern	char	*From, HostNameBuf[];

	/* Open the rewrite file */
	Nmsg = OpenTemp (NewFile);

	/* size the argument array */
#ifdef REPLYTO
	if (ReplyTo)
		for (AddrP = Addr, AddrP++; AddrP->original != CPNULL; AddrP++)
			i++;
	else
#endif /* REPLYTO */
	for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
		if (abs (AddrP->code) == LR_OK)
			i++;
	np = nap = (char **) Malloc ((unsigned)(i * sizeof (char *)));

	/* Fill out the first portion of the sendmail argument vector */
	*np++ = "-sendmail";
	*np++ = "-oi";
	*np++ = "-f";
#ifdef REPLYTO
	if (ReplyTo) {
		/*
		 * Tack on .xyzzy to the From address so sendmail will know
		 * it's been here.
		 */
		(void) strcpy (nFrom, From);
		(void) strcat (nFrom, ".xyzzy");
		*np++ = nFrom;
	}
	else
#endif /* REPLYTO */
	{
		*np++ = From;
		for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
			if (abs (AddrP->code) == LR_OK)
				*np++ = AddrP->new;
	}

	/* Read and copy the header block, adding X-PH-To: or X-PH: header */
	rewind (Omsg);
	while (fgets (Buf, MAXSTR, Omsg) != CPNULL && *Buf != '\n') {
		if ((nequal (Buf, "To:", 3) || nequal (Buf, "Cc:", 3)
		    || nequal (Buf, "From:", 5)) && pid == 0) {
			int	LineLength;

#ifdef REPLYTO
			if (ReplyTo) {

				/* Add the Reply-To: fields */
				AddrP = Addr;
				if (fprintf (Nmsg, "Comment: Reply-To: added by phquery (V%s)\n", VERSION) < 0)
					DANGER_WILL_ROBINSON("ReMail: comment")
				if (fprintf (Nmsg, "Resent-From: postmaster@%s\n", HostNameBuf) < 0)
					DANGER_WILL_ROBINSON("ReMail: resent")
				if (fprintf (Nmsg, "Reply-To: %s\n", AddrP->new) < 0)
					DANGER_WILL_ROBINSON("ReMail: reply")
				for (AddrP++; AddrP->original != CPNULL; AddrP++)
					*np++ = AddrP->original;
				pid++;
			}
			else
#endif /* REPLYTO */
			{

				/* Write the PH header and add to argv */
#ifdef	EXPAND_TO
				if (fprintf (Nmsg, "X-PH(%s)-To:", VERSION) < 0)
					DANGER_WILL_ROBINSON("ReMail: x-pht")
				LineLength = 8;
				for (AddrP = Addr; AddrP->original != CPNULL; AddrP++)
					if (abs (AddrP->code) == LR_OK) {
						if ((LineLength + strlen (AddrP->new)) > 75) {
							if (fprintf (Nmsg, "\n\t") < 0)
								DANGER_WILL_ROBINSON("ReMail: \\n\\t")
							LineLength = 8;
						}
						if (fprintf (Nmsg, " %s", AddrP->new) < 0)
							DANGER_WILL_ROBINSON("ReMail: x-pht2")
					}
				if (putc('\n', Nmsg) < 0)
					DANGER_WILL_ROBINSON("ReMail: putc")
#else /* ! EXPAND_TO */
				if (fprintf (Nmsg, "X-PH: V%s@%s\n", VERSION, HostNameBuf) < 0)
					DANGER_WILL_ROBINSON("ReMail: x-phv")
#endif /* EXPAND_TO */
				pid++;
			}
		}
		if (fputs (Buf, Nmsg) < 0)
			DANGER_WILL_ROBINSON("ReMail: fputs")
	}
	if (fputs (Buf, Nmsg) < 0)
		DANGER_WILL_ROBINSON("ReMail: fputs2")
	*np = CPNULL;

	if (Debug) {
		printf ("Final send vector:");
		for (np = nap; *np != CPNULL; np++)
			printf (" %s", *np);
		(void) putchar ('\n');
	}

	/* Copy the remainder of the message */
	while ((i = fread (Buf, sizeof (char), MAXSTR, Omsg)) != 0)
		if (fwrite (Buf, sizeof (char), i, Nmsg) != i)
			DANGER_WILL_ROBINSON("ReMail: nmsg copy")

	/* Re-arrange the stream pointers and invoke sendmail */
	if (fflush (Nmsg) < 0)
		DANGER_WILL_ROBINSON("ReMail: fflush")
	(void) fclose (Nmsg);
	if (freopen (NewFile, "r", stdin) == FILE_NULL)
		DANGER_WILL_ROBINSON("ReMail: NewFile freopen")

	/* Zap file so it disappears automagically */
	if (! Debug)
		(void) unlink (NewFile);

	/*
	 * fork, then execve sendmail for delivery
	 */

	pid = 0;
	if (! Debug && (pid = fork ()) == -1)
		DANGER_WILL_ROBINSON("ReMail: fork")
	if (pid) {
		(void) wait(0);
		return;
	}
	else if (! Debug)
		execve (SENDMAIL, nap, envp);
}
/*
**  CodeString -- Return text string corresponding to supplied reply code
**
**	Parameters:
**		code -- reply value
**
**	Returns:
**		char pointer to text string or NULL pointer if no matching
**		key is located.
**
**	Side Effects:
**		None
*/

char *
CodeString (code)
	int	code;
{
		struct QiReplyCode	*Cpnt;
	extern	struct QiReplyCode	QiCodes[];

	for (Cpnt = QiCodes; Cpnt->key != -1; Cpnt++)
		if (Cpnt->key == abs (code))
			return (Cpnt->value);
	return (CPNULL);
}
/*
**  OpenTemp -- Create and open a temporary file
**
**	For the supplied file name, create, open, and chmod the file
**
**	Parameters:
**		Name -- pathname of file to create in mkstemp format
**
**	Returns:
**		Stream descriptor of resulting file, or NULL if error
**
**	Side Effects:
**		mkstemp modifies calling argument
*/

FILE *
OpenTemp (Name)
	const char *Name;
{
	int	fd;
	FILE	*Stream;

	if ((fd = mkstemp ((char *)Name)) == -1)
		DANGER_WILL_ROBINSON("OpenTemp: mkstemp")

	/* Protect it */
	if (fchmod (fd, S_IREAD|S_IWRITE) == -1)
		DANGER_WILL_ROBINSON("OpenTemp: fchmod")

	/* Make fd a stream */
	if ((Stream = fdopen (fd, "r+")) == FILE_NULL)
		DANGER_WILL_ROBINSON("OpenTemp: fdopen")
	return (Stream);
}
/*
**  Query -- Create queries to send to the CSnet central server
**
**	Using the alias, call-sign, and full name fields, as known by the
**	CSnet central name server Query Interpreter, Query creates variants
**	of the supplied name (New->original) if a straight alias lookup fails.
**	For each variant, SendQuery() is called until either one succeeds or
**	all variants are exhausted.
**
**	Parameters:
**		New -- pointer to NewAddress struct
**
**	Returns:
**		None
**
**	Side Effects:
**		Modifies contents under New pointer.
*/

void
Query(New)
	NADD	*New;
{
	char	scratch[MAXSTR];	/* copy of FullName w.o. punct */
	char	*sp;			/* work ptrs for scratch */
#ifdef	WILDNAMES
	char	*sp2;			/* work ptrs for scratch */
#endif /* WILDNAMES */
	char	**Lpnt = TryList;	/* Loop pointer for TryList */
	int	NoMore = -1;		/* set if all name variants done */

	/* Unquote the address if needed */
	while (*New->original == '"' &&
	       New->original[strlen(New->original)-1] == '"') {
		New->original[strlen(New->original)-1] = CHNULL;
		New->original++;
	}

	/*
	 * Try the query as an alias lookup first, then as a full name lookup.
	 */

	do {
		/*
		 * Convert punctuation/separators in scratch to space
		 * characters one at a time if testing for name.  If
		 * WILDNAMES is #define'd, a wildcard char '*' will be
		 * appended after each single character name, e.g. p-pomes
		 * is tried as p* pomes.  This has risks as follows:  assume
		 * Duncan Lawrie sets his alias to "lawrie".  A query for
		 * d-lawrie will fail as a alias lookup but succeed as a
		 * name lookup when written as "d* lawrie".  This works until
		 * Joe Student sets his alias to "d-lawrie".  Whoops.
		 * Still in a non-hostile environment, this function may be
		 * more useful than dangerous.
		 */
		if ((New->field = FieldValue(*Lpnt)) ==  -1)
		{
			if (Debug)
				printf ("%s not a server field\n", *Lpnt);
			if (Log)
				syslog (LOG_ERR, "%s not a server field",*Lpnt);
			Lpnt++;
			continue;
		}
		if (equal (*Lpnt, "name")) {

			/* Try as is first time for hyphenated names */
			if (NoMore == -1) {
				(void) strcpy (scratch, New->original);
				if (SendQuery (New, *Lpnt, scratch))
					return;
				NoMore = 0;
			}
			else {
				char stemp[MAXSTR], *st = stemp;

				for (sp = scratch; *sp != CHNULL; ) {

					/* copy until non-space punct char */
					if (!ispunct (*sp) || *sp == ' ' || *sp == '*') {
#ifdef	WILDNAMES
						sp2 = sp;
#endif /* WILDNAMES */
						*st++ = *sp++;
						if (*sp == CHNULL)
							NoMore++;
						continue;
					}

#ifdef	WILDNAMES
					/* if one non-punct char, append * */
					if ((sp - sp2) == 1)
						*st++ = '*';
#endif /* WILDNAMES */
					*st++ = ' ';
					sp++;
					break;
				}
				while (*sp != CHNULL)
					*st++ = *sp++;
				*st = CHNULL;
				(void) strcpy (scratch, stemp);
				if (SendQuery (New, *Lpnt, scratch))
					return;
				if (NoMore > 0)
					Lpnt++;
				continue;
			}
		}

		/*
		 * Convert punctuation/separators in scratch to hyphen
		 * characters if testing for alias.
		 */
		else if (equal (*Lpnt, "alias")) {
			(void) strcpy (scratch, New->original);
			for (sp = scratch; *sp != CHNULL; sp++)
				if (ispunct(*sp))
					*sp = '-';
			if (SendQuery (New, *Lpnt, scratch))
				return;
			Lpnt++;
		}
		else {
			(void) strcpy (scratch, New->original);
			if (SendQuery (New, *Lpnt, scratch))
				return;
			Lpnt++;
		}
	} while (*Lpnt != CPNULL);
}
/*
**  SendQuery -- Send queries to the local CSnet central name server
**
**	Takes a field type (alias, call-sign, full name, etc), as known by
**	the CSnet central name server Query Interpreter, and looks up the
**	corresponding email address "usercode@host".  Cases where the
**	alias/name aren't found, are ambiguous, or lack an email address
**	return a message instead of the address.  Additional information is
**	returned as an array of QIR records pointed to by New->QIalt.
**
**	Parameters:
**		New -- pointer to NewAddress struct
**		Field -- type of field (name, alias, etc) for Value
**		Value -- name to lookup
**
**	Returns:
**		1 if a match(es) is found including too many
**		0 otherwise
**
**	Side Effects:
**		Modifies contents under New pointer.
*/

SendQuery(New, Field, Value)
	NADD	*New;
	const char *Field, *Value;
{
	QIR	*EmailQ, *QRp;	/* For handling ReadQi() responses */
	int	i;		/* good ol' i */

	/* Make a query out of the arguments */
	if (Debug)
		printf ("querying for %s \"%s\"\n", Field, Value);
	if (Log)
		syslog (LOG_DEBUG, "querying for %s \"%s\"", Field, Value);
	if (fprintf (ToQi, "query %s=%s return all\n", Field, Value) < 0)
		DANGER_WILL_ROBINSON("SendQuery: qi query")
	if (fflush (ToQi) < 0)
		DANGER_WILL_ROBINSON("SendQuery: qi fflush")

	/*
	 * Grab the responses and let the fun begin.
	 * The possibilities are:
	 *
	 * 102:There were N matches to your query
	 * -200:1:         alias: Paul-Pomes
	 * -200:1:          name: pomes paul b
	 * -200:1:      callsign: See Figure 1
	 * -508:1:    curriculum: Not present in entry.
	 * -200:1:    department: Computing Services Office
	 * -200:1:         email: paul@uxc.cso.uiuc.edu
	 * 200:Ok.
	 *
	 * 501:No matches to your query.
	 *
	 * 502:Too many matches to request.
	 *
	 * -515:no non-null key field in query.
	 * -515:Initial metas may be used as qualifiers only.
	 * 500:Did not understand query.
	 */
	EmailQ = ReadQi (FromQi, NULL);

	/*
	 * If we read a temporary error, be a nice program and defer.
	 */
	if (EmailQ->code > 399 && EmailQ->code < 500)
		finis ();

	/*
	 * No matches at all?  Too many?  Note that single line errors
	 * will have code > 0.
	 */
	if (EmailQ->code > 0) {
		New->new = CodeString (EmailQ->code);
		New->code = EmailQ->code;
		New->QIalt = QIR_NULL;
		FreeQIR (EmailQ);
		if (New->code == LR_TOOMANY)
			return (1);
		return (0);
	}

	/* anything else must be multi-line */
	assert (EmailQ->code < 0);

	/* Are there multiple responses (subcode > 1)? */
	for (QRp = EmailQ; QRp->code < 0; QRp++)
		if (QRp->subcode > 1) {
			New->code = LR_AMBIGUOUS;
			New->new = CodeString (LR_AMBIGUOUS);
			New->QIalt = EmailQ;
			return (1);
		}

	/* a multi-line error? */
	New->QIalt = EmailQ;
	if (abs (QRp->code) >= 500) {
		for (QRp = EmailQ; QRp->code < 0; QRp++)
			;
		New->code = QRp->code;
		New->new = CodeString (QRp->code);
		return (1);
	}

	/* If one person, handle as single match alias */
	QRp = PickField (EmailQ, EmailVal);
	if (QRp == QIR_NULL) {
		New->code = LR_ABSENT;
		New->new = CodeString (LR_ABSENT);
		return (1);
	}
	if (QRp->field != EmailVal) {
		if (Log)
			syslog (LOG_ERR, "Email field for %s (%s) in ph/qi database is present but null",
				Value, Field);
		if (Debug)
			fprintf (stderr, "Email field for %s (%s) in ph/qi database is present but null\n",
				Value, Field);
		New->code = LR_ABSENT;
		New->new = CodeString (LR_ABSENT);
		return (1);
	}
	New->code = abs (QRp->code);
	switch (abs (QRp->code)) {
	    case LR_ABSENT:
		New->new = CodeString (QRp->code);
		return (1);

	    case LR_OK:
		New->new = QRp->message;

		/* chop at first address */
		{
			char *cp = New->new;

			/* skip any white space */
			while (*cp != CHNULL && isspace(*cp))
				cp++;

			while (*cp != CHNULL) {
				if (isspace(*cp) || *cp == ',') {
					*cp = CHNULL;
					break;
				}
				cp++;
			}
		}
		return (1);

	    default:
		if (Debug)
			fprintf (stderr, "unexpected code %d\n",
			    QRp->code);
		if (Log)
			syslog (LOG_ERR, "Query: %s: unexpected code %d", Field, QRp->code);
		finis ();
	}
	FreeQIR (EmailQ);
	return (0);
}
/*
**  RevQuery -- Reverse query, email to ph alias
**
**	Takes a email address as known by the CSnet central name server
**	Query Interpreter, and looks up the corresponding alias. Cases
**	where the email address matches multiple aliases return the
**	original address.  In addition the global variable ReplyTo is
**	set to -1.
**
**	Parameters:
**		New -- pointer to NewAddress struct
**
**	Returns:
**		None
**
**	Side Effects:
**		Modifies contents under New pointer.
**		ReplyTo set to -1 if QI returns multiple aliases or
**		no match.
*/

void
RevQuery(New)
	NADD	*New;
{
		int	i;
		QIR	*AliasQ, *QRp;
	extern	char	*From, HostNameBuf[];
	extern	FILE	*ToQi, *FromQi;

	/*
	 * We have to have a from address here.  If it doesn't have
	 * a fully qualified form, convert it to name@domain by
	 * appending our Fully Qualified Domain Name.  FQDN, the
	 * litany of the new Internet Age.
	 */

	assert (From != CPNULL);
	if (strchr (From, '@') == CPNULL) {
		char	*nFrom;

		/*
		 * We can't Realloc(From) since it may point to
		 * an area on the stack.
		 */
		nFrom = Malloc ((unsigned)(strlen (From) + 1));
		(void) strcpy (nFrom, From);
		From = Realloc (nFrom, (unsigned)(strlen(nFrom) +
				       strlen(HostNameBuf) + 5));
		(void) strcat (From, "@");
		(void) strcat (From, HostNameBuf);
	}
	New->original = From;

	/* Send the query
	 * I'd check for a -1 here, but am unsure how network errors really
	 * are manifested.
	 */
	if (Debug)
		printf ("querying alias corresponding to \"%s\"\n", From);
	if (Log)
		syslog (LOG_DEBUG, "querying alias for \"%s\"", From);
	if (fprintf (ToQi, "query email=%s return alias \n", From) < 0)
		DANGER_WILL_ROBINSON("RevQuery: qi query")
	if (fflush (ToQi) < 0)
		DANGER_WILL_ROBINSON("RevQuery: qi fflush")

	/*
	 * Grab the responses and let the fun begin.
	 * The possibilities are:
	 *
	 * 102:There was N matches to your query.
	 *
	 * -200:1:         alias: rrv
	 * 200:Ok.
	 *
	 * -200:1:         alias: Paul-Pomes
	 * -200:2:         alias: PostMaster
	 * 200:Ok.
	 *
	 * 501:No matches to your query.
	 *
	 * 502:Too many matches to request.
	 *
	 * 5XX:Other error
	 *
	 * For anything other than the first case, set ReplyTo to -1 and
	 * set New->new = New->original .
	 */
	AliasQ = ReadQi (FromQi, NULL);

	/* Handle the 501, 502, 5XX codes */
	if (AliasQ->code > 0) {
#ifdef REPLYTO
		ReplyTo = -1;
#endif /* REPLYTO */
		New->new = New->original;
		FreeQIR (AliasQ);
		return;
	}

	/* Are there multiple responses (subcode > 1)? */
	for (QRp = AliasQ; QRp->code < 0; QRp++)
		if (QRp->subcode > 1) {
#ifdef REPLYTO
			ReplyTo = -1;
#endif /* REPLYTO */
			New->new = New->original;
			FreeQIR (AliasQ);
			return;
		}

	QRp = AliasQ;
	assert (abs (QRp->code) == LR_OK && QRp->field == AliasVal);
	New->code = abs (QRp->code);
	New->new = QRp->message;
	return;
}
/*
** Malloc -- malloc with error checking
**
**	Parameters:
**		size -- number of bytes to get
**
**	Returns:
**		(char *) of first char of block, or
**		finis() if any error
**
**	Side Effects:
**		none
*/

char *
Malloc (size)
	unsigned	size;			/* Bytes to get */
{
		char	*cp;		/* Pointer to memory */

	if ((cp = (char *) malloc (size)) == CPNULL) {
		if (Debug) {
			fprintf (stderr, "Malloc: %u bytes failed:", size);
			perror("");
		}
		if (Log)
			syslog (LOG_ERR, "Malloc: %u bytes failed: %m", size);
		finis ();
	}
	return (cp);
}
/*
** PrintMsg -- Print a message on the named stream
**
**	Parameters:
**		OutFile -- stream to print message to
**		Msg - array of char pointers that make up message,
**		      null terminated
**
**	Returns:
**		None
**
**	Side Effects:
**		none
*/

void
PrintMsg (OutFile, Msg)
	FILE	*OutFile;
	char	*Msg[];
{
	while (*Msg != CPNULL) {
		if (fprintf (OutFile, "%s\n", *Msg) < 0)
			DANGER_WILL_ROBINSON("PrintMsg")
		Msg++;
	}
}
/*
** Realloc -- realloc with error checking
**
**	Parameters:
**		ptr -- pointer to existing data
**		size -- number of bytes to get
**
**	Returns:
**		(char *) of first char of block, or
**		finis() if any error
**
**	Side Effects:
**		none
*/

char *
Realloc (ptr, size)
	char		*ptr;
	unsigned	size;
{
		char	*cp;		/* pointer to memory */

	if ((cp = (char *) realloc (ptr, size)) == CPNULL) {
		if (Debug) {
			fprintf (stderr, "Realloc: %u bytes failed:", size);
			perror("");
		}
		if (Log)
			syslog (LOG_ERR, "Realloc: %u bytes failed: %m", size);
		finis ();
	}
	return (cp);
}
/*
** PrtUsage -- Print how to use message
**
**	Print usage messages (char *usage[]) to stderr and exit nonzero.
**	Each message is followed by a newline.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		none
*/

void
PrtUsage ()
{
	int	which = 0;		/* current line */

	fprintf (stderr, "%s: version %s\n", MyName, VERSION);
	while (usage[which] != CPNULL) {
		fprintf (stderr, usage[which++], MyName);
		(void) putc ('\n', stderr);
	}
	(void) fflush (stdout);
}
/*
**  finis -- Clean up and exit.
**
**	Parameters:
**		none
**
**	Returns:
**		never
**
**	Side Effects:
**		exits sendmail
*/

void
finis()
{
	extern	FILE	*ToQi, *FromQi;

	if (ToQi)
		CloseQi(ToQi, FromQi);

	/* clean up temp files */
	if (! Debug) {
		(void) unlink (TmpFile);
		(void) unlink (ErrorFile);
		(void) unlink (NewFile);
	}

	/* and exit */
	exit (ExitStat);
}
