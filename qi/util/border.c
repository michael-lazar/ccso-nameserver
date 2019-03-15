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
static char  RcsId[] = "@(#)$Id: border.c,v 1.10 1994/09/09 20:17:46 p-pomes Exp $";
#endif

/*
 * this program goes from Vax byteorder to normal byteorder, or vice-versa
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include "bintree.h"
#include "qdb.h"

#ifndef L_SET
# define L_SET 0
#endif /* !L_SET */

#ifndef L_XTND
# define L_XTND 2
#endif /* !L_XTND */

int	Error = 0;
char	*buffer;
INT32	junk;
int	Quiet = 0;

#define B_BYTES 32000

struct suffix
{
	char	*suffix;
	int	mask;
};

#define DIR 1
#define DOV 2
#define IDX 4
#define IOV 8
#define SEQ 16
#define BDX 32

main(argc, argv)
	int	argc;
	char   **argv;
{
	char	*me = *argv;
	char	*root;
	int	doMask = 0;
	int	sNum;
	static struct suffix sfx[] =
	{
		{"dir", DIR},
		{"dov", DOV},
		{"idx", IDX},
		{"iov", IOV},
		{"seq", SEQ},
		{"bdx", BDX}
	};
	int	sCount = sizeof (sfx) / sizeof (struct suffix);

	for (argc--, argv++; argc && **argv == '-'; argc--, argv++)
	{
		if (strcmp(*argv, "-q") == 0)
		{
			Quiet++;
			continue;
		}
		for (sNum = 0; sNum < sCount; sNum++)
			if (!strcmp(*argv + 1, sfx[sNum].suffix))
			{
				doMask |= sfx[sNum].mask;
				break;
			}
	}
	if (!doMask)
		doMask = -1;

	if (argc == 0)
	{
		fprintf(stderr, "Usage:%s <database>\n", me);
		exit(1);
	}
	root = *argv;
	buffer = (char *) malloc(B_BYTES);

	if (doMask & DIR)
		ReverseDir(root);
	if (doMask & DOV)
		ReverseDov(root);
	if (doMask & IDX)
		ReverseIdx(root);
	if (doMask & IOV)
		ReverseIov(root);
	if (doMask & SEQ)
		ReverseSeq(root);
	if (doMask & BDX)
		ReverseBdx(root);
	exit(Error);
}

/*
 * reverse the dir file
 */
ReverseDir(root)
	char	*root;
{
	char	filename[80];
	int	fd;
	int	bytes;
	unsigned INT32 spot;
	INT32	lastTime;
	unsigned INT32 size;

	strcpy(filename, root);
	strcat(filename, ".dir");
	if (!Quiet)
	{
		fputs(filename, stderr);
		fputc('\n', stderr);
	}

	if ((fd = open(filename, O_RDWR)) < 0)
	{
		perror(filename);
		Error = 1;
		return;
	}
	/*
	 * the header
	 */
	if (read(fd, buffer, sizeof (struct dirhead)) < sizeof (struct dirhead))
	{
		perror(filename);
		Error = 1;
		return;
	}
	ReverseLongs(buffer, sizeof (struct dirhead) / sizeof (INT32));

	if (lseek(fd, (INT32) 0, L_SET) < 0)
	{
		perror("ReverseDir");
		exit(1);
	}
	if (write(fd, buffer, sizeof (struct dirhead)) < 0)
	{
		perror("ReverseDir");
		exit(1);
	}
	/*
	 * the rest
	 */
	lastTime = 0;
	size = lseek(fd, (INT32) 0, L_XTND);
	for (spot = DRECSIZE; lseek(fd, spot, L_SET) < size; spot += DRECSIZE)
	{
		if (!Quiet && time(&junk) - lastTime > 5)
		{
			lastTime = time(&junk);
			fprintf(stderr, "%%%ld\r", spot * (unsigned INT32) 100 / size);
		}
		if ((bytes = read(fd, buffer, 4 * sizeof (INT32) + 2 * sizeof (short))) < 0)
				 break;

		ReverseLongs(buffer, 4);
		ReverseShorts(buffer + 4 * sizeof (INT32), 2);

		if (lseek(fd, spot, L_SET) < 0)
		{
			perror("ReverseDir");
			exit(1);
		}
		if (write(fd, buffer, bytes) < 0)
		{
			perror("ReverseDir");
			exit(1);
		}
	}

	/*
	 * done
	 */
	if (!Quiet)
		fputc('\n', stderr);
	close(fd);
}

/*
 * reverse the dov file
 */
ReverseDov(root)
	char	*root;
{
	char	filename[80];
	int	fd;
	int	bytes;
	unsigned INT32 spot;
	INT32	lastTime;
	unsigned INT32 size;

	strcpy(filename, root);
	strcat(filename, ".dov");
	if (!Quiet)
	{
		fputs(filename, stderr);
		fputc('\n', stderr);
	}

	if ((fd = open(filename, O_RDWR)) < 0)
	{
		perror(filename);
		Error = 1;
		return;
	}
	/*
	 * the rest
	 */
	lastTime = 0;
	size = lseek(fd, (INT32) 0, L_XTND);
	for (spot = DOVRSIZE - sizeof (INT32); lseek(fd, spot, L_SET) < size; spot += DOVRSIZE)
	{
		if (!Quiet && time(&junk) - lastTime > 5)
		{
			lastTime = time(&junk);
			fprintf(stderr, "%%%ld\r", spot * (unsigned INT32) 100 / size);
		}
		if ((bytes = read(fd, buffer, sizeof (INT32))) < 0)
				 break;

		ReverseLongs(buffer, 1);
		if (lseek(fd, spot, L_SET) < 0)
		{
			perror("ReverseDov");
			exit(1);
		}
		if (write(fd, buffer, bytes) < 0)
		{
			perror("ReverseDov");
			exit(1);
		}
	}

	/*
	 * done
	 */
	if (!Quiet)
		fputc('\n', stderr);
	close(fd);
}

/*
 * reverse the idx file
 */
ReverseIdx(root)
	char	*root;
{
	char	filename[80];
	int	fd;
	int	bytes;
	unsigned INT32 spot;
	register INT32 *where;
	INT32	lastTime;
	unsigned INT32 size;

	strcpy(filename, root);
	strcat(filename, ".idx");
	if (!Quiet)
	{
		fputs(filename, stderr);
		fputc('\n', stderr);
	}

	if ((fd = open(filename, O_RDWR)) < 0)
	{
		perror(filename);
		Error = 1;
		return;
	}
	/*
	 * the rest
	 */
	lastTime = 0;
	size = lseek(fd, (INT32) 0, L_XTND);
	for (spot = 0; lseek(fd, spot, L_SET) < size; spot += NICHARS)
	{
		if (!Quiet && time(&junk) - lastTime > 5)
		{
			lastTime = time(&junk);
			fprintf(stderr, "%%%ld\r", spot * (unsigned INT32) 100 / size);
		}
		if ((bytes = read(fd, buffer, NICHARS)) < 0)
			break;
		for (where = (INT32 *) buffer;
		     *where & 0xff && *where & 0xff00 && *where & 0xff0000 && *where & 0xff000000;
		     where++) ;
		where++;
		ReverseLongs((char *) where, bytes / sizeof (INT32) - (where - (INT32 *) buffer));

		if (lseek(fd, spot, L_SET) < 0)
		{
			perror("ReverseIdx");
			exit(1);
		}
		if (write(fd, buffer, bytes) < 0)
		{
			perror("ReverseIdx");
			exit(1);
		}
	}

	/*
	 * done
	 */
	if (!Quiet)
		fputc('\n', stderr);
	close(fd);
}

/*
 * reverse the iov file
 */
ReverseIov(root)
	char	*root;
{
	char	filename[80];
	int	fd;
	int	bytes;
	unsigned INT32 spot;
	INT32	lastTime;
	unsigned INT32 size;

	strcpy(filename, root);
	strcat(filename, ".iov");
	if (!Quiet)
	{
		fputs(filename, stderr);
		fputc('\n', stderr);
	}

	if ((fd = open(filename, O_RDWR)) < 0)
	{
		perror(filename);
		Error = 1;
		return;
	}
	/*
	 * the rest
	 */
	lastTime = 0;
	size = lseek(fd, (INT32) 0, L_XTND);
	for (spot = 0; lseek(fd, spot, L_SET) < size; spot += B_BYTES)
	{
		if (!Quiet && time(&junk) - lastTime > 5)
		{
			lastTime = time(&junk);
			fprintf(stderr, "%%%ld\r", spot * (unsigned INT32) 100 / size);
		}
		if ((bytes = read(fd, buffer, B_BYTES)) < 0)
			break;
		ReverseLongs(buffer, B_BYTES / sizeof (INT32));

		if (lseek(fd, spot, L_SET) < 0)
		{
			perror("ReverseIov");
			exit(1);
		}
		if (write(fd, buffer, bytes) < 0)
		{
			perror("ReverseIov");
			exit(1);
		}
	}

	/*
	 * done
	 */
	if (!Quiet)
		fputc('\n', stderr);
	close(fd);
}

/*
 * reverse the seq file
 */
ReverseSeq(root)
	char	*root;
{
	char	filename[80];
	int	fd;
	int	bytes;
	unsigned INT32 spot;
	register char *cSpot;
	INT32	lastTime;
	unsigned INT32 size;
	unsigned char swapC;

	strcpy(filename, root);
	strcat(filename, ".seq");
	if (!Quiet)
	{
		fputs(filename, stderr);
		fputc('\n', stderr);
	}

	if ((fd = open(filename, O_RDWR)) < 0)
	{
		perror(filename);
		Error = 1;
		return;
	}
	/*
	 * the header
	 */
	if (read(fd, buffer, sizeof (QHEADER)) < sizeof (QHEADER))
	{
		perror(filename);
		Error = 1;
		return;
	}
	ReverseLongs(buffer, sizeof (QHEADER) / sizeof (INT32));

	if (lseek(fd, (INT32) 0, L_SET) < 0)
	{
		perror("ReverseSeq");
		exit(1);
	}
	if (write(fd, buffer, sizeof (QHEADER)) < 0)
	{
		perror("ReverseSeq");
		exit(1);
	}
	/*
	 * the rest
	 */
	lastTime = 0;
	size = lseek(fd, (INT32) 0, L_XTND);
	for (spot = HEADBLKS * LBSIZE; lseek(fd, spot, L_SET) < size; spot += sizeof (LEAF))
	{
		if (!Quiet && time(&junk) - lastTime > 5)
		{
			lastTime = time(&junk);
			fprintf(stderr, "%%%ld\r", spot * (unsigned INT32) 100 / size);
		}
		/*
		 * read in the leaf
		 */
		if ((bytes = read(fd, buffer, sizeof (LEAF))) < 0)
			break;
		ReverseLongs(buffer, 3);
		for (cSpot = buffer + 3 * sizeof (INT32); cSpot < buffer + bytes;)
		{
			/*
			 * reverse the next four bytes
			 */
			swapC = cSpot[0];
			cSpot[0] = cSpot[3];
			cSpot[3] = swapC;
			swapC = cSpot[2];
			cSpot[2] = cSpot[1];
			cSpot[1] = swapC;
			cSpot += 4;
			if (cSpot < buffer + bytes)
				while (*cSpot++) ;
		}
		if (lseek(fd, spot, L_SET) < 0)
		{
			perror("ReverseSeq");
			exit(1);
		}
		if (write(fd, buffer, bytes) < 0)
		{
			perror("ReverseSeq");
			exit(1);
		}
	}

	/*
	 * done
	 */
	if (!Quiet)
		fputc('\n', stderr);
	close(fd);
}

/*
 * reverse the bdx file
 */
ReverseBdx(root)
	char	*root;
{
	char	filename[80];
	int	fd;
	int	bytes;
	unsigned INT32 spot;
	INT32	lastTime;
	unsigned INT32 size;

	strcpy(filename, root);
	strcat(filename, ".bdx");
	if (!Quiet)
	{
		fputs(filename, stderr);
		fputc('\n', stderr);
	}

	if ((fd = open(filename, O_RDWR)) < 0)
	{
		perror(filename);
		Error = 1;
		return;
	}
	/*
	 * the first INT32
	 */
	if (read(fd, buffer, sizeof (INT32)) < sizeof (INT32))
	{
		perror(filename);
		Error = 1;
		return;
	}
	ReverseLongs(buffer, 1);
	if (lseek(fd, (INT32) 0, L_SET) < 0)
	{
		perror("ReverseBdx");
		exit(1);
	}
	if (write(fd, buffer, sizeof (INT32)) < 0)
	{
		perror("ReverseBdx");
		exit(1);
	}
	/*
	 * the rest
	 */
	lastTime = 0;
	size = lseek(fd, (INT32) 0, L_XTND);
	for (spot = sizeof (NODE) - sizeof (INT32); lseek(fd, spot, L_SET) < size; spot += sizeof (NODE))
	{
		if (!Quiet && time(&junk) - lastTime > 5)
		{
			lastTime = time(&junk);
			fprintf(stderr, "%%%ld\r", spot * (unsigned INT32) 100 / size);
		}
		if ((bytes = read(fd, buffer, 2 * sizeof (INT32))) < 0)
				 break;

		ReverseLongs(buffer, 2);
		if (lseek(fd, spot, L_SET) < 0)
		{
			perror("ReverseBdx");
			exit(1);
		}
		if (write(fd, buffer, bytes) < 0)
		{
			perror("ReverseBdx");
			exit(1);
		}
	}

	/*
	 * done
	 */
	if (!Quiet)
		fputc('\n', stderr);
	close(fd);
}

/*
 * reverse some INT32s
 */
ReverseLongs(ptr, count)
	register unsigned INT32 *ptr;
	int	count;
{
	INT32	temp;
	register unsigned char *tptr = (unsigned char *) &temp;
	register unsigned char *source;

	for (; count; count--, ptr++)
	{
		source = (unsigned char *) ptr;
		temp = *ptr;
		source[0] = tptr[3];
		source[1] = tptr[2];
		source[2] = tptr[1];
		source[3] = tptr[0];
	}
}

/*
 * reverse some shorts (2 byte integers)
 */
ReverseShorts(ptr, count)
	register unsigned short *ptr;
	int	count;
{
	short	temp;
	register unsigned char *tptr = (unsigned char *) &temp;
	register unsigned char *source;

	for (; count; count--, ptr++)
	{
		source = (unsigned char *) ptr;
		temp = *ptr;
		source[0] = tptr[1];
		source[1] = tptr[0];
	}
}
