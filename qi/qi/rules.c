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
static char  RcsId[] = "@(#)$Id: rules.c,v 1.11 1994/03/12 00:24:45 paul Exp $";
#endif

#include "protos.h"

/*
**	English to Phoneme rules.
**
**	Derived from: 
**
**	     AUTOMATIC TRANSLATION OF ENGLISH TEXT TO PHONETICS
**	            BY MEANS OF LETTER-TO-SOUND RULES
**
**			NRL Report 7948
**
**		      January 21st, 1976
**	    Naval Research Laboratory, Washington, D.C.
**
**
**	Published by the National Technical Information Service as
**	document "AD/A021 929".
**
**
**
**	The Phoneme codes:
**
**		IY	bEEt		IH	bIt
**		EY	gAte		EH	gEt
**		AE	fAt		AA	fAther
**		AO	lAWn		OW	lOne
**		UH	fUll		UW	fOOl
**		ER	mURdER		AX	About
**		AH	bUt		AY	hIde
**		AW	hOW		OY	tOY
**	
**		p	Pack		b	Back
**		t	Time		d	Dime
**		k	Coat		g	Goat
**		f	Fault		v	Vault
**		TH	eTHer		DH	eiTHer
**		s	Sue		z	Zoo
**		SH	leaSH		SH	leiSure
**		HH	How		m	suM
**		n	suN		NG	suNG
**		l	Laugh		w	Wear
**		y	Young		r	Rate
**		CH	CHar		j	Jar
**		WH	WHere
**
**
**	Rules are made up of four parts:
**	
**		The left context.
**		The text to match.
**		The right context.
**		The phonemes to substitute for the matched text.
**
**	Procedure:
**
**		Seperate each block of letters (apostrophes included) 
**		and add a space on each side.  For each unmatched 
**		letter in the word, look through the rules where the 
**		text to match starts with the letter in the word.  If 
**		the text to match is found and the right and left 
**		context patterns also match, output the phonemes for 
**		that rule and skip to the next unmatched letter.
**
**
**	Special Context Symbols:
**
**		#	One or more vowels
**		:	Zero or more consonants
**		^	One consonant.
**		.	One of B, D, V, G, J, L, M, N, R, W or Z (voiced 
**			consonants)
**		%	One of ER, E, ES, ED, ING, ELY (a suffix)
**			(Found in right context only)
**		+	One of E, I or Y (a "front" vowel)
**
*/

/* context definitions */
static char anything[] = "";	/* no context requirement */
static char nothing[] = " ";	/* context is beginning or end of word */

/* phoneme definitions */
static char aPause[] = " ";	/* short silence */
static char silent[] = "";	/* no phonemes */

#define left_part     0
#define match_part    1
#define right_part    2
#define out_part    3

typedef char *Rule[4];		/* rule is an array of 4 character pointers */

/* 0 = punctuation */
/*
 *	left_part	match_part	right_part	out_part
 */
static Rule punct_rules[] =
{
	{anything,	" ",		anything,	aPause},
	{anything,	"-",		anything,	silent},
	{".",		"'s",		anything,	"z"},
	{"#:.e",	"'s",		anything,	"z"},
	{"#",		"'s",		anything,	"z"},
	{anything,	"'",		anything,	silent},
	{anything,	",",		anything,	aPause},
	{anything,	".",		anything,	aPause},
	{anything,	"?",		anything,	aPause},
	{anything,	"!",		anything,	aPause},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule a_rules[] =
{
	{anything,	"a",		nothing,	"aa"},
	{nothing,	"are",		nothing,	"aar"},
	{nothing,	"ar",		"o",		"aar"},
	{anything,	"ar",		"#",		"ehr"},
	{"^",		"as",		"#",		"ays"},
	{anything,	"a",		"wa",		"aa"},
	{anything,	"aw",		anything,	"ao"},
	{" :",		"any",		anything,	"ehnay"},
	{anything,	"a",		"^+#",		"ay"},
	{"#:",		"ally",		anything,	"aalay"},
	{nothing,	"al",		"#",		"aal"},
	{anything,	"again",	anything,	"aagehn"},
	{"#:",		"ag",		"e",		"ihj"},
	{anything,	"a",		"^+:#",		"ae"},
	{" :",		"a",		"^+ ",		"ay"},
	{anything,	"a",		"^%",		"ay"},
	{nothing,	"arr",		anything,	"aar"},
	{anything,	"arr",		anything,	"aer"},
	{" :",		"ar",		nothing,	"aar"},
	{anything,	"ar",		nothing,	"er"},
	{anything,	"ar",		anything,	"aar"},
	{anything,	"air",		anything,	"ehr"},
	{anything,	"ai",		anything,	"ay"},
	{anything,	"ay",		anything,	"ay"},
	{anything,	"au",		anything,	"ao"},
	{"#:",		"al",		nothing,	"aal"},
	{"#:",		"als",		nothing,	"aalz"},
	{anything,	"alk",		anything,	"aok"},
	{anything,	"al",		"^",		"aol"},
	{" :",		"able",		anything,	"aybaal"},
	{anything,	"able",		anything,	"aabaal"},
	{anything,	"ang",		"+",		"aynj"},
	{anything,	"a",		anything,	"ae"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule b_rules[] =
{
	{nothing,	"be",		"^#",		"bih"},
	{anything,	"being",	anything,	"bayihng"},
	{nothing,	"both",		nothing,	"bowth"},
	{nothing,	"bus",		"#",		"bihz"},
	{anything,	"buil",		anything,	"bihl"},
	{anything,	"b",		anything,	"b"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule c_rules[] =
{
	{nothing,	"ch",		"^",		"k"},
	{"^e",		"ch",		anything,	"k"},
	{anything,	"ch",		anything,	"ch"},
	{" s",		"ci",		"#",		"say"},
	{anything,	"ci",		"a",		"sh"},
	{anything,	"ci",		"o",		"sh"},
	{anything,	"ci",		"en",		"sh"},
	{anything,	"c",		"+",		"s"},
	{anything,	"ck",		anything,	"k"},
	{anything,	"com",		"%",		"kaam"},
	{anything,	"c",		anything,	"k"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule d_rules[] =
{
	{"#:",		"ded",		nothing,	"dihd"},
	{".e",		"d",		nothing,	"d"},
	{"#:^e",	"d",		nothing,	"t"},
	{nothing,	"de",		"^#",		"dih"},
	{nothing,	"do",		nothing,	"duw"},
	{nothing,	"does",		anything,	"daaz"},
	{nothing,	"doing",	anything,	"duwihng"},
	{nothing,	"dow",		anything,	"daw"},
	{anything,	"du",		"a",		"juw"},
	{anything,	"d",		anything,	"d"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule e_rules[] =
{
	{"#:",		"e",		nothing,	silent},
	{"':^",		"e",		nothing,	silent},
	{" :",		"e",		nothing,	"ay"},
	{"#",		"ed",		nothing,	"d"},
	{"#:",		"e",		"d ",		silent},
	{anything,	"ev",		"er",		"ehv"},
	{anything,	"e",		"^%",		"ay"},
	{anything,	"eri",		"#",		"ayray"},
	{anything,	"eri",		anything,	"ehrih"},
	{"#:",		"er",		"#",		"er"},
	{anything,	"er",		"#",		"ehr"},
	{anything,	"er",		anything,	"er"},
	{nothing,	"even",		anything,	"ayvehn"},
	{nothing,	"ephen",	anything,	"ayvehn"},
	{"#:",		"e",		"w",		silent},
	{"t",		"ew",		anything,	"uw"},
	{"s",		"ew",		anything,	"uw"},
	{"r",		"ew",		anything,	"uw"},
	{"d",		"ew",		anything,	"uw"},
	{"l",		"ew",		anything,	"uw"},
	{"z",		"ew",		anything,	"uw"},
	{"n",		"ew",		anything,	"uw"},
	{"j",		"ew",		anything,	"uw"},
	{"th",		"ew",		anything,	"uw"},
	{"ch",		"ew",		anything,	"uw"},
	{"sh",		"ew",		anything,	"uw"},
	{anything,	"ew",		anything,	"yuw"},
	{anything,	"e",		"o",		"ay"},
	{"#:s",		"es",		nothing,	"ihz"},
	{"#:c",		"es",		nothing,	"ihz"},
	{"#:g",		"es",		nothing,	"ihz"},
	{"#:z",		"es",		nothing,	"ihz"},
	{"#:x",		"es",		nothing,	"ihz"},
	{"#:j",		"es",		nothing,	"ihz"},
	{"#:ch",	"es",		nothing,	"ihz"},
	{"#:sh",	"es",		nothing,	"ihz"},
	{"#:",		"e",		"s ",		silent},
	{"#:",		"ely",		nothing,	"lay"},
	{"#:",		"ement",	anything,	"mehnt"},
	{anything,	"eful",		anything,	"fuhl"},
	{anything,	"ee",		anything,	"ay"},
	{anything,	"earn",		anything,	"ern"},
	{nothing,	"ear",		"^",		"er"},
	{anything,	"ead",		anything,	"ehd"},
	{"#:",		"ea",		nothing,	"ayaa"},
	{anything,	"ea",		"su",		"eh"},
	{anything,	"ea",		anything,	"ay"},
	{anything,	"eigh",		anything,	"ay"},
	{anything,	"ei",		anything,	"ay"},
	{nothing,	"eye",		anything,	"ay"},
	{anything,	"ey",		anything,	"iy"},
	{anything,	"eu",		anything,	"yuw"},
	{anything,	"e",		anything,	"eh"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule f_rules[] =
{
	{anything,	"ful",		anything,	"fuhl"},
	{anything,	"f",		anything,	"f"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule g_rules[] =
{
	{anything,	"giv",		anything,	"gihv"},
	{nothing,	"g",		"i^",		"g"},
	{anything,	"ge",		"t",		"geh"},
	{"su",		"gges",		anything,	"gjehs"},
	{anything,	"gg",		anything,	"g"},
	{" b#",		"g",		anything,	"g"},
	{anything,	"g",		"+",		"j"},
	{anything,	"great",	anything,	"grayt"},
	{"#",		"gh",		anything,	silent},
	{anything,	"g",		anything,	"g"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule h_rules[] =
{
	{nothing,	"hav",		anything,	"haev"},
	{nothing,	"here",		anything,	"hayr"},
	{nothing,	"hour",		anything,	"awer"},
	{anything,	"how",		anything,	"haw"},
	{anything,	"h",		"#",		"h"},
	{anything,	"h",		anything,	silent},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule i_rules[] =
{
	{nothing,	"in",		anything,	"ihn"},
	{nothing,	"i",		nothing,	"ay"},
	{anything,	"in",		"d",		"ayn"},
	{anything,	"ier",		"^",		"er"},
	{anything,	"ier",		anything,	"ayer"},
	{"#:r",		"ied",		anything,	"ayd"},
	{anything,	"ied",		nothing,	"ayd"},
	{anything,	"ien",		anything,	"ayehn"},
	{anything,	"ie",		"t",		"ayeh"},
	{" :",		"i",		"%",		"ay"},
	{anything,	"i",		"%",		"ay"},
	{anything,	"ie",		anything,	"ay"},
	{anything,	"i",		"^+:#",		"ih"},
	{anything,	"ir",		"#",		"ayr"},
	{anything,	"iz",		"%",		"ayz"},
	{anything,	"is",		"%",		"ayz"},
	{anything,	"i",		"d%",		"ay"},
	{"+^",		"i",		"^+",		"ih"},
	{anything,	"i",		"t%",		"ay"},
	{"#:^",		"i",		"^+",		"ih"},
	{anything,	"i",		"^+",		"ay"},
	{anything,	"ir",		anything,	"er"},
	{anything,	"igh",		anything,	"ay"},
	{anything,	"ild",		anything,	"ayld"},
	{anything,	"ign",		nothing,	"ayn"},
	{anything,	"ign",		"^",		"ayn"},
	{anything,	"ign",		"%",		"ayn"},
	{anything,	"ique",		anything,	"ayk"},
	{anything,	"i",		anything,	"ih"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule j_rules[] =
{
	{anything,	"j",		anything,	"j"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule k_rules[] =
{
	{nothing,	"k",		"n",		silent},
	{anything,	"k",		anything,	"k"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule l_rules[] =
{
	{anything,	"lo",		"c#",		"low"},
	{"l",		"l",		anything,	silent},
	{"#:^",		"l",		"%",		"aal"},
	{anything,	"lead",		anything,	"layd"},
	{anything,	"l",		anything,	"l"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule m_rules[] =
{
	{anything,	"mov",		anything,	"muwv"},
	{anything,	"m",		anything,	"m"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule n_rules[] =
{
	{"e",		"ng",		"+",		"nj"},
	{anything,	"ng",		"r",		"ngg"},
	{anything,	"ng",		"#",		"ngg"},
	{anything,	"ngl",		"%",		"nggaal"},
	{anything,	"ng",		anything,	"ng"},
	{anything,	"nk",		anything,	"ngk"},
	{nothing,	"now",		nothing,	"naw"},
	{anything,	"nn",		anything,	"n"},
	{anything,	"n",		anything,	"n"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule o_rules[] =
{
	{anything,	"of",		nothing,	"aav"},
	{anything,	"orough",	anything,	"erow"},
	{"#:",		"or",		nothing,	"er"},
	{"#:",		"ors",		nothing,	"erz"},
	{anything,	"or",		anything,	"aor"},
	{nothing,	"one",		anything,	"waan"},
	{anything,	"own",		anything,	"own"},
	{anything,	"ow",		anything,	"aw"},
	{nothing,	"over",		anything,	"owver"},
	{anything,	"ov",		anything,	"aav"},
	{anything,	"oer",		anything,	"er"},
	{anything,	"o",		"^%",		"ow"},
	{anything,	"o",		"^en",		"ow"},
	{anything,	"o",		"^i#",		"ow"},
	{anything,	"ol",		"d",		"owl"},
	{anything,	"ought",	anything,	"aot"},
	{anything,	"ough",		anything,	"aaf"},
	{nothing,	"ou",		anything,	"aw"},
	{"h",		"ou",		"s#",		"aw"},
	{anything,	"ous",		anything,	"aas"},
	{anything,	"our",		anything,	"aor"},
	{anything,	"ould",		anything,	"uhd"},
	{"^",		"ou",		"^l",		"aa"},
	{anything,	"oup",		anything,	"uwp"},
	{anything,	"ou",		anything,	"aw"},
	{anything,	"oy",		anything,	"oy"},
	{anything,	"oing",		anything,	"owihng"},
	{anything,	"oi",		anything,	"oy"},
	{anything,	"oor",		anything,	"aor"},
	{anything,	"ook",		anything,	"uhk"},
	{anything,	"ood",		anything,	"uhd"},
	{anything,	"oo",		anything,	"uw"},
	{anything,	"o",		"e",		"ow"},
	{anything,	"o",		nothing,	"ow"},
	{anything,	"oa",		anything,	"ow"},
	{nothing,	"only",		anything,	"ownlay"},
	{nothing,	"once",		anything,	"waans"},
	{anything,	"on't",		anything,	"ownt"},
	{"c",		"o",		"n",		"aa"},
	{anything,	"o",		"ng",		"ao"},
	{" :^",		"o",		"n",		"aa"},
	{"i",		"on",		anything,	"aan"},
	{"#:",		"on",		nothing,	"aan"},
	{"#^",		"on",		anything,	"aan"},
	{anything,	"o",		"st ",		"ow"},
	{anything,	"of",		"^",		"aof"},
	{anything,	"other",	anything,	"aather"},
	{anything,	"oss",		nothing,	"aos"},
	{"#:^",		"om",		anything,	"aam"},
	{anything,	"o",		anything,	"aa"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule p_rules[] =
{
	{nothing,	"ph",		anything,	"f"},
	{anything,	"ph",		nothing,	"f"},
	{anything,	"ph",		anything,	"v"},
	{anything,	"peop",		anything,	"payp"},
	{anything,	"pow",		anything,	"paw"},
	{anything,	"put",		nothing,	"puht"},
	{anything,	"p",		anything,	"p"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule q_rules[] =
{
	{anything,	"quar",		anything,	"kwaor"},
	{anything,	"qu",		anything,	"kw"},
	{anything,	"q",		anything,	"k"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule r_rules[] =
{
	{nothing,	"re",		"^#",		"ray"},
	{anything,	"r",		anything,	"r"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule s_rules[] =
{
	{anything,	"sh",		anything,	"sh"},
	{"#",		"sion",		anything,	"shaan"},
	{anything,	"some",		anything,	"saam"},
	{"#",		"sur",		"#",		"sher"},
	{anything,	"sur",		"#",		"sher"},
	{"#",		"su",		"#",		"shuw"},
	{"#",		"ssu",		"#",		"shuw"},
	{"#",		"sed",		nothing,	"zd"},
	{"#",		"s",		"#",		"z"},
	{anything,	"said",		anything,	"sehd"},
	{"^",		"sion",		anything,	"shaan"},
	{anything,	"son",		nothing,	"saan"},
	{anything,	"sen",		nothing,	"saan"},
	{anything,	"s",		"s",		silent},
	{".",		"s",		nothing,	"z"},
	{"#:.e",	"s",		nothing,	"z"},
	{"#:^##",	"s",		nothing,	"z"},
	{"#:^#",	"s",		nothing,	"s"},
	{"u",		"s",		nothing,	"s"},
	{" :#",		"s",		nothing,	"z"},
	{nothing,	"sch",		anything,	"sk"},
	{anything,	"s",		"c+",		silent},
	{"#",		"sm",		anything,	"zm"},
	{"#",		"sn",		"'",		"zaan"},
	{anything,	"s",		anything,	"s"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule t_rules[] =
{
	{nothing,	"the",		nothing,	"thaa"},
	{anything,	"to",		nothing,	"tuw"},
	{anything,	"that",		nothing,	"thaet"},
	{nothing,	"this",		nothing,	"thihs"},
	{nothing,	"they",		anything,	"thay"},
	{nothing,	"there",	anything,	"thehr"},
	{anything,	"ther",		anything,	"ther"},
	{anything,	"their",	anything,	"thehr"},
	{nothing,	"than",		nothing,	"thaen"},
	{nothing,	"them",		nothing,	"thehm"},
	{anything,	"these",	nothing,	"thayz"},
	{nothing,	"then",		anything,	"thehn"},
	{anything,	"through",	anything,	"thruw"},
	{anything,	"those",	anything,	"thowz"},
	{anything,	"though",	nothing,	"thow"},
	{nothing,	"thus",		anything,	"thaas"},
	{anything,	"th",		anything,	"th"},
	{"#:",		"ted",		nothing,	"tihd"},
	{"s",		"ti",		"#n",		"ch"},
	{anything,	"ti",		"o",		"sh"},
	{anything,	"ti",		"a",		"sh"},
	{anything,	"tien",		anything,	"shaan"},
	{anything,	"tur",		"#",		"cher"},
	{anything,	"tu",		"a",		"chuw"},
	{nothing,	"two",		anything,	"tuw"},
	{anything,	"tch",		anything,	"ch"},
	{anything,	"tsch",		anything,	"ch"},
	{anything,	"t",		anything,	"t"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule u_rules[] =
{
	{nothing,	"un",		"i",		"yuwn"},
	{nothing,	"un",		anything,	"aan"},
	{nothing,	"upon",		anything,	"aapaon"},
	{"t",		"ur",		"#",		"uhr"},
	{"s",		"ur",		"#",		"uhr"},
	{"r",		"ur",		"#",		"uhr"},
	{"d",		"ur",		"#",		"uhr"},
	{"l",		"ur",		"#",		"uhr"},
	{"z",		"ur",		"#",		"uhr"},
	{"n",		"ur",		"#",		"uhr"},
	{"j",		"ur",		"#",		"uhr"},
	{"th",		"ur",		"#",		"uhr"},
	{"ch",		"ur",		"#",		"uhr"},
	{"sh",		"ur",		"#",		"uhr"},
	{anything,	"ur",		"#",		"yuhr"},
	{anything,	"ur",		anything,	"er"},
	{anything,	"u",		"^ ",		"aa"},
	{anything,	"u",		"^^",		"aa"},
	{anything,	"uy",		anything,	"ay"},
	{" g",		"u",		"#",		silent},
	{"g",		"u",		"%",		silent},
	{"g",		"u",		"#",		"w"},
	{"#n",		"u",		anything,	"yuw"},
	{"t",		"u",		anything,	"uw"},
	{"s",		"u",		anything,	"uw"},
	{"r",		"u",		anything,	"uw"},
	{"d",		"u",		anything,	"uw"},
	{"l",		"u",		anything,	"uw"},
	{"z",		"u",		anything,	"uw"},
	{"n",		"u",		anything,	"uw"},
	{"j",		"u",		anything,	"uw"},
	{"th",		"u",		anything,	"uw"},
	{"ch",		"u",		anything,	"uw"},
	{"sh",		"u",		anything,	"uw"},
	{anything,	"u",		anything,	"yuw"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule v_rules[] =
{
	{anything,	"view",		anything,	"vyuw"},
	{anything,	"v",		anything,	"v"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule w_rules[] =
{
	{nothing,	"were",		anything,	"wer"},
	{anything,	"wa",		"s",		"waa"},
	{anything,	"wa",		"t",		"waa"},
	{anything,	"where",	anything,	"wehr"},
	{anything,	"what",		anything,	"waat"},
	{anything,	"whol",		anything,	"howl"},
	{anything,	"who",		anything,	"huw"},
	{anything,	"wh",		anything,	"w"},
	{anything,	"war",		anything,	"waor"},
	{anything,	"wor",		"^",		"wer"},
	{anything,	"wr",		anything,	"r"},
	{anything,	"w",		anything,	"w"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule x_rules[] =
{
	{anything,	"x",		anything,	"ks"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule y_rules[] =
{
	{anything,	"young",	anything,	"yaang"},
	{nothing,	"you",		anything,	"yuw"},
	{nothing,	"yes",		anything,	"yehs"},
	{nothing,	"y",		anything,	"y"},
	{"#:^",		"y",		nothing,	"ay"},
	{"#:^",		"y",		"i",		"ay"},
	{" :",		"y",		nothing,	"ay"},
	{" :",		"y",		"#",		"ay"},
	{" :",		"y",		"^+:#",		"ih"},
	{" :",		"y",		"^#",		"ay"},
	{anything,	"y",		anything,	"ih"},
	{anything,	0,		anything,	silent},
};


/*
 *	left_part	 match_part	right_part	out_part
 */
static Rule z_rules[] =
{
	{anything,	"z",		anything,	"z"},
	{anything,	0,		anything,	silent},
};

Rule	*rules[] =
{
	punct_rules,
	a_rules, b_rules, c_rules, d_rules, e_rules, f_rules, g_rules,
	h_rules, i_rules, j_rules, k_rules, l_rules, m_rules, n_rules,
	o_rules, p_rules, q_rules, r_rules, s_rules, t_rules, u_rules,
	v_rules, w_rules, x_rules, y_rules, z_rules
};
