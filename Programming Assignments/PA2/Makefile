all: server

server: server.c
	gcc  -O0 -g   -o server server.c -lpthread

.PHONY : clean

clean:
	rm server