# This Makefile is automatically generated.  Do NOT edit it here.
######################################################################
# variables set by Configure
######################################################################
CC	= gcc
CFLAGS	= -I/usr/local/src/net/qi/include  -DNO_MEMMOVE -DNO_VFPRINTF -DNO_STRSTR -DNO_STRERROR -DKRBNSREALM=\"UIUC.EDU\" -I/usr/local/include -g -I/usr/local/include
LFLAGS	= 
######################################################################
# other variables
######################################################################
SRCS	= fascist.c packlib.c rules.c stringlib.c
OBJS	= fascist.o packlib.o rules.o stringlib.o
OTHERS	= 
BINS	= /usr/local/src/net/qi/lib/libcrack.a

all: $(BINS)
######################################################################
# specific targets
######################################################################
/usr/local/src/net/qi/lib/libcrack.a:	$(OBJS)
	rm -f $@
	ar crv $@ $(OBJS)
	ranlib $@
######################################################################
# standard targets
######################################################################
rcs:	$(SRCS)
	ci $?
	touch rcs

clean:
	rm -f $(BINS) $(OBJS) $(OTHERS) tags LINT install

depend: 
	perl -i -e 'while(<>){print;if (/^# DO NOT DELETE/) {last;}}' Makefile
	$(CC)  $(CFLAGS) $(SRCS) >> Makefile

install: $(BINS)

# DO NOT DELETE THIS LINE.

