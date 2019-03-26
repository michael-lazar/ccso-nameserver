# CCSO Nameserver

The CCSO nameserver was an early form of database search on the Internet. 
In its most common form, it was used as an electronic "Phone Book" to
lookup information such as telephone numbers and email addresses. Today,
this service has largely been replaced by LDAP. It was used mainly in the
early-to-middle 1990s. [[1]](https://en.wikipedia.org/wiki/CCSO_Nameserver)

This repository contains a working version of the original CCSO nameserver
(aka *"Qi"*) developed by Steven Domer at University of Illinois in '93. I
salvaged the source code from some long forgotten FTP server. Then I stuck it
in a *CentOS* Docker Container, and proceeded to smash it with a hammer until
*gcc* cried uncle and submitted to compiling without errors.

The fruits of this labor can be seen here:

**cso://mozz.us:105**

There are less than a handful of CCSO servers still running on the open web.
This is probably the first new Nameserver that has been made public in over a
decade!

## Documentation

*In progress...*

## Clients (Or, how do I connect to it?)

CCSO's simple, plaintext ASCII protocol lends itself to work with a wide
range of terminal clients. Some are more sophisticated than others.

### Ph

Ph (stands for *"Phonebook"*) was the original CCSO client that was developed
alongside the Qi server by University of Illinois. The source code is bundled
with Qi, and its built and installed inside of the docker image. I've included
a shortcut command to launch Ph from the repository. Use the ``-s`` flag to
specify a server host. 

<p align="center">
<img alt="Ph Demo" src="resources/demo_ph.png"/>
</p>

### Lynx

Lynx (the curses web browser) supports CCSO links using the ``cso://`` protocol.
When you open a cso link, lynx will present you with a form that contains all of
the fields that are indexed and searchable by the CCSO server. Pretty neat!

**Example:** ``$ lynx cso://mozz.us``

<p align="center">
<img alt="Lynx Demo" src="resources/demo_lynx.png"/>
<img alt="Lynx Demo 2" src="resources/demo_lynx2.png"/>
</p>

Lynx also recognizes gopher URLs with item type 2, and will parse everything
after the ``?`` as a CCSO query.

**Example:** ``$ lynx "gopher://mozz.us:105/2?name=adams clay"``

<p align="center">
<img alt="Lynx Demo 3" src="resources/demo_lynx3.png"/>
</p>

### Telnet

Good ol' Telnet. Is there anything it can't do?

<p align="center">
<img alt="Telnet Demo" src="resources/demo_telnet.png"/>
</p>

### Python

This repo also includes a python script for automating connections to CCSO servers.
The script was borrowed from [https://github.com/jcollie/ccso](https://github.com/jcollie/ccso)
and has been ported to work with python 3.

<p align="center">
<img alt="Python Demo" src="resources/demo_python.png"/>
</p>

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
