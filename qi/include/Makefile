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
# @(#)$Id: Makefile.templ,v 1.2 1995/06/26 22:12:44 p-pomes Exp $

SHELL = /bin/sh

######################################################################
# variables set by ../configure
######################################################################
######################################################################
# other variables
######################################################################
SRCS	=
OBJS	=
BINS	=
INCLS	= bintree.h commands.h db.h field.h options.h qi.h

all:$(BINS)
######################################################################
# specific targets
######################################################################
######################################################################
# standard targets
######################################################################
install:
	-mkdir /usr/local/include
	/nfs/src/net/qi-3.1B4/bin/install.sh -c -m 444 -o nameserv -g  nameserv qiapi.h /usr/local/include
	/nfs/src/net/qi-3.1B4/bin/install.sh -c -m 444 -o nameserv -g  nameserv qicode.h /usr/local/include
	touch install

rcs:	$(INCLS) $(SRCS) $(SCRIPTS)
	ci $?
	touch rcs

clean:
	rm -f $(BINS) $(OBJS) tags install

tags:
	touch tags

depend:
# DO NOT DELETE THIS LINE.

