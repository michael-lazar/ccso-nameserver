# CCSO Nameserver

The CCSO nameserver was an early form of database search on the Internet. 
In its most common form, it was used as an electronic "Phone Book" to
lookup information such as telephone numbers and email addresses. Today,
this service has largely been replaced by LDAP. It was used mainly in the
early-to-middle 1990s. [[1]](https://en.wikipedia.org/wiki/CCSO_Nameserver)

This repository contains a working version of the original CCSO nameserver
(aka *"Qi"*) developed by Steven Domer at University of Illinois in '93. I
salvaged the source code from some long forgotten FTP server. Then I stuck it
in a CentOS Docker Container, and proceeded to smash it with a hammer until
*gcc* cried uncle and submitted to compiling without errors.

The fruits of this labor can be seen here:

**cso://mozz.us:105**

This is one of very few CCSO servers left running on the open web, and possibly
the first new server that's been lanched in over a decade.

## Documentation

*In progress...*

## Interesting Documents

A series of reference documents were published by the University of Illinois
between 1988 and 1992. These cover everything from the motivations for creating
the CCSO Nameserver, to a retrospective of what worked and what didn't.
These are a great place to start if you're looking for some historical context:

- [The CCSO Nameserver - A Description](https://mozz.us/static/ccso/description.pdf)
- [The CCSO Nameserver - Guide to Installation](https://mozz.us/static/ccso/install.pdf)
- [The CCSO Nameserver - Programmer's Guide](https://mozz.us/static/ccso/programmer.pdf)
- [The CCSO Nameserver - Why?](https://mozz.us/static/ccso/why.pdf)
- [The CCSO Nameserver - An Introduction](https://mozz.us/static/ccso/introduction.pdf)
- [The CCSO Nameserver - Server Client Protocol](https://mozz.us/static/ccso/protocol.pdf)

An RFC memo was later published in 1998 that formally describes the CCSO protocol:

- [RFC 2378](https://tools.ietf.org/html/rfc2378)

Man pages:

- [Ph User Manual](https://mozz.us/static/ccso/ph.0.pdf)
- [Qi User Manual](https://mozz.us/static/ccso/qi.0.pdf)

Other miscellaneous documentation:

- [A CCSO User Guide for the University of Illinois](https://mozz.us/static/ccso/ph.pdf) *(This one is very comprehensive!)*
- [CSO Nameserver FAQ](https://mozz.us/static/ccso/FAQ.txt)
- [Qi Installation Quick-Start](https://mozz.us/static/ccso/quick-start.txt)
- [Rebuilding a Nameserver Database in 24 Easy Steps](https://mozz.us/static/ccso/rebuild.pdf)
