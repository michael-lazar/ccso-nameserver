#!/bin/sh
# PH database build script
# Designed from numerous contributions to the info-ph list
# Coded by Noel Hunter (noel@camelcity.com)
#
# Builds a PH Database from the input stored in the file qi.input.
# During the build, works on a copy of the database, not the working version.
# If disk space is a premium, modify the script to work on prod, not prod-new.
#
# The latest version of this script is available from:
# http://www.camelcity.com/~noel/usenet/ph-build.sh
#
echo "PH database build script started..."
#
# cd to the cso library directory.  We assume all the cso programs
# are installed here:
cd /usr/app/bin
#
echo "Making a working copy of prod.cnf for building the new database..."
cp ../qi/sample/prod.cnf prod-new.cnf
cp ../qi/sample/f.tape qi.input

#
# Determine the size for the database using the "sizedb" program
# that comes with the server.  You need perl to use sizedb, along
# with the file primes.shar.  If you don't have these, you can hard-
# code in a prime bigger than the number of indexed fields (from the
# cnf file) times the number of records in your database (qi.input):
echo "Calculating size..."
size=`./sizedb prod-new.cnf qi.input`
#
# Build the database using the specifications in "prod-new.cnf", and the
# data in "qi.input"
echo "Executing credb..."
./credb $size prod-new
echo "Executing maked..."
./maked prod-new <qi.input
echo "Executing makei..."
./makei prod-new
echo "Executing build..."
./build -s prod-new
#
# Move the new database into place:
echo "Moving database into place..."
mv prod-new.bdx prod.bdx
mv prod-new.bnr prod.bnr
mv prod-new.dir prod.dir
mv prod-new.dov prod.dov
mv prod-new.idx prod.idx
mv prod-new.iov prod.iov
mv prod-new.lck prod.lck
mv prod-new.seq prod.seq
#
# Set permissions so that users cannot access the database directly.
# We assume that the qi server is running under a login that can
# access the files, if not, change "whois" below to the appropriate
# user name:
chown nameserv *
chmod -R o-rwx,g-rwx *
#
echo "PH database build script complete."