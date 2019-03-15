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
# @(#)$Id: Makefile.templ,v 1.4 1995/06/26 22:17:13 p-pomes Exp $

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
SRCS	=\
    add.c auth.c bintree.c change.c commands.c conf.c \
    dbd.c dbi.c dbm.c field.c help.c kdb.c lock.c lookup.c \
    phoneme.c qi.c query.c rules.c set.c version.c
OBJS	=\
    add.o auth.o bintree.o change.o commands.o conf.o \
    dbd.o dbi.o dbm.o field.o help.o kdb.o lex.yy.o lock.o lookup.o \
    phoneme.o qi.o query.o rules.o set.o version.o
BINS	= qi

all:$(BINS) /nfs/src/net/qi-3.1B4/lib/libqi.a
######################################################################
# specific targets
######################################################################
qi	: $(OBJS) main.o
	$(CC) $(LFLAGS) -o qi main.o $(OBJS) -lu -lqiapi -ll -L/usr/local/lib -lkrb -ldes -L/usr/local/lib -lkrb5 -lkdb5 -lcrypto -lcom_err -L/usr/local/src/security/fwtk-1.3 -lauth -lfwall /usr/local/src/security/fwtk-1.3/des/libdes.a -L/usr/local/lib -lresolv -l44bsd
	chmod 770 $@

/nfs/src/net/qi-3.1B4/lib/libqi.a : $(OBJS)
	rm -f $@
	ar crv $@ $(OBJS)
	ranlib $@

lex.yy.c: language.l
	cp language.l temp.l  
	lex temp.l
	/usr/bin/perl -i -p -e 's/define YYLMAX .*/define YYLMAX 4096/;' lex.yy.c
lex.yy.o: ../include/commands.h
######################################################################
# standard targets
######################################################################
install:$(BINS)
	-mkdir /usr/local/libexec
	for b in $? ; do \
	  /nfs/src/net/qi-3.1B4/bin/install.sh -m 0755 -o nameserv -g  nameserv $$b /usr/local/libexec; done
	touch install

rcs:	$(SRCS) language.l
	ci $?
	touch rcs

clean:
	rm -f $(BINS) $(OBJS) /nfs/src/net/qi-3.1B4/lib/libqi.a tags LINT lex.yy.c temp.l main.o install

depend: 
	perl -i -e 'while(<>){print;if (/^# DO NOT DELETE/) {last;}}' Makefile
	$(CC)  $(CFLAGS) $(SRCS) >> Makefile
# DO NOT DELETE THIS LINE.

