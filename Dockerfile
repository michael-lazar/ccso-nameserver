FROM centos:7

RUN yum -y groupinstall "Development Tools"
RUN yum -y install gdbm-devel flex-devel bison-devel
RUN yum -y install flex bison bc

RUN mkdir /opt/csso
RUN mkdir -p /usr/app/nameserv/bin
RUN mkdir -p /usr/app/nameserv/help
RUN mkdir -p /usr/app/nameserv/db/prod

COPY qi /opt/csso/qi
WORKDIR /opt/csso/qi
RUN ./Configure centos7
RUN make install

COPY util /opt/csso/util
WORKDIR /opt/csso/util/primes
RUN make primes
RUN make install

WORKDIR /opt/csso/util/db_seed
RUN ./build_database.sh

EXPOSE 105

CMD python2 -m SimpleHTTPServer 105
