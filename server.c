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

// Parsing JSON variables
category_t* category_hashmap = NULL;

// Creating game variables
game_t game;
pthread_mutex_t add_player_lock;
int remaining_questions = 25;

// Checking of submitted answers
pthread_mutex_t answer_list_lock;
answer_t* answers_head = NULL;


/**
 * Takes in a JSON file and outputs a new file of the first num_lines_wanted
 * lines of the file in order to have a reasonably sized file
 */
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

/**
 * Convert a value amount of $NUMBER string format into an integer
 *
 * \param val_str - the string that should be parsed
 */
int parseValue(char* val_str) {
  int val = 0;
  if (val_str == NULL) return val;
  
  val_str[0] = ' ';
  char* end;
  val = (int)strtol(val_str, &end, 10);
  return val;
}

/**
 * Takes in a string and returns the string in all lower case
 *
 * \param string - the string to make lower case
 */
char* str_tolower(char* string) {
  int len = strlen(string);
  char *ret = (char*) malloc(sizeof(char) * len);
  for(int i = 0; i < len; i++) {
    ret[i] = tolower(string[i]);
  }
  return ret;
}

/**
 * Uses the levenshtein algorithm to determine if a guess is close
 * enough to the answer to be considered correct
 *
 * \param guess - the user's guess
 * \param answer - the correct answer
 */
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

/**
 * Given a json string for a single board square (containing the question, 
 * answer, value, and category) create a new square and add it to the hashmap
 *
 * \param json_str - the json string to parse as a square
 */
int add_square_from_json(char* json_str) {
    cJSON* json_category;
    cJSON* json_question;
    cJSON* json_value;
    cJSON* json_answer;
    cJSON* json = cJSON_Parse(json_str);
    
    if (json == NULL) {
      return 0;
    }

    // Get JSON objects
    json_question = cJSON_GetObjectItem(json, "question");
    json_answer = cJSON_GetObjectItem(json, "answer");
    json_value = cJSON_GetObjectItem(json, "value");
    json_category = cJSON_GetObjectItem(json, "category");

    // Copy the string versions of the JSON objects into a new struct
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

    // Try to find the category of the question in the Hashmap. If it's there, add the square to
    // the existing category. If the category isn't not there, create a new hashmap entry and add it
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

/**
 * Read in a JSON file object by object and pass each object to the parser
 * 
 * \param input - the JSON file to read from
 */
int parse_json(FILE* input) {
  int max_file_size = 100000;
  char buffer[max_file_size];
  fgets(buffer, max_file_size, input);
  char token[2] = "}";
  char* next = strtok(buffer, token);

  // Loop over each JSON object
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

/**
 * Adds a new player to the game  with the given name and id if there is room
 *
 * \param name - the name of the player
 * \param id - the id of the player (unique)
 */
int add_player(char* name, int id) {
  pthread_mutex_lock(&add_player_lock);
  if (game.num_players == MAX_NUM_PLAYERS) return 0;
  player_t new_player;
  strncpy(new_player.name, name, MAX_ANSWER_LENGTH);
  new_player.score = 0;
  new_player.id = id;
  game.players[game.num_players] = new_player;
  game.num_players++;
  pthread_mutex_unlock(&add_player_lock);
  return 1;
}

/**
 * Simply iterates through the hashmap 'index' times to get the entry and 'index'
 *
 * \param index - the index we want to get
 */
category_t* get_category_at_index(int index) {
  category_t* c = category_hashmap;
  for (int i=0; i<index; i++) {
     c = c->hh.next;
  }
  return c;
}

/**
 * Creates an empty game, including filling out the Jeopardy board 
 */
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

/**
 * Adds the answer ans to the list of answers to be checked later (thread safe)
 *
 * param ans - the answer struct submitted by a user
 */
void add_answer_to_list(answer_t* ans) {
  pthread_mutex_lock(&answer_list_lock);
  if (answers_head == NULL) {
    answers_head = ans;
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

/**
 * Returns the user id of the user who correctly answered the question the quickest.
 * Returns -1 if no user answered correctly in time
 * 
 * \param answer - the correct answer to check against all users' answers
 */
int get_quickest_answer(char* answer) {
  int correct_answer_id = -1;
  time_t best_time = -1;
  // get correct answer id
  while (answers_head != NULL) {
    printf("CHECKING A NODE: %d %d\n", answers_head->did_answer, check_answer(answers_head->answer, answer));
    if (answers_head->did_answer && check_answer(answers_head->answer, answer)) {
      if (best_time == -1 || answers_head->time < best_time) {
        correct_answer_id = answers_head->id;
        best_time = answers_head->time;
      }
    }
    
    // free node
    answer_t* temp = answers_head;
    answers_head = answers_head->next;
    free(temp);
  }

  printf("Correct answer id: %d\n", correct_answer_id);
  return correct_answer_id;
}

/**
 * Thread function to handle each client that connected to the game
 *
 * \param input - an input struct that contains all necessary info (name, fd, etc.)
 */
void* handle_client(void* input) {
  // Parse username and add client to board
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

  add_player(username, args->id);
  if (write(args->socket_fd, &args->id, sizeof(int)) != sizeof(int)) {
    perror("Unable to send id to client!");
  }

  // Wait for enough players to have connected
  while (game.num_players < MAX_NUM_PLAYERS) {
    sleep(1);
  }
  
  // TODO: sync up threads?
  
  char* coords = (char*) malloc(sizeof(char)*2);
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
    }

    
    // TODO: send coords to all clients from just one thread??
    if(write(args->socket_fd, coords, sizeof(char)*coord_size) !=
       sizeof(char)*coord_size) {
      perror("Unable to send question coords!");
    }

    answer_t* ans = (answer_t*)malloc(sizeof(answer_t));
    // TODO: get answers from all the clients
    while (read(args->socket_fd, ans, sizeof(answer_t)) != sizeof(answer_t)) {
      perror("Answer was not read properly");
    }
    
    ans->id = args->id;
    add_answer_to_list(ans);
    // thread 0 always updates the score and board
    int correct_answer_id = -1;
    if (args->id == 0) {

      // set game as over if there are no more valid questions
      if(remaining_questions == 0) game.is_over = 1;
      
      // check the answers' correctness in order
      printf("Submitted: %s, correct: %s\n", ans->answer, correct_ans);
      correct_answer_id = get_quickest_answer(correct_ans);

      // update score and turn if someone answered correctly
      if (correct_answer_id != -1) {
        game.players[correct_answer_id].score += question_value; //TODO: what happens when this isn't set locally in thread 0 because a differnt client answered??
        game.id_of_player_turn = correct_answer_id;
      }
    }

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
    if (write(args->socket_fd, ans, sizeof(answer_t)) != sizeof(answer_t)) {
      perror("Sending correct answer doesn't work!");
    }
    
  }
  
  return NULL;
}

/**
 * Runs the game loop including waiting for clients to connect and setting up
 * the appropriate file streams
 *
 * \param server_socket_fd - the fd of the server
 * \param num_connections_allowed - the number of connections the server should allow
 */
void run_game(int server_socket_fd, int num_connections_allowed) {
  int counter = 0;
  pthread_t thrd_arr[num_connections_allowed];
  while(1){
    if(game.is_over) break;
    
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

    // Set up arguments and spin up new thread
    input_t* in = (input_t*) malloc(sizeof(input_t));
    in->to = to_client;
    in->from = from_client;
    in->socket_fd = client_socket_fd;
    in->id = counter;

    printf("Starting thread to handle new client \n");
    if (pthread_create(&thrd_arr[counter], NULL, handle_client, in)) {
      perror("PTHREAD CREATE FAILED:");
    }
    //pthread_join(thrd_arr[counter], NULL);
    //break;
    counter++;
  }
}

/**
 * Sets up the server and runs the game
 */
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
