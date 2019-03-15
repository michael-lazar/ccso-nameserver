#!/bin/csh -f
######################################################################
# The following tests are by no means exhaustive.  They do try to catch
# some obvious errors.
######################################################################
# Assumptions:
# - the name field must exist, Indexed, Lookup, but not Change
# - the alias field must exist, Indexed, Lookup, Change
# - the email field must exist, Indexed, Lookup, Change
# - Set the following to the address you wish to disallow in
#   email fields.
set badEmail=uiuc.edu
# - set the following to the qi you wish to use
set qi=/nameserv/src/qi/qi
# - set the following to the host and port your test qi is running on
set host=ns
set port=105
# - set the following to the ph you wish to use
set ph=/nameserv/src/ph/ph
#
# leave these alone
set ph="$ph -s $host -p $port"
alias e echo
e "######################################################################"
e qi testing
e "######################################################################"
e The following should all succeed.
$qi <<SUCCEED
add name="test 1" email=t1@foo.bar id=eno-gnitset alias=testing-one
add alias=testing-two name="test 2" email=t2@foo.bar id=owt-gnitset
change alias=testing-one make alias=testing-three
change alias=testing-three make alias=test-1
change alias=test-1 make alias=testing-one
query test
query alias=testing-one
query alias=testin*
SUCCEED
e "######################################################################"
e The following should all fail, save the last.
$qi <<FAIL
add alias=testing-one
add alias=testing-three email="t3@$badEmail"
change alias=testing-one make alias=testing-two
change alias=testing-one make email="t3@$badEmail"
query test
FAIL
e "######################################################################"
e ph testing
e "######################################################################"
e # I\'m gonna mess with your .netrc file\; if something goes wrong, you\'ll
e # have to move it back.
mv ~/.netrc{,.real}
cat <<NETRC_END >~/.netrc
machine ph
login testing-one
password -gnitset
NETRC_END
e "######################################################################"
e Ph should fail to login here, since your .netrc file is readable
$ph <<FAIL_LOGIN
me
FAIL_LOGIN
e "######################################################################"
e Ph should login now, since we fixed the .netrc file
chmod 600 ~/.netrc
$ph <<GOOD_LOGIN
me
GOOD_LOGIN
e "######################################################################"
e These things should all work
$ph <<GOOD_THINGS
me
make email=t1@bar.foo
make nickname=tiger
query tiger test
make nickname=""
query test
GOOD_THINGS
e "######################################################################"
e These things should all fail
$ph <<BAD_THINGS
change test make other=bad
make name="test but dont work"
BAD_THINGS
e "######################################################################"
e Done - cleanup
mv ~/.netrc{.real,}
echo delete alias=testing-\* | $qi
