MAKEFLAGS += --silent

.PHONY: build initdb shell ph qi qi-daemon

build:
	docker build -t ccso -f Dockerfile .

initdb:
	docker run -it --rm -v ccso:/opt/nameserv/db ccso /opt/nameserv/util/db/initdb

shell:
	docker run -it --rm -v ccso:/opt/nameserv/db ccso /bin/bash

ph:
	docker run -it --rm -v ccso:/opt/nameserv/db ccso /opt/nameserv/bin/ph

qi:
	docker run -it --rm -v ccso:/opt/nameserv/db ccso /opt/nameserv/bin/qi

qi-daemon:
	docker run -i --rm -a stdin -a stdout -v ccso:/opt/nameserv/db ccso /opt/nameserv/bin/qi -d
