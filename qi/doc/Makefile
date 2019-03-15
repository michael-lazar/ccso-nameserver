#MANMACRO= -man
MANMACRO= -mandoc

SRCS=	description.me install.me introduction.me programmer.me \
	protocol.me rebuild.me why.me ph.1 qi.8

PS=	description.ps install.ps introduction.ps programmer.ps \
	protocol.ps why.ps ph.0.ps qi.0.ps # rebuild.ps

TXT=	description.txt install.txt introduction.txt programmer.txt \
	protocol.txt why.txt ph.0 qi.0 # rebuild.txt

all:	${PS} ${TXT}

description.ps:	description.me
	groff -p -me description.me > description.ps
#	pic description.me | psroff -me > description.ps

install.ps:	install.me
	groff -me install.me > install.ps
#	psroff -me install.me > install.ps

introduction.ps:	introduction.me
	groff -t -me introduction.me > introduction.ps
#	tbl introduction.me | psroff -me > introduction.ps

programmer.ps:	programmer.me
	groff -t -me programmer.me > programmer.ps
#	tbl programmer.me | psroff -me > programmer.ps

protocol.ps:	protocol.me
	groff -me protocol.me > protocol.ps
#	psroff -me protocol.me > protocol.ps

rebuild.ps:	rebuild.me
	groff -p -me rebuild.me > rebuild.ps
#	pic rebuild.me | psroff -me > rebuild.ps

why.ps:	why.me
	groff -t -me why.me > why.ps
#	tbl why.me | psroff -me > why.ps

ph.0.ps:	ph.1
	groff ${MANMACRO} ph.1 > ph.0.ps
#	psroff ${MANMACRO} ph.1 > ph.0.ps

qi.0.ps:	qi.8
	groff ${MANMACRO} qi.8 > qi.0.ps
#	psroff ${MANMACRO} qi.8 > qi.0.ps

description.txt:	description.me
#	pic description.me | nroff -me > description.txt
	gpic description.me | nroff -me > description.txt

install.txt:	install.me
	nroff -me install.me > install.txt

introduction.txt:	introduction.me
	tbl introduction.me | nroff -me > introduction.txt

programmer.txt:	programmer.me
	tbl programmer.me | nroff -me > programmer.txt

protocol.txt:	protocol.me
	nroff -me protocol.me > protocol.txt

rebuild.txt:	rebuild.me
#	pic rebuild.me | nroff -me > rebuild.txt
	gpic rebuild.me | nroff -me > rebuild.txt

why.txt:	why.me
	tbl why.me | nroff -me > why.txt

ph.0:	ph.1
	nroff ${MANMACRO} ph.1 > ph.0

qi.0:	qi.8
	nroff ${MANMACRO} qi.8 > qi.0
