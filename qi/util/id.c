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
static char  RcsId[] = "@(#)$Id: id.c,v 1.9 1994/09/09 20:17:46 p-pomes Exp $";
#endif

#include <string.h>
#include <stdio.h>
#include <ndbm.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>

static DBM *db;

/*
 * initialize the id database
 */
IdInit(dbName)
	char *dbName;
{
	if ((db = dbm_open(dbName, O_RDWR | O_CREAT, 0660)) == NULL)
		return (1);

	srand(time(0));
	return (0);
}

/*
 * make the database go away
 */
IdDone()
{
	dbm_close(db);
}

/*
 * assign an id number to an UVID
 */
char    *
AssignId(uvid)
	char    *uvid;
{
	datum    key;
	datum    rec;
	int      id;
	static char scratch[40];

	key.dptr = uvid;
	key.dsize = strlen(uvid) + 1;

	rec = dbm_fetch(db, key);
	if (rec.dptr)
		return (NULL);

	rec.dptr = scratch;

	do
	{
		id = rand() + 10000000;
		sprintf(scratch, "%c%d", strncmp(uvid, "000", 3) ? 'i' : 't', id);
		rec.dsize = strlen(scratch) + 1;
	}
	while (dbm_store(db, rec, key, DBM_INSERT) != 0);
	dbm_store(db, key, rec, DBM_INSERT);
	return (scratch);
}

/*
 * given an uvid, find an id, or vice-versa
 */
char    *
FindId(id)
	char    *id;
{
	static datum key, rec;

	key.dptr = id;
	key.dsize = strlen(id) + 1;

	rec = dbm_fetch(db, key);

	return (rec.dptr);
}

/*
 * Dump the database
 */
DumpId()
{
	datum    key, rec;
	char     scratch[256];

	for (key = dbm_firstkey(db); key.dptr; key = dbm_nextkey(db))
	{
		strcpy(scratch, key.dptr);
		key.dptr = scratch;
		rec = dbm_fetch(db, key);
		if (rec.dptr)
			printf("%s:%s\n", key.dptr, rec.dptr);
		else
			printf("%s:not found\n", key.dptr);
	}
}
