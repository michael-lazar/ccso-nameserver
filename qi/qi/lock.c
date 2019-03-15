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
static char  RcsId[] = "@(#)$Id: lock.c,v 1.27 1994/05/05 21:21:51 paul Exp $";
#endif

#include "protos.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>

#define READ  0
#define WRITE	1

static int Lock = 0;
static int Type;
jmp_buf	WhereIWas;
SIG_TYPE (*OldAlarm) __P((int));
static void LockInit();
static void TimeOut __P((int));

extern int ReadOnly;		/* mqi.c */
extern int LockTimeout;		/* qi.c */

/*
 * the following macros do all the junk required to make the lock
 * routines timeout.
 */
#define TIMEOUT_ENABLE()		\
    if (!setjmp(WhereIWas))		\
    {			      \
      OldAlarm = signal(SIGALRM,TimeOut);     \
      alarm(LockTimeout);	     \
    }			      \
    else		      \
    {			      \
      signal(SIGALRM,OldAlarm);		  \
      DoReply(LR_LOCK,"Lock timed out.");     \
      return(0);      /* timeout popped */  \
    }

#define TIMEOUT_DISABLE()		  \
    alarm(0);			  \
    signal(SIGALRM,TimeOut);

/*
 * initialize the lock mechanism
 */
static void 
LockInit()
{
	if (!Lock)
	{
		char	lockfile[256];

		(void) sprintf(lockfile, "%s.lck", Database);
		Lock = open(lockfile, O_RDWR | O_CREAT);
		if (Lock < 0)
		{
			IssueMessage(LOG_ERR, "LockInit: open(%s): %s",
				lockfile, strerror(errno));
			exit(1);
		}
		(void) chmod(lockfile, 0660);
	}
}

/*
 * Set an exclusive lock
 */
int 
GonnaWrite(msg)
char *msg;
{
#ifdef FCNTL_LOCK
	struct flock arg;

	memset(&arg, (char)0, sizeof (arg));
	arg.l_type = F_WRLCK;
#endif /* FCNTL_LOCK */
	LockInit();
	if (ReadOnly)
	{
		DoReply(LR_READONLY, "Database is currently read-only.");
		return (0);
	}
	IssueMessage(LOG_DEBUG, "%s wlock", msg);
	TIMEOUT_ENABLE();
#ifdef FCNTL_LOCK
	if (fcntl(Lock, F_SETLKW, &arg) == -1)
#else /* !FCNTL_LOCK */
	if (flock(Lock, LOCK_EX) < 0)
#endif /* FCNTL_LOCK */
	{
		DoReply(LR_LOCK, "Locking error: %s", strerror(errno));
		return (0);
	}
	TIMEOUT_DISABLE();
	Type = WRITE;
	bintree_init(Database); /* initialize the bintree code */
	read_index(Database);
	if (!dbi_init(Database) || !dbd_init(Database))
	{
		fprintf(Output, "%d:Couldn't open database.", LR_INTERNAL);
		exit(1);
	}
	get_dir_head();
	return (1);
}

/*
 * Set a shared lock
 */
int 
GonnaRead(msg)
char *msg;
{
#ifndef NO_READ_LOCK
# ifdef FCNTL_LOCK
	struct flock arg;

	memset(&arg, (char)0, sizeof (arg));
	arg.l_type = F_RDLCK;
# endif /* FCNTL_LOCK */
	LockInit();
	IssueMessage(LOG_DEBUG, "%s rlock", msg);
	TIMEOUT_ENABLE();
# ifdef FCNTL_LOCK
	if (fcntl(Lock, F_SETLKW, &arg) == -1)
# else /* !FCNTL_LOCK */
	if (flock(Lock, LOCK_SH) < 0)
# endif /* FCNTL_LOCK */
	{
		DoReply(LR_LOCK, "Locking error: %s", strerror(errno));
		return (0);
	}
	TIMEOUT_DISABLE();
#endif /* !NO_READ_LOCK */
	Type = READ;
	bintree_init(Database); /* initialize the bintree code */
	read_index(Database);
	if (!dbi_init(Database) || !dbd_init(Database))
	{
		fprintf(Output, "%d:Couldn't open database.", LR_INTERNAL);
		exit(1);
	}
	get_dir_head();
	return (1);
}

/*
 * Release a lock
 */
void 
Unlock(msg)
char *msg;
{
#ifdef FCNTL_LOCK
	struct flock arg;

	memset(&arg, (char)0, sizeof (arg));
	arg.l_type = F_UNLCK;
#endif /* FCNTL_LOCK */
	LockInit();
	if (Type == WRITE)
	{
		put_dir_head();
		put_tree_head();
	} else
		close_tree();
#ifdef NO_READ_LOCK
	if (Type == READ)
		return;
#endif
	IssueMessage(LOG_DEBUG, "%s %sunlock", msg, (Type == READ) ? "r" : "w");
#ifdef FCNTL_LOCK
	if (fcntl(Lock, F_SETLKW, &arg) == -1)
#else /* !FCNTL_LOCK */
	if (flock(Lock, LOCK_UN) < 0)
#endif /* FCNTL_LOCK */
		IssueMessage(LOG_ERR, "Unlock: flock: %s", strerror(errno));
}

/*
 * function that handles timeouts
 */
static void 
TimeOut(sig)
	int sig;
{
	longjmp(WhereIWas, 1);
}
