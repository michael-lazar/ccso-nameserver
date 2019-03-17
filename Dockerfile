FROM centos:7

RUN yum -y groupinstall "Development Tools"
RUN yum -y install gdbm-devel flex-devel bison-devel
RUN yum -y install flex bison bc

RUN mkdir -p /opt/nameserv/{util,source,bin,db}

COPY qi /opt/nameserv/source
COPY util /opt/nameserv/util

WORKDIR /opt/nameserv/source
RUN ./Configure centos7
RUN make install
RUN cp -r help /opt/nameserv/help

WORKDIR /opt/nameserv/util/primes
RUN make primes
RUN make install

WORKDIR /opt/nameserv/util/db_seed
RUN ./build_database.sh

EXPOSE 105

CMD python2 -m SimpleHTTPServer 105
