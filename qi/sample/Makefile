WIDEPRINT =	rsh pequod enscript -rGf Courier5
#
#  This makefile is intended to help the build process.
#  Because of the changing nature of ph updates, it may not be
#  perfect, and may require tweaking from time to time
#
# Your raw materials should be:
# 	s.tape.raw	the raw student tape
#	f.tape.raw	the raw staff tape
#	s.all.raw	list of all student id's
#	old.dir		the old .dir file (symlink to ../db/prod.dir)
#	old.dov		the old .dov file (symlink to ../db/prod.dov)
#	old.cnf		the old .cnf file (symlink to ../db/prod.cnf)
#	prod.cnf	the new .cnf file (symlink to ../db/prod.cnf)
#
# Your finished products should be:
#	prod.{dir,dov,idx,iov,seq,bdx}	the new database!
#	sf.kill		the people who were deleted from the database
#
# Many intermediaries are used.
#
# IMPORTANT WARNING: Few UNIX utilities can adequately deal with large files
# of long lines.  I have had trouble with sort, awk, and grep in this regard.
# I suggest you stick to perl wherever possible.

# What sort program to use.
SORT=brute

# Directory binaries are stored in.  Only fill this in if the binaries are
# not in the search path.
B=

all:	prod.bdx

install:	prod.bdx
	chmod 660 prod.bdx prod.dov prod.iov prod.seq prod.dir prod.idx
	chown nameserv prod.bdx prod.dov prod.iov prod.seq prod.dir prod.idx
	chgrp nameserv prod.bdx prod.dov prod.iov prod.seq prod.dir prod.idx
	mv prod.bdx prod.dov prod.iov prod.seq prod.dir prod.idx  ../db

clean:
	rm -f db-size frosh.logins frosh.problems i.tape i.tape.raw \
		idx-size other.old rejects.old sf.comb sf.kill sf.new \
		sf.old sf.prefrosh sf.prephone

# Build the .bdx and .seq files from the nameserver index
prod.bdx: prod.idx
	${B}build -s prod

# Build the inverted index.  This takes a long time, and needs a fast disk.
# It first creates an empty database, so it has empty indices to start with.
# Size should be a prime number > 6*#entries.
prod.idx: prod.dir scratch idx-size
	cd scratch; ${B}credb `cat ../idx-size` prod; mv prod.idx prod.iov ..; rm prod.*; cd ..
	${B}makei prod

# Create a directory to make blank databases in
scratch:
	mkdir scratch

# Calculate the size of the needed index files
idx-size:	sf.new other.old
	sizedb prod.cnf sf.new other.old > idx-size

# Calculate the size of the needed database
db-size:	sf.new other.old
	echo `cat sf.new other.old | wc -l` " 5 * p" | dc | primes | head -1 > db-size

# This produces the main database, using maked on the concatenation of
# sf.new (the merged student, staff, and old data) and other.old (non-people
# data from the old database).  Size should be a prime =~ 5*number of entries.
prod.dir: sf.new other.old rejects.old db-size
	rm -rf prod.dir prod.dov
	${B}credb `cat db-size` prod
	rm prod.idx prod.iov
	cat sf.new other.old | ${B}maked prod

# Pull non-people out of the old database, for reinsertion.
# The timetable is done separately, so we can filter out unwanted semesters.
# "pg" is grep rewritten in perl.
other.old: old.dir old.dov
	${B}mdump other old > other.old
	${B}mdump time old | pg -v "\t4:timetable spring" >> other.old

# Entries not marked "other" or "person"
rejects.old: old.dir old.dov other.old
	${B}mdump rejects old > rejects.old
	${B}pg '^5:\t' sf.old >> rejects.old
	${B}pg -v '\t4:outsider phone' other.old | \
	${B}pg -v '\t4:unit phone' | ${B}pg -v '\t4:other phone' | \
	${B}pg -v '\t4:weather' | ${B}pg -v '\t4:serverlist' | \
	${B}pg -v '\t4:food' | ${B}pg -v '\t4:timetable' | \
	${B}pg -v '\t4:areacode'  >> rejects.old

# Build sf.new, which is sf.prefrosh + freshman logins
sf.new: sf.prefrosh
	${B}froshlog prod.cnf sf.new frosh.logins sf.prefrosh > frosh.problems

# Build sf.prefrosh, which is the data for all people in the database.
# This step runs phoneaddr to copy either the work or home phone & address
# into the default phone & address fields, and also makes sure the FERPA
# people get suppressed.
sf.prefrosh: sf.prephone s.suppress
	${B}phoneaddr sf.prephone | perl s.suppress | ${B}addnickname prod.cnf > sf.prefrosh

# Build sf.prephone, which is data for all the people, but without the
# phone and address fields.
# This step uses aliasprepare and aliasassign to assign aliases to those
# people who don't have them.  aliasassign also needs to know about any
# aliases in other.old but only prints entries with type person.
sf.prephone:	sf.comb other.old
	${B}aliasprepare prod.cnf < sf.comb |${SORT} > lop
	${B}aliasprepare prod.cnf < other.old | pg -v '6:~~~~' |${SORT} > loper
	${B}aliasassign prod.cnf loper lop > sf.prephone
	rm -f lop loper

# Build sf.comb, which is merged student, staff, and old data.
# This uses merge3, and takes quite some time.
# This step can remove old entries from the database, or not, as you wish.
sf.comb sf.kill: s.merge f.tape sf.old
	${B}merge3 prod.cnf sf.old s.merge f.tape sf.comb sf.kill
#	if you don't want deletions, uncomment the following line
#	and comment out the one after
	cat sf.kill >> sf.comb
#	${B}age prod.cnf sf.kill sf.kill.dead >> sf.comb

# Produce sf.old, the data about people from the old database.
sf.old:	old.dir old.dov
	${B}mdump people old | perl -p -e 's/\t11:incoming//;' | ${SORT} > sf.old

# s.suppress.raw is a list of the SSN's of the FERPA people.  The FERPA
# people are not in s.tape.raw, but their id's appear in s.tape.all, so
# we can generate the FERPA list by comparing the two files.
s.suppress.raw: s.tape.raw s.all.raw
	cat s.tape.raw s.all.raw | colrm 10 | sort | uniq -u > s.suppress.raw

# generate a perl script (s.suppress) to do the suppression of FERPA people.
# the script either adds or removes the suppress field, depending on whether
# or not the id is a FERPA id.
s.suppress: s.suppress.raw
	perl -p -e 's/.*/5:$$&\t/;' s.suppress.raw | \
	perl -e 'print "while(<>){\nif (0) {}\n";while(<>){chop;print "elsif (/^$$_/) {s/\t42:[^\t]*//;if (!/\t43:/){s/\$$/\t43:y/;}}\n";} print "print;\n}\n";' > s.suppress

# Unblock the raw student tape into maked format
s.tape: s.tape.raw
	${B}s.unblock prod.cnf s.tape.raw | ${SORT} > s.tape

# Unblock frosh tape into maked format
h.tape: h.tape.raw
	${B}h.unblock prod.cnf h.tape.raw > h.tape

# merge students and incoming frosh
s.merge: s.tape h.tape
	${B}merge3 prod.cnf s.tape h.tape /dev/null s.frosh s.notfrosh
	cat s.frosh s.notfrosh | ${SORT} > s.merge

# Unblock the raw staff tape into maked format
f.tape: f.tape.raw
	${B}f.unblock prod.cnf f.tape.raw | ${SORT} > f.tape

# copy over the old database, and fix byte order.  This is usually used only
# for testing; I normally do this by hand for real updates.
#old.dov old.dir:
#	echo "read testing, five minutes" |\
#	rsh garcon cat \>/nameserv/db/prod.sta
#	rcp garcon:/nameserv/db/prod.dir old.dir
#	rcp garcon:/nameserv/db/prod.dov old.dov
#	rsh garcon rm /nameserv/db/prod.sta
#	${B}border -dir -dov old
