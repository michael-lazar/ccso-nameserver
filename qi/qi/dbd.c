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
static char  RcsId[] = "@(#)$Id: dbd.c,v 1.25 1994/09/09 20:17:03 p-pomes Exp $";
#endif

#include "protos.h"

static int	debug;

int	dirfd;			/* file descriptor for the directory file
				 * used also in dbm.c */
static int dovfd;		/* file descriptor for the directory overflow
				 * file */
static int DIRSIZE;		/* size of the directory file, in records */
static int DOVSIZE;		/* size of the directory overflow file, in
				 * records */

static int d_oread __P((DOVR *, int));
static int d_owrite __P((DOVR *, int));
static int d_read __P((DREC *, INT32));
static int d_write __P((DREC *, INT32));
static int newdovr();

/*
 * This function returns a pointer to a directory entry which is gotten by
 * indexing into the directory file with parameter entry.
 */

DREC *
getdirent(entry)
	INT32 entry;
{
	DOVR	morestrs;
	DREC	*area;
	int	asize, nextrec, i, indx;

	if ((area = (DREC *) malloc(sizeof (DREC))) == NULL)
	{
		IssueMessage(LOG_ERR, "getdirent: malloc: %s",
			strerror(errno));
		cleanup(-1);
	}
	if (!d_read(area, entry))
	{
		IssueMessage(LOG_ERR, "getdirent: d_read failed");
		return (0);
	}
	asize = area->d_datalen - NDCHARS;
	if (asize > 0)
	{			/* there's more folks */
		area = (DREC *) realloc((char *) area, (unsigned) (sizeof (DREC)
				       + (asize / NDOCHARS + 1) * NDOCHARS));
		nextrec = area->d_ovrptr;

		indx = NDCHARS;
		do
		{		/* we're going to tack the rest on the end of
			       * area */

			if (!d_oread(&morestrs, nextrec))
			{
				IssueMessage(LOG_ERR, "getdirent: d_oread failed");
				return (0);
			}
			for (i = 0; i < NDOCHARS && asize-- > 0; i++, indx++)
				area->d_data[indx] = morestrs.d_mdata[i];

		}
		while (asize > 0 && (nextrec = morestrs.d_nextptr));
	}
	return (area);
}

/*
 * This function takes the directory structure pointed to by dirp and puts
 * it in the directory file at offset entry.
 */

void 
putdirent(entry, dirp)
	INT32 entry;
	DREC *dirp;
{
	int	stringsize, i, indx, nextovr;
	DOVR ovrarea;

	/* figure out how INT32 it is */
	stringsize = dirp->d_datalen;
	stringsize -= NDCHARS;
	if (stringsize > 0)
	{			/* record larger than DREC */
		indx = NDCHARS;
		if (!dirp->d_ovrptr)
			dirp->d_ovrptr = newdovr();
		nextovr = dirp->d_ovrptr;
		do
		{
			ovrarea.d_nextptr = 0;
			d_oread(&ovrarea, nextovr);
			for (i = 0; stringsize; i++, indx++, stringsize--)
			{
				if (i == NDOCHARS)
					break;
				ovrarea.d_mdata[i] = dirp->d_data[indx];
			}
			if (stringsize && !ovrarea.d_nextptr)
				ovrarea.d_nextptr = newdovr();
			if (d_owrite(&ovrarea, nextovr) == 0)
				IssueMessage(LOG_ERR, "putdirent: d_owrite failed");

			nextovr = ovrarea.d_nextptr;
		}
		while (stringsize);
	}
	if (d_write(dirp, entry) == 0)
		IssueMessage(LOG_ERR, "putdirent: d_write failed");
#ifdef PARANOID
	{
		char	*oldData = malloc(dirp->d_datalen);
		register char *np, *op;

		for (op = oldData, np = dirp->d_data; np - dirp->d_data < dirp->d_datalen; np++, op++)
			*op = *np;
		if (getdirent(dirp) == NULL)
		{
			IssueMessage(LOG_ERR, "putdirent: PARANOID getdirent() failed");
			return;
		}
		for (op = oldData, np = dirp->d_data; np - dirp->d_data < dirp->d_datalen; np++, op++)
			if (*op != *np)
			{
				system("Mail -s \"big trouble in ph\" paul < /dev/null");
				IssueMessage(LOG_ERR, "putdirent: PARANOID detects mismatch");
				break;
			}
	}
#endif
}

static int 
d_write(x, dloc)
	DREC *x;
	INT32 dloc;
{
	if (lseek(dirfd, (int) (dloc * (sizeof *x)), 0) != dloc * (sizeof *x))
	{
		IssueMessage(LOG_ERR, "d_write: lseek(%d,%ld,0): %s",
			dirfd, (int) (dloc * (sizeof *x)), strerror(errno));
		return (0);
	}
	if (write(dirfd, (char *) x, sizeof *x) < 0)
	{
		IssueMessage(LOG_ERR, "d_write: write() at %ld: %s",
			dloc, strerror(errno));
	}
	return (1);
}

static int 
d_read(x, dloc)
	DREC *x;
	INT32 dloc;
{
	if (lseek(dirfd, dloc * (sizeof *x), 0) != dloc * (sizeof *x))
	{
		IssueMessage(LOG_ERR, "d_read: lseek(%d,%ld,0): %s",
			dirfd, (int) (dloc * (sizeof *x)), strerror(errno));
		return (0);
	}
	if (read(dirfd, (char *) x, sizeof *x) == -1)
	{
		IssueMessage(LOG_INFO, "d_read: read: %s", strerror(errno));
		return (0);
	}
	return (1);
}

static int 
d_oread(x, dloc)
	DOVR *x;
	int dloc;
{
	if (lseek(dovfd, dloc * (sizeof *x), 0) != dloc * (sizeof *x))
	{

		IssueMessage(LOG_ERR, "d_oread: lseek(%d,%ld,0): %s",
			dovfd, dloc * (sizeof *x), strerror(errno));
		return (0);
	}
	if (read(dovfd, x, sizeof *x) == -1)
	{
		IssueMessage(LOG_ERR, "d_oread: read: %s", strerror(errno));
		return (0);
	}
	return (1);
}

static int 
d_owrite(x, dloc)
	DOVR *x;
	int dloc;
{
	if (lseek(dovfd, dloc * (sizeof *x), 0) != dloc * (sizeof *x))
	{
		IssueMessage(LOG_ERR, "d_owrite: lseek(%d,%ld,0): %s",
			dovfd, dloc * (sizeof *x), strerror(errno));
		return (0);
	}
	if (write(dovfd, x, sizeof *x) == -1)
	{
		IssueMessage(LOG_ERR, "d_owrite: write: %s", strerror(errno));
		return (0);
	}
	return (1);
}

static int 
newdovr()
{
	char	i = 0;
	DOVR	*x;

	if (lseek(dovfd, (sizeof *x * (DOVSIZE + 1)) - 1, 0) < 0)
	{
		IssueMessage(LOG_ERR, "newdovr: lseek(%d,%ld,0): %s",
			dovfd, (sizeof *x * (DOVSIZE + 1)) - 1, strerror(errno));
		return (-1);
	}
	if (write(dovfd, &i, 1) < 0)
	{
		IssueMessage(LOG_ERR, "newdovr: write: %s", strerror(errno));
	}
	return (DOVSIZE++);
}

int 
dbd_init(file)
	char *file;
{
	DREC	*x;
	DOVR	*y;
	char	dirname[100], dovname[100];
	static int firstTime = 1;

	/* make file names */
	(void) strcpy(dirname, file);
	(void) strcat(dirname, ".dir");
	(void) strcpy(dovname, file);
	(void) strcat(dovname, ".dov");

	if (firstTime && (dirfd = open(dirname, 2)) < 0)
	{
		IssueMessage(LOG_ERR, "dbd_init: open(%s): %s",
			dirname, strerror(errno));
		return (0);
	}
	DIRSIZE = lseek(dirfd, 0L, 2) / sizeof *x;

	if (firstTime && (dovfd = open(dovname, 2)) < 0)
	{
		IssueMessage(LOG_ERR, "dbd_init: open(%s): %s", dovname, strerror(errno));
		return (0);
	}
	DOVSIZE = lseek(dovfd, 0L, 2) / (sizeof *y);
	if (DOVSIZE == 0)
		newdovr();
	if (debug)
		printf("dinit: DIRSIZE= %d, DOVSIZE= %d\n", DIRSIZE, DOVSIZE);
	firstTime = 0;
	return (1);
}
