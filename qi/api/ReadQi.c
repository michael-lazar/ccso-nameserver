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
static char  RcsId[] = "@(#)$Id: ReadQi.c,v 1.5 1995/06/07 19:15:22 p-pomes Exp $";
#endif

/*
**  ReadQi -- Read and store response from Qi server
**
**	A Qi response has one of the following structures:
**
**	<-><code>:<subcode><ws><field name>:<string>
**	5XX:Error message
**	200:Ok.
**
**	The leading '-' marks a continuation line.  The last line of a
**	response will not have the '-'.
**
**	<code> is the response code.  Response codes are listed in qiapi.h
**	and closely follow the conventions of SMTP (RFC-821):
**
**	1XX - status
**	2XX - information
**	3XX - additional information or action needed
**	4XX - temporary errors
**	5XX - permanent errors
**	6XX - phquery specific codes
**
**	<subcode> links multiple fields (e.g., email and pager) to a single
**	individual.  If a name query results in a multiple match, subcode
**	increments by 1 for each person but has the same value for all response
**	lines for that individual.
**
**	<ws> is sufficient white space to right adjust <field name>: to the
**	same position on each line.
**
**	<field name> is one of the field type in phquery.h (e.g., department,
**	mailcode, etc).
**
**	<string> is either the value for <field name>, if <code> == 200 (LR_OK),
**	or an error message it <code> is anything else.
**
**	Parameters:
**		FromQi - stream descriptor for reading qi response
**		count - integer pointer for number of matches found.
**
**	Returns:
**		A pointer to a malloc()'ed block of QI_response structs that
**		is terminated with QI_response.code > 0.
**		Errors return a NULL pointer with the details reported via
**		syslog(3).
**
**	Side Effects:
**		Creates a block of data that must be later free()'d.
**		Advances FromQi.
*/

#include <syslog.h>
#ifdef __STDC__
# include <string.h>
#else /* !__STDC__ */
# include <strings.h>
#endif /* __STDC__ */
#include "qiapi.h"

#define		MAXSTR		4608
#define		CHNULL		('\0')
#define		CPNULL		((char *) NULL)

QIR *
ReadQi (FromQi, count)
    FILE *FromQi;
    int *count;
{
    int		i, code;
    int		loopcnt = 1;
    char	*tp;
    unsigned	qsize = sizeof (QIR);
    char	fstring[MAXSTR];	/* field string */
    char	message[MAXSTR];	/* field value */
    char	Temp[MAXSTR];
    int		CurField = -1;
    register QIR *Base, *RepChain;

    if (count != NULL)
	*count = 0;
    if ((Base = RepChain = (QIR *) malloc(qsize)) == NULL) {
	syslog(LOG_ERR, "ReadQi: malloc(%u): %m", qsize);
	return(QIR_NULL);
    }
    RepChain->field = -1;
    Base->message = CPNULL;
    do {
	*fstring = *message = CHNULL;
	if (fgets(Temp, MAXSTR-1, FromQi) == CPNULL) {
	    syslog(LOG_ERR, "ReadQi: premature EOF");
	    return(QIR_NULL);
	}
	if (QiDebug > 1)
	    syslog(LOG_DEBUG, "ReadQi read =%s=", Temp);
	code = atoi(Temp);

	/* Loop immediately if we read a preliminary response (99<x<200) */
	if (code > 99 && code < 200)
	    continue;

	/* Positive response codes are formatted "<code>:<message>" */
	if (code > 0 || abs(code) != 200) {
	    RepChain->subcode = -1;
	    if (sscanf(Temp, "%d:%[^\n]", &RepChain->code, message)
	      != 2 || *message == CHNULL) {
		syslog(LOG_ERR, "ReadQi: short #1 sscanf: %m");
		return(QIR_NULL);
	    }
	}

	/* Otherwise they are the 4 field type */
	else if ((i = sscanf(Temp, "%d:%d:%[^:]: %[^\n]",
	  &RepChain->code, &RepChain->subcode, fstring, message))
	  != 4 || *fstring == CHNULL || *message == CHNULL) {
	    /*
	     * The short sscanf() read may be due to a embedded
	     * newline.  If so, continue for a bit to fill out the
	     * code field before reading another line.
	     */
	    if (!(i == 3 && *message == CHNULL)) {
		syslog(LOG_ERR, "ReadQi: short #2 sscanf, expected 4 got %d",i);
		return(QIR_NULL);
	    }
	}
	if (count != NULL && RepChain->subcode > 0)
	    *count = RepChain->subcode;

	/*
	 * Some fields go over multiple response lines.  In that case
	 * the field is all blanks.  Copy the response field from the
	 * previous response if not already set.
	 */
	for (tp = fstring; tp <= fstring + (MAXSTR-1) && *tp == ' '; tp++)
	    ;
	if (*tp)
	    CurField = RepChain->field = FieldValue(tp);
	else
	    RepChain->field = CurField;

	/* Now get a new line if message was empty. */
	if (*message == CHNULL)
	    continue;
	RepChain->message = strdup(message);
	if (RepChain->code > 0)
	    break;
	qsize += sizeof (QIR);
	if ((Base = (QIR *) realloc((char *) Base, qsize)) == NULL) {
	    syslog(LOG_ERR, "ReadQi: realloc(%u): %m", qsize);
	    return(QIR_NULL);
	}
	RepChain = Base + loopcnt;
	RepChain->field = -1;
	loopcnt++;
    } while (loopcnt);
    if (QiDebug)
	for (RepChain = Base; RepChain->code < 0; RepChain++)
	    syslog(LOG_DEBUG, "code %d, subcode %d, field %d, message: %s",
	      RepChain->code, RepChain->subcode,
	      RepChain->field, RepChain->message);
    return(Base);
}
