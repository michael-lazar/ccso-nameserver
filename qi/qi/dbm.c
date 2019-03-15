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
static char  RcsId[] = "@(#)$Id: dbm.c,v 1.25 1994/09/09 20:17:03 p-pomes Exp $";
#endif

#include "protos.h"

#include <sys/types.h>
#include <sys/time.h>

#define printd	if(dbdebug)printf

static int dbdebug = 0;

struct directory_entry
{
	INT32	ent_index;
	DREC	*ent_ptr;
};

struct dirhead DirHead;
int	Have_head;
static struct directory_entry cur_ent =
{0, 0};

static int CountDir __P((char *, int));
static void MakeDir __P((QDIR *, int));
static int print_ent __P((char *));

extern int dirfd;

/*
 * This routine causes dirp to be the current directory entry. It returns 0
 * on failure and 1 on success.
 */
int 
next_ent(dirp)
	INT32 dirp;
{
	/*
	 * fetch the entry every time; who knows who may have changed it.
	 */
	/*if (dirp == cur_ent.ent_index)*/
	/*return (1);*/
	if (!dirp || dirp >= DirHead.nents)
	{
		cur_ent.ent_index = 0;
		return (0);
	}
	if (cur_ent.ent_index)
	{
		free(cur_ent.ent_ptr);
	}
	if ((cur_ent.ent_ptr = getdirent(dirp)) == NULL)
	{
		cur_ent.ent_index = 0;
		return (0);
	}
	if (cur_ent.ent_ptr->d_dead)
	{
		cur_ent.ent_index = 0;
		free(cur_ent.ent_ptr);
		cur_ent.ent_ptr = 0;
		return (0);
	}
	cur_ent.ent_index = dirp;
	if (dbdebug)
		print_ent("next_ent");
	return (1);
}

int 
ent_dead()
{
	return (cur_ent.ent_ptr->d_dead);
}

/*
 * This routine causes dirp to be the current directory entry. It returns 0
 * on failure and 1 on success.  It differs from next_ent only in that
 * it does not check the ``dead'' flag.
 */
int 
dnext_ent(dirp)
	INT32 dirp;
{
	/*
	 * fetch the entry every time; who knows who may have changed it.
	 */
	/*if (dirp == cur_ent.ent_index)*/
	/*return (1);*/
	if (!dirp || dirp >= DirHead.nents)
	{
		cur_ent.ent_index = 0;
		return (0);
	}
	if (cur_ent.ent_index)
	{
		free(cur_ent.ent_ptr);
	}
	if ((cur_ent.ent_ptr = getdirent(dirp)) == NULL)
	{
		cur_ent.ent_index = 0;
		return (0);
	}
	cur_ent.ent_index = dirp;
	if (dbdebug)
		print_ent("next_ent");
	return (1);
}

static int 
print_ent(str)
	char *str;
{
	int	i;

	printf("%s  Entry %d\n", str, cur_ent.ent_index);
	if (!cur_ent.ent_index)
	{
		printf("    no current entry./n");
		return;
	}
	printf("\td_ovrptr = %d\n", cur_ent.ent_ptr->d_ovrptr);
	printf("\td_id = %d\n", cur_ent.ent_ptr->d_id);
	printf("\td_crdate = %s", ctime(&cur_ent.ent_ptr->d_crdate));
	printf("\td_chdate = %s", ctime(&cur_ent.ent_ptr->d_chdate));
	printf("\td_datalen = %d\n", cur_ent.ent_ptr->d_datalen);
	for (i = 0; i < cur_ent.ent_ptr->d_datalen; i++)
		if (cur_ent.ent_ptr->d_data[i])
			putchar(cur_ent.ent_ptr->d_data[i]);
		else
			putchar('\n');
}

void 
store_ent()
{
	if (cur_ent.ent_ptr)
	{
		putdirent(cur_ent.ent_index, cur_ent.ent_ptr);
	}
}

void 
set_date(which)
	int which;
{
	INT32	num;

	if (!cur_ent.ent_index)
		return;
	time(&num);
	if (which)
		cur_ent.ent_ptr->d_chdate = num;
	else
		cur_ent.ent_ptr->d_crdate = num;
	printd("time = %d\n", num);
	return;
}

int 
new_ent()
{
	char	i = 0;

	if (DirHead.nents == 0)
		DirHead.nents++;

	/* extend .dir file */
	if (lseek(dirfd, ((sizeof (DREC)) * (DirHead.nents + 1)) - 1, 0) < 0)
	{
		IssueMessage(LOG_ERR, "new_ent: lseek(%d,%ld,0): %s",
			dirfd, ((sizeof (DREC)) * (DirHead.nents + 1)) - 1,
			strerror(errno));
		return (-1);
	}
	if (write(dirfd, &i, 1) < 0)
	{
		IssueMessage(LOG_ERR, "new_ent: write: %s", strerror(errno));
		return (-1);
	}
	if (cur_ent.ent_index)
	{
		free(cur_ent.ent_ptr);
	}
	/* setup current entry structure */
	cur_ent.ent_index = DirHead.nents++;
	cur_ent.ent_ptr = (DREC *) malloc(sizeof (DREC));
	memset((void *) cur_ent.ent_ptr, (char)0, sizeof (DREC));
	set_date(0);
	cur_ent.ent_ptr->d_id = DirHead.next_id++;

	if (dbdebug)
		print_ent("new_ent");

	return (cur_ent.ent_index);
}


void 
put_dir_head()
{
	if (lseek(dirfd, 0, 0) < 0)
	{
		IssueMessage(LOG_ERR, "put_dir_head: lseek(%d,0L,0): %s",
			dirfd, strerror(errno));
	}
	if (write(dirfd, &DirHead, sizeof (DirHead)) < 0)
	{
		IssueMessage(LOG_ERR, "put_dir_head: write: %s", strerror(errno));
	}
	Have_head = 0;
}

void 
get_dir_head()
{
	if (lseek(dirfd, 0, 0) < 0)
	{
		IssueMessage(LOG_ERR, "get_dir_head: lseek(%d,0L,0): %s",
			dirfd, strerror(errno));
		return;
	}
	if (read(dirfd, &DirHead, sizeof (DirHead)) < 0)
	{
		IssueMessage(LOG_ERR, "get_dir_head: read: %s", strerror(errno));
		return;
	}
	Have_head = 1;
}

char   **
getdata(dirp)
	QDIR *dirp;
{
	int	i, dsize;
	char	*ptr;

	if (!cur_ent.ent_index)
	{
		MakeDir(dirp, 0);
		(*dirp)[0] = 0;
		return (NULL);
	}
	/* fill in the pointers */
	ptr = cur_ent.ent_ptr->d_data;
	dsize = cur_ent.ent_ptr->d_datalen;
	MakeDir(dirp, CountDir(cur_ent.ent_ptr->d_data, dsize));

	for (i = 0; dsize > 0; i++, ptr++, dsize--)
	{
		(*dirp)[i] = strdup(ptr);
		while (*ptr)
		{
			ptr++;
			dsize--;
		}
	}
	(*dirp)[i] = 0;
	return ((*dirp));
}

int 
putdata(ptr_ary)
	char **ptr_ary;
{
	int	i, memsize, dsize = 0;
	char	*aptr, *dptr;
	DREC	*new_ent;

	if (!cur_ent.ent_index)
		return (0);

	/* find out how much data there is */
	for (i = 0; ptr_ary[i]; i++)
	{
		dsize += strlen(ptr_ary[i]) + 1;
	}
	/* allocate mem */
	memsize = (NDCHARS > dsize) ? 0 : (dsize - NDCHARS);
	new_ent = (DREC *) malloc(sizeof (DREC) + memsize);
	memset((void *) new_ent, (char)0, sizeof (DREC) + memsize);

	/* make copy of header info */
	new_ent->d_ovrptr = cur_ent.ent_ptr->d_ovrptr;
	new_ent->d_id = cur_ent.ent_ptr->d_id;
	new_ent->d_crdate = cur_ent.ent_ptr->d_crdate;
	new_ent->d_chdate = time(0);
	new_ent->d_dead = cur_ent.ent_ptr->d_dead;
	free(cur_ent.ent_ptr);
	cur_ent.ent_ptr = new_ent;
	set_date(1);		/* new change date */

	/* copy data into record */
	cur_ent.ent_ptr->d_datalen = dsize;

	dptr = cur_ent.ent_ptr->d_data;
	for (i = 0; ptr_ary[i]; i++)
	{
		aptr = ptr_ary[i];
		while (*dptr++ = *aptr++) ;
	}
	return (OK);
}

/*
 * Free a dirp structure
 */
void 
FreeDir(dirp)
	QDIR *dirp;
{
	char   **p;

	if (*dirp)
	{
		for (p = *dirp; *p; p++)
		{
			free(*p);
			*p = 0;
		}
		free(*dirp);
		*dirp = 0;
	}
}

/*
 * make a dirp structure
 */
static void 
MakeDir(dirp, count)
	QDIR *dirp;
	int count;
{
	*dirp = (QDIR) malloc((count + 1) * sizeof (char *));
	memset(*dirp, 0, ((count + 1) * sizeof (char *)));
}

/*
 * count the number of entries in a data string
 */
static int 
CountDir(s, size)
	char *s;
	int size;
{
	register int count;

	for (count = 0; size; s++, size--)
		if (!*s)
			count++;
	return (count);
}

/*
 * set the delete flag in ent
 */
void 
SetDeleteMark()
{
	cur_ent.ent_ptr->d_dead = 1;
}

/*
 * return the index of the current entry
 */
int 
CurrentIndex()
{
	return (cur_ent.ent_index);
}

/*
 * return the date of the current entry
 */
int 
CurrentDate()
{
	return (cur_ent.ent_ptr->d_chdate);
}
