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
 *
 * @(#)$Id: commands.h,v 1.28 1995/06/28 20:34:44 p-pomes Exp $
 */

#ifndef COMMANDS_H
#define COMMANDS_H
#include <stdio.h>
#include "conf.h"
#include "log.h"
#include "field.h"
#include "qi.h"
#include "qiapi.h"

/*
 * Commands.  Changes made here should also be made in language.l
 */
#define C_QUERY		1
#define C_CHANGE	2
#define C_LOGIN		3
#define C_ANSWER	4
#define C_LOGOUT	5
#define C_FIELDS	6
#define C_ADD		7
#define C_DELETE	8
#define C_SET		9
#define C_QUIT		10
#define C_STATUS	11
#define C_ID		12
#define C_HELP		13
#define C_CLEAR		14
#define C_INFO		15
#define C_EMAIL		16
#define C_XLOGIN	17	/* always defined, even if not enabled */

extern QDIR User;		/* in commands.c */
extern char *UserAlias;		/* in commands.c */
extern int UserEnt;		/* in commands.c */
extern FILE *Input;		/* mqi.c */
extern FILE *Output;		/* mqi.c */
extern QDIR HeroDir;		/* commands.c */
extern char *Hero;		/* commands.c */
extern int AmHero;		/* commands.c */
extern int AmOpr;		/* commands.c */

/*
 * server states
 */
extern int State;		/* in commands.c */

# define S_IDLE		0	/* nothing in the works */
# define S_E_PENDING	1	/* waiting for login response */

/*
 * Value types
 */

/* the following types may be or'd together */
#define VALUE	1		/* single value */
#define EQUAL	2		/* an equals sign */
#define VALUE2	4		/* a value AFTER an equals sign */

/* other types */
#define RETURN	8		/* a return token */
#define COMMAND 16		/* a command name */
#define TILD_E 32		/* a tilde */

/* Flags */
#define A_NO_RECURSE	1	/* don't perform tail recursion on names */

/*
 * Argument structure
 */
struct argument
{
	int	aType;
	int	aKey;
	int	aFlag;
	int	aRating;
	char	*aFirst;
	char	*aSecond;
	FDESC	*aFD;
	struct argument	*aNext;
};
typedef struct argument ARG;
extern void (*CommandTable[]) __P((ARG *));	/* in commands.c */

/*
 * delimiters for word breakup
 */
#define IDX_DELIM " \t\n,;:"

/*
 * flags
 */
extern int ReadOnly;		/* mqi.c */

/*
 * just in case...
 */
#define MAXSTR 256

#endif
