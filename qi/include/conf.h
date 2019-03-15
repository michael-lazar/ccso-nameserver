#ifndef CONF_H
#define CONF_H
#define KRB4_AUTH
#define KRB5_AUTH
#define FWTK_AUTH
#define PASS_AUTH
#define EMAIL_AUTH
#define CLEAR_AUTH
#define KDB
#define NO_READ_LOCK
#define OWNER "nameserv"
#define GROUP "nameserv"
extern char *OkAddrs[];
extern char *LocalAddrs[];
extern char *Strings[];
#define QI_ALT (Strings[1])
#define TEMPFILE (Strings[3])
#define NOHELP (Strings[5])
#define HELPDIR (Strings[7])
#define MAILBOX (Strings[9])
#define QI_HOST (Strings[11])
#define ADMIN (Strings[13])
#define MAILDOMAIN (Strings[15])
#define DATABASE (Strings[17])
#define RUNDIR (Strings[19])
#define PASSW (Strings[21])
#define MAILFIELD (Strings[23])
#define NATIVESUBDIR (Strings[25])
#define PERSONLIMIT 25
#define DOVRSIZE 400
#define MEM_TYPE void
#define LOG_QILOG LOG_LOCAL0
#define CPU_LIMIT 20
#define NICHARS 32
#define INT32 long
#define MAX_KEY_LEN 16
#define NOCHARS 1024
#define DRECSIZE 400
#define SIG_TYPE void
#define MAX_ALIAS 8
extern char *Database;
#endif
