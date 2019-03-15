%start C
%start CE
%start Q
%start I
%start F
%start A
%{
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
static char  RcsId[] = "@(#)$Id: language.l,v 1.34 1995/06/27 16:07:43 p-pomes Exp $";
#endif

#include "protos.h"

int	cmd;
char	*quote;

int	OldPh = 0;		/* is user using an old version of ph? */

#define OLD		OldPh=1
#define INCREMENT	80
#define UNREASONABLE	8192
extern FILE *Input;
extern FILE *Output;

#undef input
#undef unput
#undef output

static int TC;


/*
 * input/output with backup and a record of the last command
 */
char	CommandText[UNREASONABLE];
char	*CommandSpot = CommandText;

static void CollectQuote __P((char *, int));
static int GetAC __P((FILE *));

#define RESET	CommandSpot = CommandText
#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
{ \
int c = GetAC(Input); \
result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
}
#else /* !FLEX_SCANNER */
#define input()		(((TC=GetAC(Input))==EOF)?0:(*CommandSpot++ = TC))
#endif /* FLEX_SCANNER */
#define unput(c)	CommandSpot--,ungetc(c,Input)
#define output(c)	putc(c,Output)
%}
%%
	BEGIN I;				/* start state */
<I>query	{AddValue((char *)yytext,COMMAND); cmd = C_QUERY; BEGIN C;}
<I>ph		{AddValue((char *)yytext,COMMAND); cmd = C_QUERY; BEGIN C;}
<I>change	{AddValue((char *)yytext,COMMAND); cmd = C_CHANGE; BEGIN C;}
<I>login	{AddValue((char *)yytext,COMMAND); cmd = C_LOGIN; BEGIN C;}
<I>xlogin	{AddValue((char *)yytext,COMMAND); cmd = C_XLOGIN; BEGIN A;}
<I>klogin	{AddValue((char *)yytext,COMMAND); cmd = C_XLOGIN; BEGIN A;}
<I>answer	{AddValue((char *)yytext,COMMAND); cmd = C_ANSWER; BEGIN A;}
<I>email	{AddValue((char *)yytext,COMMAND); cmd = C_EMAIL; BEGIN C;}
<I>clear	{AddValue((char *)yytext,COMMAND); cmd = C_CLEAR; BEGIN C;}
<I>logout	{AddValue((char *)yytext,COMMAND); cmd = C_LOGOUT; BEGIN C;}
<I>fields	{AddValue((char *)yytext,COMMAND); cmd = C_FIELDS; BEGIN C;}
<I>add		{AddValue((char *)yytext,COMMAND); cmd = C_ADD; BEGIN C;}
<I>delete	{AddValue((char *)yytext,COMMAND); cmd = C_DELETE; BEGIN C;}
<I>set		{AddValue((char *)yytext,COMMAND); cmd = C_SET; BEGIN C;}
<I>quit		{AddValue((char *)yytext,COMMAND); cmd = C_QUIT; BEGIN C;}
<I>stop		{AddValue((char *)yytext,COMMAND); cmd = C_QUIT; BEGIN C;}
<I>exit		{AddValue((char *)yytext,COMMAND); cmd = C_QUIT; BEGIN C;}
<I>status	{AddValue((char *)yytext,COMMAND); cmd = C_STATUS; BEGIN C;}
<I>help		{AddValue((char *)yytext,COMMAND); cmd = C_HELP; BEGIN C;}
<I>id		{AddValue((char *)yytext,COMMAND); cmd = C_ID; BEGIN C;}
<I>siteinfo	{AddValue((char *)yytext,COMMAND); cmd = C_INFO; BEGIN C;}

<I>interactive\ncsnet\nversion\n\t3.0\nwhois\n	{OLD;RESET;}

<I>exact\n	{AddValue("query",COMMAND); CommandSpot--; cmd = C_QUERY; BEGIN C;}
<I>[ \t]+	;				/* ignore whitespace */
<I>[^ \377\t\n]+	{Unknown((char *)yytext);BEGIN F;}	/* extraneous junk */
<I,F>\n		return(-1);			/* no command */
<I>'#'		{BEGIN F;}

<F>.		;				/* just toss it all */

<A>[ \t]	;
<A>[^ \t\n]+	{AddValue((char *)yytext,VALUE);}
<A>\n		{DoCommand(cmd); RESET; return(cmd);}
<C>return	{AddValue("return",RETURN);}	/* make , return, and */
<C>force	{AddValue("force",RETURN);}	/* force all serve */
<C>make		{AddValue("make",RETURN);}	/* the same function */
<C,CE>\"	{CollectQuote(0,0); BEGIN Q;}	/* begin a quoted value */
<C,CE>[ \t]	{BEGIN C;}			/* ignore whitespace */
<C,CE>\\\n	{BEGIN C;}			/* ignore escaped newlines */
<C>[^ \"\n\t=~]+	{AddValue((char *)yytext,VALUE);}		/* a single value */
<CE>[^ \"\n\t]+	{AddValue((char *)yytext,VALUE); BEGIN C;}	/* value after = */
<C>=		{AddValue("=",EQUAL);BEGIN CE;}	/* the equal sign */
<C>~		{AddValue("~",EQUAL|TILD_E);BEGIN CE;}	/* the tilde */
<C,CE>\n	{DoCommand(cmd); RESET; return(cmd);}/* execute */

<Q>[^\\\"]+	CollectQuote((char *)yytext,yyleng);	/* copy any char but quote */
<Q>\\\"		CollectQuote("\"",1);		/* copy an escaped quote */
<Q>\\\\		CollectQuote("\\",1);		/* copy an escaped backslash */
<Q>\\n		CollectQuote("\n",1);		/* C newline */
<Q>\\t		CollectQuote("\t",1);		/* C tab */
<Q>\"		{AddValue((quote)?quote:"",VALUE); BEGIN C;}	/* enter the quoted value */

\377.		;	/* ignore telnet commands; refuse telnet options */
\377\373.	{fprintf(Output,"\377\376%c",(unsigned char) yytext[2]);}
\377\375.	{fprintf(Output,"\377\374%c",(unsigned char) yytext[2]);}
\\		;				/* ignore other backslashes */

%%
static void
CollectQuote(text, len)
	char	*text;
	int	len;
{
	static char *endt;
	static int size = 0;
	static int cnt = 0;

	if (text == NULL)
	{
		if (quote)
		{
			free(quote);
			quote = 0;
			size = 0;
			cnt = 0;
		}
	} else
	{
		if (cnt + len >= size && cnt + len < UNREASONABLE)
		{
			if (!quote)
			{
				endt = quote = malloc(len + INCREMENT);
				size = len + INCREMENT;
				cnt = 0;
			} else
			{
				size = cnt + len + INCREMENT;
				quote = realloc(quote, size);
				endt = quote + cnt;
			}
		}
		strcpy(endt, text);
		endt += len;
		cnt += len;
	}
}

/*
 * get a non-null, non-return character
 */
static int 
GetAC(fp)
	FILE *fp;
{
	int	c;

	do
	{
		c = getc(fp);
	}
	while (c == '\r' || c == '\0');

	return (c);
}
