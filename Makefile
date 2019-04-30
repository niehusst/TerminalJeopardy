CC := clang
CFLAGS := -g -lpthread

all: client server

clean:
	rm -rf *~ server client server.dSYM client.dSYM

server: server.c socket.h game_structs.h
	$(CC) $(CFLAGS) -o server server.c

client: client.c socket.h game_structs.h
	$(CC) $(CFLAGS) -o client client.c

