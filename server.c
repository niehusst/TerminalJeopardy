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

/*
TODO: who get the points if 1 player buzzes in first but gets it wrong, and another player buzzes in later but gets it right? 
*/

category_t* category_hashmap = NULL;
game_t game;
pthread_mutex_t answer_list_lock;
pthread_mutex_t add_player_lock;
int remaining_questions = 25;

answer_t* head = NULL;

void truncate_questions_file() {
  FILE* read = fopen("JEOPARDY_QUESTIONS1.json","r");
  FILE* write = fopen("questions.json","w");
  int num_lines_wanted = 1000;
  for (int i=0; i<num_lines_wanted; i++) {
    char line[100];
    fgets(line, 100, read);
    fprintf(write, "%s", line);
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

int add_square_from_json(char* json_str) {
    cJSON* json_category;
    cJSON* json_question;
    cJSON* json_value;
    cJSON* json_answer;
    cJSON* json = cJSON_Parse(json_str);
    
    if (json == NULL) {
      return 0;
    }

    // Debugging 
    // char* parsed_json_as_string = cJSON_Print(json);
    //printf("JSON: %s\n", parsed_json_as_string);
    
    json_question = cJSON_GetObjectItem(json, "question");
    json_answer = cJSON_GetObjectItem(json, "answer");
    json_value = cJSON_GetObjectItem(json, "value");
    json_category = cJSON_GetObjectItem(json, "category");
    
    square_t new_square;
    strncpy(new_square.question, cJSON_GetStringValue(json_question), MAX_QUESTION_LENGTH-1);
    new_square.question[MAX_QUESTION_LENGTH-1] = '\0';
    strncpy(new_square.answer, cJSON_GetStringValue(json_answer), MAX_ANSWER_LENGTH-1);
    new_square.answer[MAX_ANSWER_LENGTH-1] = '\0';
    new_square.value = parseValue(cJSON_GetStringValue(json_value));
    new_square.is_answered = 0;

    char category[MAX_ANSWER_LENGTH];
    strncpy(category, cJSON_GetStringValue(json_category), MAX_ANSWER_LENGTH-1);
    category[MAX_ANSWER_LENGTH-1] = '\0';
    //printf("%s", new_square.question);
    category_t* query;
    HASH_FIND_STR(category_hashmap, category, query);
    if (query == NULL) {
      query = malloc(sizeof(category_t));
      strncpy(query->title, category, MAX_ANSWER_LENGTH-1);
      query->title[MAX_ANSWER_LENGTH-1] = '\0';
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
    if (!success) return 1;

    next = strtok(NULL, token);
  }
  
  return 0;
}

int add_player(char* name, input_t* args) {
  pthread_mutex_lock(&add_player_lock);
  if (game.num_players == MAX_NUM_PLAYERS) return 0;
  player_t new_player;
  strncpy(new_player.name, name, MAX_ANSWER_LENGTH);
  new_player.score = 0;
  new_player.id = args->id;
  new_player.socket_fd = args->socket_fd;
  game.players[game.num_players] = new_player;
  game.num_players++;
  pthread_mutex_unlock(&add_player_lock);
  return 1;
}

category_t* get_category_at_index(int index) {
  category_t* c = category_hashmap;
  for (int i=0; i<index; i++) {
     c = c->hh.next;
  }
  return c;
}

game_t create_game() {
  game_t game;
  game.num_players = 0;
  game.is_over = 0;
  game.id_of_player_turn = 0;

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
    game.categories[i] = *c;
  }
  
  return game;
}

void add_answer_to_list(answer_t* ans) {
  pthread_mutex_lock(&answer_list_lock);
  if (head == NULL) {
    head = ans;
  } else {
    answer_t* temp = ans;
    while (temp->next != NULL && temp->next->time < ans->time) {
      temp = temp->next;
    }
    ans->next = temp->next;
    temp->next = ans;
  }
  pthread_mutex_unlock(&answer_list_lock);
}


int get_quickest_answer(char* answer) {
  int correct_answer_id = -1;
  time_t best_time = -1;
  // get correct answer id
  while (head != NULL) {
    printf("CHECKING A NODE: %d %d\n", head->did_answer, check_answer(head->answer, answer));
    if (head->did_answer && check_answer(head->answer, answer)) {
      if (best_time == -1 || head->time < best_time) {
        correct_answer_id = head->id;
        best_time = head->time;
      }
    }
    
    // free node
    answer_t* temp = head;
    head = head->next;
    free(temp);
  }

  printf("Correct answer id: %d\n", correct_answer_id);
  return correct_answer_id;
}



void* handle_client(void* input) {
  // Parse username
  input_t* args = (input_t*) input;
  char username[MAX_ANSWER_LENGTH];
  int user_len = 0 ;
  if (read(args->socket_fd, &user_len, sizeof(int)) != sizeof(int)) {
    perror("Couldn't read username length");
  }
  if (read(args->socket_fd, &username, sizeof(char)*user_len) < 0) {
    char* placeholder = "Anonymous";
    strncpy(username, placeholder, strlen(placeholder)+1);
  }
  // add player to board
  add_player(username, args); // CHANGED: id is socket_fd to allow any thread to contact all clients
  if (write(args->socket_fd, &args->id, sizeof(int)) != sizeof(int)) {
    perror("Unable to send id to client!");
  }

  // Wait for enough players to have connected to play game
  while (game.num_players < MAX_NUM_PLAYERS) {
    usleep(500);
  }
  
  // TODO: sync up threads?
  // to sync, do some fancy locking shit??
  
  char* coords = (char*) malloc(sizeof(char)*2);
  // communication loop with designated client
  while (1) {
    // send the game to client
    if (write(args->socket_fd, &game, sizeof(game_t)) != sizeof(game_t)) {
      perror("Writing game didn't work");
      return NULL;
    }

    // only exit if game is over after game is sent to clients so that clients
    // also know that game is over.
    if (game.is_over) break;
    
    // get next question
    int question_value = 0;
    char* correct_ans;
    int coord_size = 3; //2 coord chars, null char
    int row, col;
    printf("Waiting on coords selection from user\n");
    // only 1 thread (corresponding to the client who buzzed first) can enter
    if (game.id_of_player_turn == args->id) {
      // get question coordinates from the client
      if (read(args->socket_fd, coords, sizeof(char)*coord_size) != sizeof(char)*coord_size) {
        perror("Reading in square selection didn't work");
        exit(2);
      }
    
      col = coords[0] - 'A';      //range A-E
      row = coords[1] - '0' - 1;  //range 1-5
      question_value = game.categories[col].questions[row].value;
      correct_ans = game.categories[col].questions[row].answer;

      // mark the question as done so it cannot be done again
      game.categories[col].questions[row].is_answered = 1;
      game.categories[col].questions[row].value = -1;
      
      // decrement global count of remaining questions
      remaining_questions--;
      
      //TODO: save read coords to somewhere other threads can see so that they can all send the same coords to their clients?
      // TODO: send coords to all clients from just one thread??
      for(int player = 0; player < MAX_NUM_PLAYERS; player++) {
        if(write(game.players[player].socket_fd, coords, sizeof(char)*coord_size) !=
           sizeof(char)*coord_size) {
          perror("Unable to send question coords!");
        }
      }
    }

    
    // get answer and buzz-time from the client
    answer_t* ans = (answer_t*)malloc(sizeof(answer_t));
    while (read(args->socket_fd, ans, sizeof(answer_t)) != sizeof(answer_t)) {
      perror("Answer was not read properly");
    }
    // add the read information to the list of answers for this round
    ans->id = args->id;
    add_answer_to_list(ans);
    
    // thread 0 always updates the score and board  TODO: maybe make it thread whose turn it is responsibiltiy?
    int correct_answer_id = -1;
    if (args->id == 0) {

      // set game as over if there are no more valid questions
      if(remaining_questions == 0) game.is_over = 1;
      
      // check the answers' correctness in order
      printf("Submitted: %s, correct: %s\n", ans->answer, correct_ans);
      correct_answer_id = get_quickest_answer(correct_ans);

      // update score and player turn if someone answered correctly
      if (correct_answer_id != -1) {
        game.players[correct_answer_id].score += question_value; //TODO: what if this isn't set locally in thread 0 because a differnt client answered??
        game.id_of_player_turn = correct_answer_id;
      }

      // TODO: move these following thigns into the update section??? <does a single thread have ability to send data to all the clients???>
      // TODO: send answer and answer correctness to all clients
      ans->id = game.id_of_player_turn;
      memcpy(ans->answer, correct_ans, MAX_ANSWER_LENGTH); //write in correct answer
      if (correct_answer_id == -1) {
        ans->did_answer = 0;
      } else {
        ans->did_answer = 1;
      }

      // send the answer struct back with results of answering round
      //TODO: send to all clients
      for(int player = 0; player < MAX_NUM_PLAYERS; player++) {
        if (write(game.players[player].socket_fd, ans, sizeof(answer_t)) != sizeof(answer_t)) {
          perror("Sending correct answer doesn't work!");
        }
      }
    
    }
    //TODO: will other threads wait??? stick on read of answer struct?
  }
  
  return NULL;
}

void run_game(int server_socket_fd, int num_connections_allowed) {
  // threads to handle each of the clients that connects 
  pthread_t thrd_arr[num_connections_allowed];

  // launch threads to handle each client
  for(int client = 0; client < num_connections_allowed; client++) {
    
    // Wait for a client to connect
    int client_socket_fd = server_socket_accept(server_socket_fd);
    if(client_socket_fd == -1) {
      perror("accept failed");
      exit(2);
    }
	
    printf("Client %d connected!\n", client);
  
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

    // Set up arguments and spin up new thread
    input_t* in = (input_t*) malloc(sizeof(input_t));
    in->to = to_client;
    in->from = from_client;
    in->socket_fd = client_socket_fd;
    in->id = client;

    printf("Starting thread to handle new client \n");
    if (pthread_create(&thrd_arr[client], NULL, handle_client, in)) {
      perror("PTHREAD CREATE FAILED:");
    }
  }

  // join threads as each one exits at the end of the game
  for(int i = 0; i < num_connections_allowed; i++) {
    if(pthread_join(thrd_arr[i], NULL) != 0) {
      perror("Failed to join thread");
    }
  }
}

int main() {

  // Initialize everything
  srand(time(NULL));
  pthread_mutex_init(&add_player_lock, NULL);
  pthread_mutex_init(&answer_list_lock, NULL);

  // Parse JSON and create a new game
  FILE* read = fopen("questions.json","r");
  parse_json(read);
  game = create_game();
  
  // Open a (arbitrary cpu chosen) server socket
  unsigned short port = 0;
  int server_socket_fd = server_socket_open(&port);
  if(server_socket_fd == -1) {
    perror("Server socket was not opened");
    exit(2);
  }
  printf("Server listening on port %u\n", port);
	
  // Start listening for connections
  int num_connections_allowed = MAX_NUM_PLAYERS;
  if(listen(server_socket_fd, num_connections_allowed)) {
    perror("listen failed");
    exit(2);
  }

  // Run the game
  run_game(server_socket_fd, num_connections_allowed);
  close(server_socket_fd);
	
  return 0;
}

/* TODO:
does answer linked list get freed?
does player struct contain contact info? can it (as the id?)?

 */
