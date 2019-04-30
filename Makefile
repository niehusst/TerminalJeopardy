CC := clang
CFLAGS := -g -lpthread

all: server client

clean:
	rm -rf server client server.dSYM client.dSYM

server: server.c deps/socket.h deps/cJSON.h deps/cJSON.c deps/uthash.h
	$(CC) $(CFLAGS) -o server server.c deps/cJSON.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

