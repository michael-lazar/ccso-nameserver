run:
	docker build -t ccso_nameserver -f Dockerfile . && \
	docker run --rm -it -p 105:105 --name csso_nameserver ccso_nameserver

exec:
	docker exec -it csso_nameserver /bin/bash