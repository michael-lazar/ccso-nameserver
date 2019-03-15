CFLAGS= -Wall -Wextra -O2 -Dlint -D__dead= -DHAVE_OPENSSL
prefix=/usr/local

primes: primes.o pr_tbl.o pattern.o spsp.o
	$(CC) -o $@ primes.o pr_tbl.o pattern.o spsp.o -lm

install: primes
	install -m 0755 primes $(prefix)/bin

.PHONY: install
