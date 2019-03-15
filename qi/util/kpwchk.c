/*
 * $Source: /usr/local/src/net/qi/util/RCS/kpwchk.c,v $
 * $Author: p-pomes $
 *
 * A lightweight Kerberos password checker.  It does *not* do mutual
 * authentication for faster execution by default.  If someone has
 * compromised the host enough to change where this program looks for
 * Kerberos information, then the information this check authorizes
 * has already been compromised.
 *
 * Don't use this program to mediate remote access without compiling with
 * -DMUTUAL_AUTH=1  !!!  This also requires running with root privs to read
 * /etc/srvtab.
 *
 * Compile with
 *
 * cc -DKRB4_AUTH -DMUTUAL_AUTH=1 -DKDOMAIN=\"UIUC.EDU\" \
 *	-I/usr/local/include/kerberosIV -o kcheck kcheck.c \
 *	-L/usr/local/lib -lkrb -ldes -lresolv -l44bsd -lsocket -lnsl
 *
 * Input is "<login><WS><password>\n"
 *   <WS> is a *single* white space character (not newline)
 *   <password> can have embedded spaces
 * Output is "<login><WS><code><WS><message>\n"
 *   A <code> of "0" indicates success
 *   <message> is the text explaining <code>
 *
 * Copyright (c) 1995 University of Illinois Board of Trustees and Paul Pomes
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

#define MUTUAL_AUTH	1

#ifndef lint
static char rcsid[] = "$Id: kpwchk.c,v 1.4 1995/06/10 17:36:30 p-pomes Exp $";
#endif /* lint */

#include "conf.h"

#ifdef KRB4_AUTH
#include <mit-copyright.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <krb.h>
#include <netdb.h>
#include <sys/ioctl.h>

extern char *krb_err_txt[];
int     kerno;
char    lrealm[REALM_SZ];

int main(argc, argv, envp)
	int argc;
	char **argv, **envp;
{
	extern char **environ;
	extern int errno, optind;
	register char *name, *pwd, *pnt, *end;
	unsigned long faddr;
	KTEXT_ST ticket;
	AUTH_DAT authdata;
	char buf[BUFSIZ];
	char myname[256];
	char hostname[MAXHOSTNAMELEN], savehost[MAXHOSTNAMELEN];

	envp = 0;	/* *IMPORTANT* Throw environment away */
	if ((pwd = strrchr(argv[0], '/')) != NULL)
		strcpy(myname, pwd+1);
	else
		strcpy(myname, argv[0]);
	openlog(myname, 0, LOG_AUTH);
#ifdef KRBNSREALM
	strcpy(lrealm, KRBNSREALM);
#else
	if (krb_get_lrealm(lrealm, 1) != KSUCCESS) {
		fprintf(stderr,"Unable to get local realm\n");
		exit(1);
	}
#endif
	errno = 0;
	name = buf;
	end = &buf[1024-1];

	if (gethostname(hostname, sizeof(hostname)) == -1) {
		perror("cannot retrieve hostname");
		exit(1);
	}
	(void) strncpy(savehost, (char *)krb_get_phost(hostname),
		sizeof(savehost));
	savehost[sizeof(savehost)-1] = 0;

	while (fgets(buf, BUFSIZ-1, stdin) != NULL) {
		for (pwd = buf; pwd < end && *pwd && !isspace(*pwd);
		     pwd++)
			;
		*pwd = '\0';
		if (pwd < end)
			pwd++;
		for (pnt = pwd; pnt < end && *pnt && *pnt != '\n'; pnt++)
			;
		*pnt = '\0';
		kerno = krb_get_pw_in_tkt(name, "", lrealm, "krbtgt",
		      lrealm, 2, pwd);
		memset(pwd, 0, strlen(pwd));
#ifdef MUTUAL_AUTH
		if (kerno == KSUCCESS)
			kerno = krb_mk_req(&ticket, "rcmd", savehost, lrealm, 33);
		if (kerno == KSUCCESS)
			kerno = krb_rd_req(&ticket, "rcmd", savehost,
				faddr, &authdata, "");
#endif /* MUTUAL_AUTH */
		printf("%s %d %s\n", name, kerno, krb_err_txt[kerno]);
		syslog(LOG_INFO, "%s %s", name, krb_err_txt[kerno]);
		dest_tkt();
	}
}
#else /* !KRB4_AUTH */
int main(argc, argv, envp)
	int argc;
	char **argv, **envp;
{
	exit(0);
}
#endif /* KRB4_AUTH */
