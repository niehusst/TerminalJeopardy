#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "socket.h"

#define BUFFER_LEN 256
#define NUM_QUESTIONS_PER_CATEGORY 5
#define NUM_CATEGORIES 5
#define MAX_NUM_PLAYERS 4

typedef struct input{
  FILE* from_client;
  FILE* to_client;
  int client_socket_fd;
}input_t;

typedef struct square{
  int value;
  char* question;
  char* answer;
}square_t;

typedef struct category{
  square_t questions[NUM_QUESTIONS_PER_CATEGORY];
  char* title;
}category_t;

typedef struct player{
  char* name;
  int score;
}player_t;

typedef struct game{
  category_t* categories[NUM_CATEGORIES];
  player_t* players[MAX_NUM_PLAYERS];
  int num_players;
}game_t;


void truncate_questions_file() {
  FILE* read = fopen("JEOPARDY_QUESTIONS1.json","r");
  FILE* write = fopen("questions.json","w");
  int num_lines_wanted = 1000;
  for (int i=0; i<num_lines_wanted; i++) {
    char line[100];
    fgets(line, 100, read);
    fprintf(write, line);
  }

  fclose(read);
  fclose(write);
}

category_t* generate_category() {
  category_t* category = malloc(sizeof(category));
  // get random key from dictionary
  // if fewer than 5 questions, pick new key
  // get 5 random questions from that key and generate category
  return category;
}

int add_player(char* name, game_t* game) {
  // Needs locks
  if (game->num_players > MAX_NUM_PLAYERS) return 1;
  player_t* new_player = malloc(sizeof(player_t));
  new_player->name = name;
  new_player->score = 0;
  game->players[game->num_players] = new_player;
  game->num_players++;
  return 0;
}

game_t* create_game() {
  game_t* game = malloc(sizeof(game_t));
  game->num_players = 0;
  for (int i=0; i<NUM_CATEGORIES; i++) {
    game->categories[i] = generate_category();
  }

  return game;
}

char* str_toupper(const char* string) {
  int len = strlen(string);
  char *ret = (char*) malloc(sizeof(char) * len);
  for(int i = 0; i < len; i++) {
    ret[i] = toupper(string[i]);
  }
  return ret;
}

void* thrd_function(void* input){
  input_t* arg = (input_t*) input;
  while(1) {
    
    // Receive a message from the client
    char buffer[BUFFER_LEN];
    if(fgets(buffer, BUFFER_LEN, arg->from_client) == NULL) {
      perror("Reading from client failed");
      exit(2);
    }

    buffer[strlen(buffer)-1] = '\0';
      
    printf("Client sent: %s\n", buffer);
    
    if(strcmp("quit", buffer) == 0) {
      fprintf(arg->to_client, "%s", "Terminating connection\n");
      printf("Closing connection to Client\n");
      break;
    }
      
    // Send the message back to the client
    fprintf(arg->to_client, "'%s'\n", str_toupper(buffer));
  
    // Flush the output buffer
    fflush(arg->to_client);
  
  }
  // Close file streams
  fclose(arg->to_client);
  fclose(arg->from_client);
  
  // Close sockets
  close(arg->client_socket_fd);

  return NULL;
}

int main() {
  // Open a (arbitrary cpu chosen) server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if(server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(2);
  }
	
  // Start listening for connections, with a maximum of one queued connection
  int num_connections_allowed = 10;
  if(listen(server_socket_fd, num_connections_allowed)) {
    perror("listen failed");
    exit(2);
  }
	
  printf("Server listening on port %u\n", port);

  int counter = 0;
  pthread_t thrd_arr[num_connections_allowed];
  while(1){

    
    // Wait for a client to connect
    int client_socket_fd = server_socket_accept(server_socket_fd);
    if(client_socket_fd == -1) {
      perror("accept failed");
      exit(2);
    }
	
    printf("Client connected!\n");
  
    // Set up file streams to access the socket
    FILE* to_client = fdopen(dup(client_socket_fd), "wb");
    if(to_client == NULL) {
      perror("Failed to open stream to client");
      exit(2);
    }
  
    FILE* from_client = fdopen(dup(client_socket_fd), "rb");
    if(from_client == NULL) {
      perror("Failed to open stream from client");
      exit(2);
    }

    
    input_t* in = (input_t*) malloc(sizeof(input_t));
    in->to_client = to_client;
    in->from_client = from_client;
    in->client_socket_fd = client_socket_fd;
    printf("Starting thread to handle new client\n");
    pthread_create(&thrd_arr[counter%num_connections_allowed],NULL,
                   thrd_function, in);
    counter++;
                   
    
  }
  close(server_socket_fd);
	
  return 0;
}





















