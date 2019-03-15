# This Makefile is automatically generated.  Do NOT edit it here.
######################################################################
# variables set by Configure
######################################################################
CC	= cc
CFLAGS	= -I/usr/local/src/net/qi/include 
LFLAGS	= 
######################################################################
# other variables
######################################################################
SRCS	= whoi.c
OBJS	= whoi.o
BINS	= whoi

all:$(BINS)
######################################################################
# specific targets
######################################################################
whoi	: $(OBJS) /usr/local/src/net/qi/lib/libu.a
	$(CC) $(CFLAGS) -o $@ $(OBJS) /usr/local/src/net/qi/lib/libu.a -ll
	chmod 770 $@

######################################################################
# standard targets
######################################################################
install: $(BINS)
	for b in $? ; do \
	  install -c -m 0755 -o nameserv -g  nameserv $$b /usr/local/bin; done
	touch install

rcs:	$(SRCS)
	ci $?
	touch rcs

clean:
	rm -f $(BINS) $(OBJS) libqi.a tags install

depend: 
	perl -i -e 'while(<>){print;if (/^# DO NOT DELETE/) {last;}}' Makefile
	$(CC) -M $(CFLAGS) $(SRCS) >> Makefile
# DO NOT DELETE THIS LINE.

