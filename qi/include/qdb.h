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
 * @(#)$Id: qdb.h,v 1.2 1994/09/09 20:14:36 p-pomes Exp $
 */

#ifndef QDB_H
#define QDB_H
#include "conf.h"
#define PTRTYPE INT32	/* type to use for record pointers */
#define PTRSIZE sizeof(PTRTYPE)	/* size of a record pointer */

/*
 * d_record sizes.  The total size should be DRECSIZE.  The number of bytes
 * for data is found by subtracting the sizes of the other fields from this. 
 */
#define NDCHARS	(DRECSIZE - 2*PTRSIZE - 2*sizeof(time_t) - 2*sizeof(short))

/*
 * d_ovrflo sizes.  The total size should be DOVRSIZE.  The number of bytes
 * for data is found by subtracting the sizes of the other fields from this. 
 */
#define NDOCHARS (DOVRSIZE - PTRSIZE)

/*
 * iindex sizes.  The total size should be NICHARS.  The number of pointers
 * available is dependent on the size of a pointer... 
 */
#define NIPTRS	(NICHARS/PTRSIZE)
#define NOPTRS	(NOCHARS/PTRSIZE)	/* 1K index overflow blocks */

#define EMPTY	 '\377'		/* delete signifies a once used but now empty
				 * node */
#define MAXTRIES 32
#define NHASH	32

/* structures for Nameserver database */

struct iindex
{
	union
	{
		char    ii_string[NICHARS];
		PTRTYPE ii_recptrs[NIPTRS];
	}       i_i;
};

#define i_string i_i.ii_string
#define i_recptrs i_i.ii_recptrs

struct d_record
{
	PTRTYPE d_ovrptr;		/* ptr to ovrflo block ( if any ) */
	PTRTYPE d_id;			/* unique id */
	time_t  d_crdate;		/* date of creation */
	time_t  d_chdate;		/* date of last modification */
	unsigned short d_dead;		/* deleted entry */
	unsigned short d_datalen;	/* length of data that follows */
	char    d_data[NDCHARS];	/* various strings, variable length */
};
typedef struct d_record DREC;

struct d_ovrflo
{
	char    d_mdata[NDOCHARS];
	PTRTYPE d_nextptr;		/* ptr to next ovrflo block */
};
typedef struct d_ovrflo DOVR;

struct dirhead
{				/* in block 0 of the .dir file */
	PTRTYPE nents;		/* number of entries in the .dir file */
	PTRTYPE next_id;	/* the next id capable of being issued */
	int     hashes[NHASH];	/* # of hashes to find index entries */
	int     nfree;		/* number of free entries in freelist,(not
				 * cur used) */
	int     freel[10];
};
#endif
