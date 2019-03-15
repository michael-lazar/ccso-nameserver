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
 * @(#)$Id: qicode.h,v 1.5 1995/06/28 20:37:26 p-pomes Exp $
 */

#ifndef _QICODE_H
#define _QICODE_H

struct QiReplyCode QiCodes[] = {
	LR_PROGRESS,	"Nameserver search in progress",
	LR_ECHO,	"Echoing nameserver cmd",
	LR_NUMRET,	"How many entries are being returned",
	LR_NONAME,	"No hostname found for IP address",
	LR_OK,		"Success",
	LR_RONLY,	"Nameserver database ready in read only mode",
	LR_MORE,	"More info needed to process nameserver query",
	LR_LOGIN,	"Encrypt this string",
	LR_XLOGIN,	"Prompt for password with enclosed challenge",
	LR_TEMP,	"Temporary nameserver error",
	LR_INTERNAL,	"Nameserver database error, possibly temporary",
	LR_LOCK,	"Nameserver lock not obtained within timeout period",
	LR_COULDA_BEEN,	"Login would have been ok but db read only",
	LR_DOWN,	"Nameserver database unavailable; try again later",
	LR_ERROR,	"Nameserver hard error; general",
	LR_NOMATCH,	"No matches to nameserver query",
	LR_TOOMANY,	"Too many matches found to nameserver query",
	LR_AINFO,	"May not see that nameserver field",
	LR_ASEARCH,	"May not search on that nameserver field",
	LR_ACHANGE,	"May not change that nameserver field",
	LR_NOTLOG,	"Must be logged in to nameserver",
	LR_FIELD,	"Unknown nameserver field",
	LR_ABSENT,	"E-mail field not present in nameserver entry",
	LR_ALIAS,	"Requested nameserver alias is already in use",
	LR_AENTRY,	"May not change nameserver entry",
	LR_ADD,		"May not add nameserver entries",
	LR_VALUE,	"Illegal value",
	LR_OPTION,	"Unknown nameserver option",
	LR_UNKNOWN,	"Unknown nameserver command",
	LR_NOKEY,	"No indexed field found in nameserver query",
	LR_AUTH,	"No authorization for nameserver request",
	LR_READONLY,	"Nameserver operation failed; database is read-only",
	LR_LIMIT,	"Too many nameserver entries selected for change",
	LR_HISTORY,	"History substitution failed (obsolete)",
	LR_XCPU,	"Too much cpu used",
	LR_ADDONLY,	"Addonly option set and change command applied to a field with data",
	LR_ISCRYPT,	"Attempt to view encrypted field",
	LR_NOANSWER,	"\"answer\" was expected but not gotten",
	LR_BADHELP,	"Help topics cannot contain slashes",
	LR_NOEMAIL,	"Email authentication failed",
	LR_NOADDR,	"Host name address not found in DNS",
	LR_MISMATCH,	"Host = gethostbyaddr(foo); foo != gethostbyname(host)",
	LR_KDB5,	"General kerberos v5 database error",
	LR_NOAUTH,	"Selected authentication method not available",
	LR_OFFCAMPUS,	"Remote queries not allowed",
	LR_NOCMD,	"No such command",
	LR_SYNTAX,	"Syntax error",
	LR_AMBIGUOUS,	"Multiple matches found for nameserver query",
	-1,		(char *)NULL
};
#endif /* !_QICODE_H */
