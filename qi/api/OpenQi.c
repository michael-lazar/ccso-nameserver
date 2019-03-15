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
static char  RcsId[] = "@(#)$Id: OpenQi.c,v 1.8 1995/06/10 04:05:31 p-pomes Exp $";
#endif

/*
**  OpenQi -- Connect to the Qi server
**
**	Open socket connections to the Qi server.
**
**	Parameters:
**		Host - Name of server to contact
**		To - pointer to stream descriptor
**		From - pointer to stream descriptor
**
**	Returns:
**		 0 if successful
**		-1 if any error
**
**	Side Effects:
**		Initializes global QiFields by calling GetFields()
**		Reports error via syslog
*/

#include "qiapi.h"
#include <netdb.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef NSSERVICE
# define	NSSERVICE	"csnet-ns"
#endif

#ifdef NO_MEMMOVE
# define	memmove(a,b,c)	bcopy(b,a,c)
#endif

int
OpenQi (Host, To, From)
char	*Host;
FILE 	**To, **From;
{
    int sock;				/* our socket */
    struct sockaddr_in Qi;		/* the address of the nameserver */
    struct servent *Ns;			/* nameserver service entry */
    struct hostent *hp;			/* host entry for nameserver */

    if (QiDebug)
	fprintf(stderr, "contacting qi on %s\r\n", Host);

    /* Locate the proper port */
    if (Ns = getservbyname (NSSERVICE, "tcp"))
	Qi.sin_port = Ns->s_port;
    else {
	if (QiDebug)
	    fprintf(stderr, "OpenQi: service \"%s\" unknown - using 105\r\n",
	      NSSERVICE);
	syslog(LOG_ERR, "OpenQi: service \"%s\" unknown - using 105",
	    NSSERVICE);
	Qi.sin_port = htons((unsigned short) 105);
    }
    Qi.sin_family = AF_INET;

    /* Get a socket for the Qi connection */
    if (geteuid() == 0) {	/* if running as root, get a reserved port */
      int localport = IPPORT_RESERVED - 1;
      if ((sock = rresvport(&localport)) < 0) {
	syslog(LOG_ERR, "OpenQI: rresvport(): %m");
	return(-1);
      }
    } else if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	if (QiDebug)
	    fprintf(stderr, "OpenQi: socket(): %s\r\n", sys_errlist[errno]);
	syslog(LOG_ERR, "OpenQi: socket(): %m");
	return(-1);
    }

    /* Locate the proper host */
    if (hp = gethostbyname (Host))
	memmove ((char *) &Qi.sin_addr.s_addr, hp->h_addr, 4);
    else {
	if (QiDebug)
	    fprintf(stderr, "OpenQi: gethostbyname(%s): %s\r\n",
	      Host, sys_errlist[errno]);
	syslog(LOG_ERR, "OpenQi: gethostbyname(%s): %m", Host);
	return(-1);
    }

    /* Connect to the nameserver */
    if (connect (sock, (struct sockaddr *) &Qi, sizeof (Qi)) < 0) {
	if (QiDebug)
	    fprintf(stderr, "OpenQi: connect(%s): %s\r\n",
	      Host, sys_errlist[errno]);
	syslog(LOG_ERR, "OpenQi: connect(%s): %m", Host);
	(void) close(sock);
	return (-1);
    }

    /* Create stream descriptors */
    if ((*To = fdopen(sock, "w")) == (FILE *) NULL) {
	syslog(LOG_ERR, "OpenQi: fdopen(s, \"w\"): %m");
	return(-1);
    }
    if ((*From = fdopen(sock, "r")) == (FILE *) NULL) {
	syslog(LOG_ERR, "OpenQi: fdopen(s, \"r\"): %m");
	return(-1);
    }

    /* Initialize QiFields global */
    if (QiFields == QIF_NULL && (QiFields = GetFields(*To, *From)) == QIF_NULL)
	return(-1);

    /* Connection ok */
    return (0);
}
