# This Makefile is automatically generated.  Do NOT edit it here.
# Copyright (c) 1985 Corporation for Research and Educational Networking
# Copyright (c) 1988 University of Illinois Board of Trustees, Steven
#		Dorner, and Paul Pomes
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the Corporation for
#	Research and Educational Networking (CREN), the University of
#	Illinois at Urbana, and their contributors.
# 4. Neither the name of CREN, the University nor the names of their
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE TRUSTEES AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE TRUSTEES OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# @(#)$Id: Makefile.templ,v 1.8 1995/06/26 22:17:55 p-pomes Exp $

SHELL = /bin/sh

######################################################################
# variables set by Configure
######################################################################
CC	= xlc
CFLAGS	= -I/nfs/src/net/qi-3.1B4/include  -D_ALL_SOURCE -DFCNTL_LOCK -DWAIT_INT -I/usr/local/include -DKRBNSREALM=\"UIUC.EDU\" -g -I/usr/local/include/kerberosIV -I/usr/local/include -I/usr/local/src/security/fwtk-1.3
LFLAGS	= -L/nfs/src/net/qi-3.1B4/lib
######################################################################
# other variables
######################################################################
SRCS	= border.c build.c credb.c id.c kpwchk.c maked.c \
	  makei.c maket.c mdump.c nsck.c ssnid.c ssnload.c ssndump.c

OBJS	= \
  border.o build.o credb.o kpwchk.o id.o \
  maked.o makei.o maket.o mdump.o \
  nsck.o ssnid.o ssnload.o ssndump.o brute2.o

BINS	= \
  border build credb kpwchk maked	\
  makei mdump nsck ssnid	\
  ssnload ssndump brute

SCRIPTS = addnickname allage studage aliasassign aliasprepare \
	a.unblock d.unblock f.unblock h.unblock n.unblock s.unblock \
	froshlog merge3 pg phify phoneaddr qierrs sizedb zaplist

ALLLIB = -lqi -lu -lqiapi
all:$(BINS)
######################################################################
# specific targets
######################################################################
brute: brute2.o
	$(CC) $(LFLAGS) -o brute brute2.o
	
build: build.o maket.o
	$(CC) $(LFLAGS) -o build build.o maket.o $(ALLLIB) -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd
	
maked : maked.o
	$(CC) $(LFLAGS) -o maked  maked.o $(ALLLIB) -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd

kpwchk : kpwchk.o
	$(CC) $(LFLAGS) -o kpwchk  kpwchk.o -ll -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd

mdump : mdump.o
	$(CC) $(LFLAGS) -o mdump  mdump.o $(ALLLIB) -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd

nsck : nsck.o ../qi/conf.o
	$(CC) $(LFLAGS) -o nsck  nsck.o $(ALLLIB) -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd

border : border.o
	$(CC) $(LFLAGS) -o border  border.o

ssnid : ssnid.o id.o
	$(CC) $(LFLAGS) -o ssnid  ssnid.o id.o -ldbm

ssndump : ssndump.o id.o
	$(CC) $(LFLAGS) -o ssndump  ssndump.o id.o -ldbm

ssnload : ssnload.o id.o
	$(CC) $(LFLAGS) -o ssnload  ssnload.o id.o -ldbm

makei : makei.o
	$(CC) $(LFLAGS) -o makei  makei.o $(ALLLIB) -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd

credb :	credb.o
	$(CC) $(LFLAGS) -o credb  credb.o $(ALLLIB) -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd

######################################################################
# standard targets
######################################################################
install:$(BINS) $(SCRIPTS)
	-mkdir /var/apps/nameserv/bin
	for b in $(BINS) ; do \
	  /nfs/src/net/qi-3.1B4/bin/install.sh -m 0755 -o nameserv -g  nameserv $$b /var/apps/nameserv/bin; done
	for b in $(SCRIPTS) ; do \
	  sed -e 's,%PerlProg%,/usr/bin/perl,' < $$b > /var/apps/nameserv/bin/$$b; done
	cd /var/apps/nameserv/bin; chmod 0755 $(SCRIPTS); chown nameserv $(SCRIPTS); \
	  chgrp nameserv $(SCRIPTS)
	touch install

rcs:	$(SRCS)
	ci $?
	touch rcs

clean:
	rm -f $(BINS) $(OBJS) tags install
depend: 
	perl -i -e 'while(<>){print;if (/^# DO NOT DELETE/) {last;}}' Makefile
	$(CC)  $(CFLAGS) $(SRCS) >> Makefile
# DO NOT DELETE THIS LINE.

