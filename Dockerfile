FROM centos:7

RUN yum -y groupinstall "Development Tools"
RUN yum -y install \
    gdbm-devel \
    flex-devel \
    bison-devel \
    flex \
    bison \
    bc \
    man \
    groff \
    vim

RUN mkdir -p /opt/nameserv/{util,source,bin,db}

COPY qi /opt/nameserv/source
COPY util /opt/nameserv/util

WORKDIR /opt/nameserv/source
RUN ./Configure centos7
RUN make install

RUN cp -r help /opt/nameserv/help
RUN cp doc/ph.1 /usr/share/man/man1/ph.1
RUN cp doc/qi.8 /usr/share/man/man8/qi.8

# This is an old BSD package used by the database generation scripts
WORKDIR /opt/nameserv/util/primes
RUN make primes
RUN make install

WORKDIR /opt/nameserv/

CMD ["/opt/nameserv/bin/qi"]
