all: 
	cd client; \
	gcc udp_client.c -o client; \
	cd ../server; \
	gcc udp_server.c -o server; \
	cd ..

clean:
	cd client; \
	rm client; \
	cd ../server; \
	rm server; \
	cd ..
