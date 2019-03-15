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
 * @(#)$Id: field.h,v 1.26 1994/12/17 14:11:26 p-pomes Exp $
 */

#ifndef FIELD_H
#define FIELD_H
#include "conf.h"
#define MAX_LINE 4096		/* maximum length of config file line */
#define MAX_HELP 240		/* maximum length of config file help */
#define MAX_LEN 4096		/* maximum length for any field */
#define MAX_IDVAL 100		/* highest field id number */
#define F_LEADER 15		/* width of field used to print field names */

/*
 * Field descriptor.  describes the attributes of a field
 */
struct fielddesc
{
	short	fdId;		/* id # of the field */
	short	fdMax;		/* maximum length of the field */
	int	fdIndexed;	/* do we index this field? */
	int	fdLookup;	/* do we let just anyone do lookups with this? */
	int	fdNoMeta;	/* meta characters aren't allowed for matches */
	int	fdPublic;	/* is field publicly viewable? */
	int	fdLocalPub;	/* is field publicly viewable off-site? */
	int	fdDefault;	/* print the field by default? */
	int	fdAlways;	/* print the always fields ? */
	int	fdAny;		/* the search field/property any */
	int	fdTurn;		/* can the user turn off display of this field? */
	int	fdChange;	/* is field changeable by the user? */
	int	fdSacred;	/* field requires great holiness of changer */
	int	fdEncrypt;	/* field requires encryption when it passes the net */
	int	fdNoPeople;	/* field may not be changed for "people"
				 * entries, but can for others */
	int	fdForcePub;	/* field is public, no matter what F_SUPPRESS is */
	char	*fdName;	/* name of the field */
	char	*fdHelp;	/* help for this field */
	char	*fdMerge;	/* merge instructions for this field */
};
typedef struct fielddesc FDESC;

/*
 * predefined fields.  these correspond to the ``don't touch'' fields
 * in fields.config
 */
#define F_ADDRESS	0
#define F_PHONE		1
#define F_EMAIL		2
#define F_NAME		3
#define F_TYPE		4
#define F_UNIVID	5
#define F_ALIAS		6
#define F_PASSWORD	7
#define F_PROXY		8
#define F_DEPARTMENT	9
#define F_TITLE		10
#define F_CURRICULUM	11
#define F_LOCKED	12
#define F_SOUND		13
#define F_MAILCODE	18
#define F_NICKNAME	23
#define F_HERO		30
#define F_HIGH		35
#define F_HOMEADDR	20
#define F_PERMADDR	21
#define F_HOMEPHONE	33
#define F_PAPER		38
#define F_SUPPRESS	43

/*
 * merge flags
 */
#define OLD_FIELD	'O'
#define STUDENT_FIELD	'S'
#define STAFF_FIELD	'F'
#define CONDITIONAL	'C'

static int F_TempValue;

#define FIELDVALUE(dir,select)	(strchr(dir[select],':')+1)
#define FINDVALUE(dir,f)	\
    (((F_TempValue=FindField(dir,f))>=0)?FIELDVALUE(dir,F_TempValue):"")

extern FDESC **FieldDescriptors;

#endif
