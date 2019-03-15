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
static char  RcsId[] = "@(#)$Id: cryptit.c,v 1.3 1994/03/11 23:14:20 paul Exp $";
#endif

#ifdef PH
# ifdef __STDC__
#  include <string.h>
# else /* !__STDC__ */
#  include <strings.h>
# endif /* __STDC__ */
# include <ctype.h>
# include "qiapi.h"
#else /* !PH */
# include "protos.h"
#endif /* PH */

#define ROTORSZ 256
#define MASK 0377

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sgtty.h>

static char t1[ROTORSZ];
static char t2[ROTORSZ];
static char t3[ROTORSZ];

static int Encrypting = 0;
static int ccnt, nchars, n1, n2;

static void crypt_init __P((char *));
static void threecpy __P((unsigned char *, unsigned char *));

void 
crypt_start(pass)
	char *pass;
{

	n1 = 0;
	n2 = 0;
	ccnt = 0;
	nchars = 0;
	Encrypting = 1;
	crypt_init(pass);
}


char *
decrypt(to, from)
	char *to, *from;
{
	char	scratch[4096];
	char	*sp;
	int	count;

	count = decode((unsigned char *) scratch, (unsigned char *) from);
	for (sp = scratch; count--; sp++)
	{
		*to++ = t2[(t3[(t1[(*sp + n1) & MASK] + n2) & MASK] - n2) & MASK] - n1;

		n1++;
		if (n1 == ROTORSZ)
		{
			n1 = 0;
			n2++;
			if (n2 == ROTORSZ)
				n2 = 0;
		}
	}
	*to = '\0';		/* null-terminate */
	return (0);
}

void 
decrypt_end()
{
	Encrypting = 0;
}

/* single character decode */
#define DEC(c)	(((unsigned char)(c) - '#') & 077)

int 
decode(to, from)
	unsigned char *to, *from;
{
	int	n;
	int	chars;

	chars = n = DEC(*from++);

	while (n > 0)
	{
		/*
		 * convert groups of 3 bytes (4 Input characters).
		 */
		*to++ = DEC(*from) << 2 | DEC(from[1]) >> 4;
		*to++ = DEC(from[1]) << 4 | DEC(from[2]) >> 2;
		*to++ = DEC(from[2]) << 6 | DEC(from[3]);
		from += 4;
		n -= 3;
	}
	return (chars);
}

static void 
crypt_init(pw)
	char *pw;
{
	int	ic, i, k, temp;
	unsigned random;
	char	buf[13];
	int	seed;
	char	*crypt();

	/* must reinitialize the arrays */
	for (i = 0; i < ROTORSZ; i++)
		t1[i] = t2[i] = t3[i] = 0;

#ifdef PRE_ENCRYPT
	strncpy(buf, pw, 13);
#else
	strncpy(buf, crypt(pw, pw), 13);
#endif

	seed = 123;
	for (i = 0; i < 13; i++)
		seed = seed * buf[i] + i;
	for (i = 0; i < ROTORSZ; i++)
		t1[i] = i;
	for (i = 0; i < ROTORSZ; i++)
	{
		seed = 5 * seed + buf[i % 13];
		random = seed % 65521;
		k = ROTORSZ - 1 - i;
		ic = (random & MASK) % (k + 1);
		random >>= 8;
		temp = t1[k];
		t1[k] = t1[ic];
		t1[ic] = temp;
		if (t3[k] != 0)
			continue;
		ic = (random & MASK) % k;
		while (t3[ic] != 0)
			ic = (ic + 1) % k;
		t3[k] = ic;
		t3[ic] = k;
	}
	for (i = 0; i < ROTORSZ; i++)
		t2[t1[i] & MASK] = i;
}

/*
 * encrypts a string
 * first byte of string is encoded length of string
 * returns length of encoded string
 */
int 
encryptit(to, from)
	char *to, *from;
{
	char	scratch[4096];
	char	*sp;

	sp = scratch;
	for (; *from; from++)
	{
		*sp++ = t2[(t3[(t1[(*from + n1) & MASK] + n2) & MASK] - n2) & MASK] - n1;
		n1++;
		if (n1 == ROTORSZ)
		{
			n1 = 0;
			n2++;
			if (n2 == ROTORSZ)
				n2 = 0;
		}
	}
	return (encode(to, scratch, sp - scratch));
}

/*
 * Basic 1 character encoding function to make a char printing.
 */
#define ENC(c) (((unsigned char)(c) & 077) + '#')

int 
encode(out, buf, n)
	char *out, *buf;
	int n;
{
	int	i;
	char	*outStart;

	outStart = out;
	*out++ = ENC(n);

	for (i = 0; i < n; buf += 3, i += 3, out += 4)
		threecpy((unsigned char *) out, (unsigned char *) buf);

	/* null terminate */
	*out = '\0';
	return (out - outStart);
}

static void 
threecpy(to, from)
	unsigned char *to, *from;
{
	int	c1, c2, c3, c4;

	c1 = *from >> 2;
	c2 = (*from << 4) & 060 | (from[1] >> 4) & 017;
	c3 = (from[1] << 2) & 074 | (from[2] >> 6) & 03;
	c4 = from[2] & 077;
	*to++ = ENC(c1);
	*to++ = ENC(c2);
	*to++ = ENC(c3);
	*to = ENC(c4);
}
