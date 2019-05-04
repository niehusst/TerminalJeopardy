#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "game_structs.h"
#include "deps/socket.h"
#include "deps/cJSON.h"
#include "deps/uthash.h"
#include "deps/levenshtein.h"

#define BUFFER_LEN 256

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

int parseValue(char* val_str) {
  int val = 0;
  if (val_str == NULL) return val;
  
  val_str[0] = ' ';
  char* end;
  val = (int)strtol(val_str, &end, 10);
  return val;
}

char* str_tolower(char* string) {
  int len = strlen(string);
  char *ret = (char*) malloc(sizeof(char) * len);
  for(int i = 0; i < len; i++) {
    ret[i] = toupper(string[i]);
  }
  return ret;
}

int check_answer(char* guess, char* answer) {
  int is_correct = 0;
  char* guess_formatted = str_tolower(guess);
  char* answer_formatted = str_tolower(answer);
  
  size_t difference = levenshtein(guess, answer);
  int cutoff_factor = 2;
  if (difference-1 < strlen(answer)/cutoff_factor) is_correct = 1;
  
  free(guess_formatted);
  free(answer_formatted);
  return is_correct;
}

category_t* category_hashmap = NULL;
int add_square_from_json(char* json_str) {
    cJSON* json_category;
    cJSON* json_air_date;
    cJSON* json_question;
    cJSON* json_value;
    cJSON* json_answer;
    cJSON* json_round;
    cJSON* json_show_number;
    cJSON* json = cJSON_Parse(json_str);
    
    if (json == NULL) {
      //printf("Question json is NULL!");
      return 0;
    }

    // Debugging 
    //char* parsed_json_as_string = cJSON_Print(json);
    //printf("JSON: %s\n", parsed_json_as_string);
    
    json_question = cJSON_GetObjectItem(json, "question");
    json_answer = cJSON_GetObjectItem(json, "answer");
    json_value = cJSON_GetObjectItem(json, "value");
    json_category = cJSON_GetObjectItem(json, "category");
    
    square_t new_square;
    new_square.question = cJSON_GetStringValue(json_question);
    new_square.answer = cJSON_GetStringValue(json_answer);
    new_square.value = parseValue(cJSON_GetStringValue(json_value));
    char* category = cJSON_GetStringValue(json_category);

    category_t* query;
    HASH_FIND_STR(category_hashmap, category, query);
    if (query == NULL) {
      query = malloc(sizeof(category_t));
      query->title = category;
      query->questions[0] = new_square;
      query->num_questions = 1;
      HASH_ADD_STR(category_hashmap, title, query);
    } else if (query->num_questions < NUM_QUESTIONS_PER_CATEGORY) {
      query->questions[query->num_questions] = new_square;
      query->num_questions++;
    }
    // TODO: HANDLE FREEING LATER
    //cJSON_Delete(json);
    
    return 1;
}

// clrs textbook
int parse_json(FILE* input) {
  int max_file_size = 100000;
  char buffer[max_file_size];
  fgets(buffer, max_file_size, input);
  char token[2] = "}";
  char* next = strtok(buffer, token);
  
  while (next != '\0') {
    char json_buffer[max_file_size];
    strcpy(json_buffer, next);
    strcat(json_buffer, "}");
    if (json_buffer[0] == ',') {
      json_buffer[0] = ' ';
    }

    int success = add_square_from_json(json_buffer);
    if (!success) break;

    next = strtok(NULL, token);
  }
  
  return 0;
}

int add_player(char* name, game_t* game) {
  // Needs locks
  if (game->num_players == MAX_NUM_PLAYERS) return 0;
  player_t* new_player = malloc(sizeof(player_t));
  new_player->name = name;
  new_player->score = 0;
  game->players[game->num_players] = new_player;
  game->num_players++;
  // unlock
  return 1;
}

category_t* get_category_at_index(int index) {
  category_t* c = category_hashmap;
  for (int i=0; i<index; i++) {
     c = c->hh.next;
  }
  return c;
}

game_t* create_game() {
  game_t* game = malloc(sizeof(game_t));
  game->num_players = 0;

  category_t* c;
  category_t* temp;

  // Get rid of all non-completely filled categories (gets rid of final
  // jeopardy too as a consequence)
  HASH_ITER(hh, category_hashmap, c, temp) {
    if (c->num_questions != NUM_QUESTIONS_PER_CATEGORY) {
      HASH_DEL(category_hashmap, c);
    }
  }

  int map_size = HASH_COUNT(category_hashmap);

  // Selects five random categories next to each other to create a game
  int r = rand() % (map_size-5);
  for (int i=0; i<NUM_CATEGORIES; i++) {
    category_t* c = get_category_at_index(r+i);
    game->categories[i] = c;
  }
  
  return game;
}

void* thrd_function(void* input){
  input_t* arg = (input_t*) input;
  while(1) {
    
    // Receive a message from the client
    char buffer[BUFFER_LEN];
    if(fgets(buffer, BUFFER_LEN, arg->from) == NULL) {
      perror("Reading from client failed");
      exit(2);
    }

    buffer[strlen(buffer)-1] = '\0';
      
    printf("Client sent: %s\n", buffer);
    
    if(strcmp("quit", buffer) == 0) {
      fprintf(arg->to, "%s", "Terminating connection\n");
      printf("Closing connection to Client\n");
      break;
    }
      
    // Send the message back to the client
    fprintf(arg->to, "'%s'\n", buffer);
    // Flush the output buffer
    fflush(arg->to);
  
  }
  // Close file streams
  fclose(arg->to);
  fclose(arg->from);
  
  // Close sockets
  close(arg->socket_fd);

  return NULL;
}

void print_board(game_t* g) {
  for (int i=0; i<5; i++) {
    printf("%s\n", g->categories[i]->title);
    for (int j=0; j<5; j++) {
      printf("Q: %s,A: %s, V: %d\n", g->categories[i]->questions[j].question, g->categories[i]->questions[j].answer, g->categories[i]->questions[j].value);
    }
  }
}

int main() {
  srand(time(NULL));
  FILE* read = fopen("questions.json","r");
  parse_json(read);
  game_t* g = create_game();
  print_board(g);
  int a = check_answer("the neck", "your neck");
  printf("Answer: %d\n", a);
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
    in->to = to_client;
    in->from = from_client;
    in->socket_fd = client_socket_fd;
    printf("Starting thread to handle new client\n");
    pthread_create(&thrd_arr[counter%num_connections_allowed],NULL,
                   thrd_function, in);
    counter++;
                   
    
  }
  close(server_socket_fd);
	
  return 0;
}





















