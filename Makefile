CC := clang
CFLAGS := -g -lpthread

all: client server

clean:
	rm -rf *~ server client server.dSYM client.dSYM

server: server.c deps/socket.h deps/cJSON.h deps/cJSON.c deps/uthash.h deps/levenshtein.h game_structs.h
	$(CC) $(CFLAGS) -o server server.c deps/cJSON.c deps/levenshtein.c

client: client.c deps/socket.h game_structs.h
	$(CC) $(CFLAGS) -o client client.c
