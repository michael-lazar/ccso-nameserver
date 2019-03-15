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
 * @(#)$Id: phquery.h,v 1.14 1995/06/28 20:39:14 p-pomes Exp $
 */

/*
 * Configuration parameters.
 *
 * Defining EXPAND_TO will print the list of expanded addresses on an added
 * X-PH-To: header line.  This will also expand names found in /usr/lib/aliases
 * lists.  Leaving it undefined will cause phquery to print only a version
 * line with the name of the host running phquery, e.g.,
 * X-Ph: V3.5@uxc.cso.uiuc.edu .
 */
/*#define		EXPAND_TO	*/ /* Print translated addresses */

/*
 * An address tested as a name is first run through as is.  If no matches
 * are found then any punctuation characters are converted one at a time
 * (leftmost first) to space characters and the lookup is repeated until
 * there are no more punctuation characters.  If WILDNAMES is #define'd,
 * a wildcard char '*' will be appended after each single character name,
 * e.g. p-pomes is tried as "p* pomes".  This has risks as follows:  assume
 * Duncan Lawrie sets his alias to "lawrie".  A query for d-lawrie will
 * fail as a alias lookup but succeed as a name lookup when written as
 * "d* lawrie".  This works until Joe Student sets his alias to "d-lawrie".
 * Whoops.  Still in a non-hostile environment, this function may be more
 * useful than dangerous.
 */
/*#define		WILDNAMES	*/ /* Append '*' to single char names */

/*
 * The types of nameserver queries to make.
 * N.B., Query() assumes that "name" is the last token in this list.
 */
char	*TryList[] =	{ "alias", "callsign", "name", 0 };

/*
 * Domain to append to ph aliases when creating Reply-To: fields.
 * Usually set in Makefile.
 */
#if !defined(DOMAIN) && defined(REPLYTO)
# define	DOMAIN		"uiuc.edu"
#endif	/* !DOMAIN && REPLYTO */

/*
 * Sendmail path.  Override if necessary.
 */
#if defined(BSD4_4)
#define		SENDMAIL	"/usr/sbin/sendmail"
#else /* !BSD4_4 */
#define		SENDMAIL	"/usr/lib/sendmail"
#endif /* BSD4_4 */
/* End of configurable parameters. */

/* some handy defines */
#define		CHNULL			('\0')
#define		CPNULL			((char *) NULL)
#define		FILE_NULL		((FILE *) NULL)

/* some handy compare operators */
#define		nequal(s1,s2,n)		(strncasecmp (s1, s2, n) == 0)
#define		equal(s1,s2)		(strcasecmp (s1, s2) == 0)

/* Bit flags to control printing of informative messages in ErrorReturn() */
#define		NO_MATCH_MSG		0x1
#define		MULTI_MSG		0x2
#define		ABSENT_MSG		0x4
#define		TOO_MANY_MSG		0x8
#define		HARD_MSG		0x10

struct	NewAddress {
	char	*original;	/* original address */
	char	*new;		/* translated address from qi */
	int	field;		/* lookup field used for match */
	int	code;
	QIR	*QIalt;
};
typedef	struct NewAddress NADD;
#define		NADD_NULL		((NADD *) NULL)
