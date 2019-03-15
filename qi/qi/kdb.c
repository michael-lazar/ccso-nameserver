/*
 * $Source: /usr/local/src/net/qi/qi/RCS/kdb.c,v $
 * $Author: p-pomes $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *  adapted for qi by Paul Pomes, University of Illinois>
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * Kerberos database administrator's tool.
 */

#ifndef lint
static char  RcsId[] = "@(#)$Id: kdb.c,v 1.5 1995/06/10 03:42:13 p-pomes Exp $";
#endif

#include "protos.h"
#ifdef KDB
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <krb5/krb5.h>
#include <krb5/osconf.h>
#include <krb5/kdb.h>
#include <krb5/mit-des.h>
#include <krb5/adm_err.h>

extern char *Kdomain;

static char *mkey_name;
static krb5_encrypt_block master_encblock;
static krb5_pointer master_random;
static krb5_principal master_princ;
static krb5_keyblock master_keyblock;
static krb5_deltat max_life;
static krb5_deltat max_rlife;
static krb5_timestamp expiration;
static krb5_flags flags;
static krb5_kvno mkvno;
static krb5_db_entry master_entry;

static int kdb_create_db_entry __P((krb5_principal, krb5_db_entry *));

int
kdb_db_init()
{
    krb5_error_code retval;
    int nentries;
    krb5_boolean more;

    if (retval = krb5_db_init()) {
	DoReply(LR_KDB5, "krb5_db_init(): %s", error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_init(): %s.", error_message(retval));
	return(retval);
    }

    if (retval = krb5_db_set_lockmode(TRUE))
    {
	DoReply(LR_KDB5, "krb5_db_set_lockmode(TRUE): %s.",
	    error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_set_lockmode(TRUE): %s.",
	    error_message(retval));
	return(retval);
    }

    /* assemble & parse the master key name */
    if (retval = krb5_db_setup_mkey_name(mkey_name, Kdomain, 0,
					 &master_princ)) {
	DoReply(LR_KDB5, "krb5_db_setup_mkey_name(mkey): %s.",
	    error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_setup_mkey_name(mkey): %s.",
	    error_message(retval));
	return(retval);
    }
    nentries = 1;
    if (retval = krb5_db_get_principal(master_princ, &master_entry, &nentries,
				       &more)) {
	DoReply(LR_KDB5, "krb5_db_get_principal(mkey): %s.",
		error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_get_principal(mkey): %s.",
		    error_message(retval));
	(void) krb5_db_fini();
	return(retval);
    } else if (more) {
	DoReply(LR_KDB5, "Master krb5 key not unique.");
	IssueMessage(LOG_ERR, "Master krb5 key not unique.");
	(void) krb5_db_fini();
	return(retval);
    } else if (!nentries) {
	DoReply(LR_KDB5, "Master krb5 key not found.");
	IssueMessage(LOG_ERR, "Master krb5 key not found.");
	(void) krb5_db_fini();
	return(retval);
    }
    max_life = master_entry.max_life;
    max_rlife = master_entry.max_renewable_life;
    expiration = master_entry.expiration;
    /* don't set flags, master has some extra restrictions */
    mkvno = master_entry.kvno;

    krb5_db_free_principal(&master_entry, nentries);
    master_keyblock.keytype = DEFAULT_KDC_KEYTYPE;
    krb5_use_cstype(&master_encblock, DEFAULT_KDC_ETYPE);
    if (retval = krb5_db_fetch_mkey(master_princ, &master_encblock,
				    FALSE, FALSE, 0, &master_keyblock)) {
	DoReply(LR_KDB5, "krb5_db_fetch_mkey(mkey): %s.",
	    error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_fetch_mkey(mkey): %s.",
	    error_message(retval));
	return(retval);
    } else
    if (retval = krb5_db_verify_master_key(master_princ, &master_keyblock,
					   &master_encblock)) {
	DoReply(LR_KDB5, "krb5_db_verify_master_key(mkey): %s.",
	    error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_verify_master_key(mkey): %s.",
	    error_message(retval));
	memset((char *)master_keyblock.contents, 0, master_keyblock.length);
	krb5_xfree(master_keyblock.contents);
	return(retval);
    }
    if (retval = krb5_process_key(&master_encblock,
				  &master_keyblock)) {
	DoReply(LR_KDB5, "krb5_process_key(mkey): %s.",
	    error_message(retval));
	IssueMessage(LOG_ERR, "krb5_process_key(mkey): %s.",
	    error_message(retval));
	memset((char *)master_keyblock.contents, 0, master_keyblock.length);
	krb5_xfree(master_keyblock.contents);
	return(retval);
    }
    if (retval = krb5_init_random_key(&master_encblock,
				      &master_keyblock,
				      &master_random)) {
	DoReply(LR_KDB5, "krb5_init_random_key(mkey): %s.",
	    error_message(retval));
	IssueMessage(LOG_ERR, "krb5_init_random_key(mkey): %s.",
	    error_message(retval));
	(void) krb5_finish_key(&master_encblock);
	memset((char *)master_keyblock.contents, 0, master_keyblock.length);
	krb5_xfree(master_keyblock.contents);
	return(retval);
    }
    return 0;
}

/* kadmin delete old key function */
kdb_del_entry(alias)
char *alias;
{
    krb5_db_entry entry;
    int nprincs = 1;

    krb5_error_code retval;
    krb5_principal newprinc;
    krb5_boolean more;
    int one = 1;

    if (retval = krb5_parse_name(alias, &newprinc)) {
	DoReply(-LR_KDB5, "krb5_parse_name(%s): %s.",
		alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5_parse_name(%s): %s.",
		    alias, error_message(retval));
	return(retval);		/* Protocol Failure */
    }
    if (retval = krb5_db_get_principal(newprinc, &entry, &nprincs, &more))
    {
	DoReply(-LR_KDB5, "krb5_db_get_principal(%s): %s.",
		alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_get_principal(%s): %s.",
		    alias, error_message(retval));
	return(retval);
    }

    if (nprincs < 1)
    {
	DoReply(-LR_KDB5, "%s: not in krb5 database.", alias);
	IssueMessage(LOG_ERR, "%s: not in krb5 database.", alias);
	retval = 1;
	goto errout;
    }

    if (retval = krb5_db_delete_principal(newprinc, &one)) {
	DoReply(-LR_KDB5, "krb5_db_delete_principal(%s): %s.",
		alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_delete_principal(%s): %s.",
		    alias, error_message(retval));
	goto errout;
    } else if (one != 1) {
	DoReply(-LR_KDB5, "%s: unknown krb5 delete error.", alias);
	IssueMessage(LOG_ERR, "%s: unknown krb5 delete error.", alias);
	retval = 1;
	goto errout;
    }

    krb5_free_principal(newprinc);
    krb5_db_free_principal(&entry, nprincs);
    IssueMessage(LOG_NOTICE, "%s: krb5 entry deleted.", alias);
    return(0);

errout:
    krb5_free_principal(newprinc);
    if (nprincs)
	krb5_db_free_principal(&entry, nprincs);
    return(retval);
}


int
kdb_add_entry(alias)
    char *alias;
{
    krb5_error_code retval;
    krb5_keyblock *tempkey;
    krb5_principal newprinc;
    int nprincs = 1;
    krb5_db_entry entry;
    krb5_boolean more;

    if (retval = krb5_parse_name(alias, &newprinc)) {
	DoReply(-LR_KDB5, "krb5_parse_name(%s): %s.",
		alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5_parse_name(%s): %s.",
		    alias, error_message(retval));
	return (retval);
    }
    if (retval = krb5_db_get_principal(newprinc, &entry, &nprincs, &more)) {
	DoReply(-LR_KDB5, "krb5_db_get_principal(%s): %s.",
		alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_get_principal(%s): %s.",
		    alias, error_message(retval));
	return(retval);
    }
    if (nprincs) {
	DoReply(-LR_KDB5, "%s: already in krb5 database.", alias);
	IssueMessage(LOG_ERR, "%s: already in krb5 database.", alias);
	goto errout;
    }

    if (retval = kdb_create_db_entry(newprinc, &entry)) {
	DoReply(-LR_KDB5, "krb5 kdb_create_db_entry(%s): %s.",
	    alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5 kdb_create_db_entry(%s): %s.",
	    alias, error_message(retval));
	goto errout;
    }
    nprincs = 1;

    if (retval = krb5_random_key(&master_encblock, master_random, &tempkey)) {
	DoReply(-LR_KDB5, "%s: krb5_random_key(): %s.",
	    alias, error_message(retval));
	IssueMessage(LOG_ERR, "%s: krb5_random_key(): %s.",
	    alias, error_message(retval));
	goto errout;
    }

    entry.salt_type = entry.alt_salt_type = 0;
    entry.salt_length = entry.alt_salt_length = 0;

    retval = krb5_kdb_encrypt_key(&master_encblock, tempkey, &entry.key);
    krb5_free_keyblock(tempkey);
    if (retval) {
	DoReply(-LR_KDB5, "%s: krb5_kdb_encrypt_key(): %s.",
	    alias, error_message(retval));
	IssueMessage(LOG_ERR, "%s: krb5_kdb_encrypt_key(): %s.",
	    alias, error_message(retval));
	goto errout;
    }

    if (retval = krb5_db_put_principal(&entry, &nprincs)) {
	sleep(1);	/* retry in case of contention */
	retval = krb5_db_put_principal(&entry, &nprincs);
    }
    if (retval) {
	DoReply(-LR_KDB5, "krb5_db_put_principal(%s): %s.",
		alias, error_message(retval));
	IssueMessage(LOG_ERR, "krb5_db_put_principal(%s): %s.",
	    alias, error_message(retval));
	goto errout;
    }

    if (nprincs != 1) {
	DoReply(-LR_KDB5, "%s: unknown krb5 write error.", alias);
	IssueMessage(LOG_ERR, "%s: unknown krb5 write error.", alias);
	retval = 1;
	goto errout;
    }
    krb5_free_principal(newprinc);
    krb5_db_free_principal(&entry, nprincs);
    IssueMessage(LOG_NOTICE, "%s: krb5 entry added.", alias);
    return (0);

errout:
    krb5_free_principal(newprinc);
    if (nprincs)
	krb5_db_free_principal(&entry, nprincs);
    return(retval);
}

static int
kdb_create_db_entry(principal, newentry)
    krb5_principal principal;
    krb5_db_entry  *newentry;
{
    int	retval;

    memset(newentry, 0, sizeof(krb5_db_entry));

    if (retval = krb5_copy_principal(principal, &newentry->principal))
	return retval;
    newentry->kvno = 1;
    newentry->max_life = max_life;
    newentry->max_renewable_life = max_rlife;
    newentry->mkvno = mkvno;
    newentry->expiration = expiration;
    if (retval = krb5_copy_principal(master_princ, &newentry->mod_name))
	goto errout;

    newentry->attributes = flags;
    newentry->salt_type = KRB5_KDB_SALTTYPE_NORMAL;

    if (retval = krb5_timeofday(&newentry->mod_date))
	goto errout;

    return 0;

errout:
    if (newentry->principal)
	krb5_free_principal(newentry->principal);
    memset(newentry, 0, sizeof(krb5_db_entry));
    return retval;
}
#endif /* KDB */
