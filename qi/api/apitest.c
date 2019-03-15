/* $Id: apitest.c,v 1.2 1995/06/23 19:28:17 p-pomes Exp $
 * apitest: test the qiapi routines.
 */
#include <stdio.h>
#include "qiapi.h"

#ifndef QIHOST
#define QIHOST "ns"
#endif

static char *UseHost = QIHOST;
static char *User = NULL;
static char *Password = NULL;
static int Opts = -1;

static void do_opts __P((int, char **));

main(argc,argv)
int argc;
char **argv;
{
  FILE *To, *From;
  int rc;
  char *alias;

  do_opts(argc,argv);
  rc = OpenQi(UseHost,&To,&From);
  if (rc) {
    fprintf(stderr,"OpenQi failed with %d\n",rc);
    exit(1);
  }
  alias = LoginQi(UseHost,To,From,Opts,User,Password); 
  if (alias)
    printf("Logged in as %s\n",alias);
  else {
    fprintf(stderr,"Login failed.\n");
    exit(1);
  }
  rc = LogoutQi(To, From);
  if (rc) 
    printf("logged out\n");
  else {
    fprintf(stderr,"logout failed %d\n",rc);
    exit(1);
  }
  CloseQi(To, From);
  exit(0);
}

static void
do_opts(argc,argv)
int argc;
char **argv;
{
  extern int optind;
  extern char *optarg;
  int c,argerr=0;

  while ((c = getopt(argc,argv,"aik:elds:u:p:")) != EOF)
    switch(c) {
    case 'd':
      ++QiDebug;
      break;
    case 's':
      UseHost = optarg;
      break;
    case 'a':
      if (Opts == -1)
	Opts = 0;
      Opts |= LQ_AUTO;
      break;
    case 'i':
      if (Opts == -1)
	Opts = 0;
      Opts |= LQ_INTERACTIVE;
      break;
    case 'k':
      if (Opts == -1)
	Opts = 0;
      if (!optarg || *optarg == '\0' || *optarg == '4')
	Opts |= LQ_KRB4;
      else if (*optarg == '5')
	Opts |= LQ_KRB5;
      break;
    case 'e':
      if (Opts == -1)
	Opts = 0;
      Opts |= LQ_EMAIL;
      break;
    case 'l':
      if (Opts == -1)
	Opts = 0;
      Opts |= LQ_PASSWORD;
      break;
    case 'u':
      User = optarg;
      break;
    case 'p':
      Password = optarg;
      break;
    default:
      ++argerr;
      break;
    }
  if (argerr) {
    fprintf(stderr,"Usage: %s [-daikel] [-s host] [-u user] [-p pass]\n",argv[0]);
    fprintf(stderr,"\t-d debug\n");
    fprintf(stderr,"\t-a attempt auto login\n");
    fprintf(stderr,"\t-i interactively prompt for user/pass if needed\n");
    fprintf(stderr,"\t-k use kerberos login protocol\n");
    fprintf(stderr,"\t-e use email login protocol\n");
    fprintf(stderr,"\t-l use conventional login protocol\n");
    exit(1);
  }
  if (Opts == -1)
    Opts = LQ_ALL|LQ_INTERACTIVE|LQ_AUTO;
}

