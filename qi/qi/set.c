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
static char  RcsId[] = "@(#)$Id: set.c,v 1.15 1994/03/12 00:24:45 paul Exp $";
#endif

#include "protos.h"

OPTION	OptionList[] =
{
	"echo", 0,	/* echo commands before execution */
	"limit", 0,	/* limit the number of entries a command may affect */
	"verbose", 0,	/* I don't remember */
	"addonly", 0,	/* do not change fields that contain information */
	"nolog", 0,	/* do not issue syslog information */
	0, 0
};

static OPTION *FindOption __P((char *));
static void SetOption __P((OPTION *, char *));

/*
 * do a set command
 */
void 
DoSet(argp)
	ARG *argp;
{
	OPTION	*option;
	int	successes = 0;

	argp = argp->aNext;
	if (!argp)
	{
		/* list the current options */
		for (option = OptionList; option->opName; option++)
			DoReply(-LR_OK, "%s:%s", option->opName,
			      option->opValue ? (*option->opValue ?
				       option->opValue : "on") : "off");
		DoReply(LR_OK, "Done.");
	} else
	{
		for (; argp; argp = argp->aNext)
		{
			if (option = FindOption(argp->aFirst))
			{
				successes++;
				if (argp->aType & VALUE2)
					SetOption(option, argp->aSecond);
				else if (argp->aType & EQUAL)
					SetOption(option, "off");
				else
					SetOption(option, "on");
			} else
				DoReply(-LR_OPTION, "%s:unknown option", argp->aFirst);
		}
		if (successes)
			DoReply(LR_OK, "Done.");
		else
			DoReply(LR_OPTION, "No option recognized.");
	}
}

/*
 * find an option in the list
 */
static OPTION	*
FindOption(name)
	char *name;
{
	OPTION	*option;

	for (option = OptionList; option->opName; option++)
		if (!strcmp(option->opName, name))
			return (option);

	return (NULL);
}

/*
 * set an option to a value
 */
static void 
SetOption(option, value)
	OPTION *option;
	char *value;
{
	if (option->opValue)
		free(option->opValue);

	if (!value || !strcmp(value, "off") || !(*value))
	{
		option->opValue = NULL;
		if (OP_VALUE(VERBOSE_OP))
			DoReply(LR_PROGRESS, "%s=off", option->opName);
	} else if (!strcmp(value, "on"))
	{
		option->opValue = strdup("");
		if (OP_VALUE(VERBOSE_OP))
			DoReply(LR_PROGRESS, "%s=on", option->opName);
	} else
	{
		option->opValue = strdup(value);
		if (OP_VALUE(VERBOSE_OP))
			DoReply(LR_PROGRESS, "%s=%s", option->opName, value);
	}
}

/*
 * initialize the options
 */
void 
InitializeOptions()
{
	SetOption(&OptionList[LIMIT_OP], "2");
}
