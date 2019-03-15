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
 * @(#)$Id: messages.h,v 1.7 1993/11/11 15:58:42 paul Exp $
 */

/* Messages for ErrorReturn().  How simple, yet stupid, do we have to be? */

char	*NoMatchMsg[] = {
 " The message, \"No matches to nameserver query,\" is generated whenever",
 " the ph nameserver fails to locate either a ph alias or name field that",
 " matches the supplied name.  The usual causes are typographical errors or",
 " the use of nicknames.  Recommended action is to use the ph program to",
 " determine the correct ph alias for the individuals addressed.  If ph is",
 " not available, try sending to the most explicit form of the name, e.g.,",
 " if mike-fox fails, try michael-fox or michael-j-fox.",
 " ",
 NULL
};

char	*MultiMsg[] = {
 " The message, \"Multiple matches found for nameserver query,\" is generated",
 " whenever the ph nameserver finds multiple matches for the supplied name.",
 " The steering philosophy is that mail should be delivered only to the",
 " addressed individual.  Since the supplied information is insufficient",
 " to locate a specific individual, your message is being returned.",
 " To help you locate the correct individual, selected fields from the",
 " possible matches are included below.  The alias field is the only one",
 " guaranteed unique within a given ph community.",
 " ",
 NULL
};

char	*TooManyMsg[] = {
 " The message, \"Too many matches found to nameserver query,\" is generated",
 " whenever the supplied name or alias matched over twenty five ph nameserver",
 " entries.  In this case no information will be returned about possible",
 " matches.  Recommended action is to supply more specific names, e.g.,",
 " john-b-smith instead of john-smith, or use the per-person unique ph alias.",
 " You may have thought that you had used a ph alias and not a name.  This is",
 " an artifact of the address resolution process.  If the address fails as an",
 " alias, it is retried first as a callsign and then as a name.  While aliases",
 " are guaranteed unique, names can match multiple individuals depending on",
 " how common the name is.",
 " ",
 NULL
};

char	*AbsentMsg[] = {
 " The message, \"E-mail field not present in nameserver entry,\" is generated",
 " whenever the ph nameserver matched the supplied name or alias with an",
 " entry that lacked an email address field.  In this case no delivery can",
 " be made.  Recommended action is to contact the individual by alternate",
 " means via the information included below.  If the individual already has",
 " an email address, s/he should edit their ph entry to include it.  N.B.,",
 " postmaster will not have any information more current than this.",
 " ",
 NULL
};

char	*HardMsg[] = {
 " The message, \"Nameserver hard error; general,\" is generated whenever",
 " the ph nameserver encountered a permanent error in resolving an address.",
 " These errors are sometimes due to actual faults with the nameserver (rare),",
 " or with poorly formatted addresses.  The nameserver requires an address to",
 " have at least one token longer than three characters.  The exact error is",
 " reported below.",
 " ",
 NULL
};
