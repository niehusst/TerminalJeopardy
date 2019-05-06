#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "deps/socket.h"
#include "game_structs.h"

#define BOARD_WIDTH 80
#define BOARD_HEIGHT 20

// Global variable defining the state of the game as seen by user
enum game_status game_state = GAME_ONGOING;

//TODO: keep track of a local score by having a total that is existing score and a temp that is the value of the question they are trying to answer (only if it was their turn(this var would be set in the question select function))?
int player_score = 0;
int temp_score = 0; //temp is added or subtracted from player score depending on whether or not they answered right
char* my_username;

/**
 * Print a reassuring message to stdin to tell them they have connected 
 * before the game starts.
 */
void wait_message() {
  printf("You've joined the game!\nWaiting for all %d people to join the game...\n", MAX_NUM_PLAYERS);
}

/**
 * Set the game_state global to end the game, and cause the main thread
 * to begin its exit procedures.
 */
void end_game() {
  // set the game as over so that the program exit sequence will begin
  game_state = GAME_OVER;
}

/**
 * Inform the user that the game has officially ended, and display their final
 * score as well as the name of the winner of the game.
 *
 * \param server - communication info for the game server
 */
void game_over(input_t* server) { 
  // Read info on what final results were

  // read scores array
  int scores[MAX_NUM_PLAYERS];
  if(read(server->socket_fd, scores, sizeof(int)*MAX_NUM_PLAYERS) !=  sizeof(int)*MAX_NUM_PLAYERS) {
    // error reading. Init scores to arbitrary values
    for(int i = 0; i < MAX_NUM_PLAYERS; i++) {
      scores[i] = 9999;
    }
  }
  // read sizes of each player's username
  int sizes[MAX_NUM_PLAYERS];
  if(read(server->socket_fd, sizes, sizeof(int)*MAX_NUM_PLAYERS) !=  sizeof(int)*MAX_NUM_PLAYERS) {
    // error reading. Init sizes to 0
    for(int i = 0; i < MAX_NUM_PLAYERS; i++) {
      sizes[i] = 0;
    }
  }
  
  // read usernames (bytes according to previously sent sizes)
  char* usernames[MAX_NUM_PLAYERS];
  for(int user = 0; user < MAX_NUM_PLAYERS; user++) {
    char* buf;
    if(sizes[user] != 0) {
      
      // read the username into buf
      if(read(server->socket_fd, buf, sizes[user]) != sizes[user]) {
        // error reading username, write a default value
        buf = malloc(sizeof(char) * 3);
        for(int i = 0; i < 3; i++) {
          buf[i] = '?';
        }
      }
      
    } else {
      // error reading username, no size of username to get
      buf = malloc(sizeof(char) * 3);

      // put default into buf
      for(int i = 0; i < 3; i++) {
        buf[i] = '?';
      }
    }

    // write buf into usernames array
    usernames[user] = buf;
  }
  
  // determine who was the winner (max score)
  int max = 0;
  for(int score = 1; score < MAX_NUM_PLAYERS; score++) {
    if(scores[max] < scores[score]) {
      max = score;
    }
  }
  char* winner = usernames[max];
  
  // Show appropriate UI for end of game
  printf("%s has won the game!\n", winner);
  //print all scores and usernames
  for(int player = 0; player < MAX_NUM_PLAYERS; player++) {
    printf("Player %s scored: %d\n", usernames[player], scores[player]);
  }
  
  // Allow user to determine when they are done looking at the message
  getchar(); // wait until user input to finish
}

/**
 * Get the list of category titles (truncated to maximum size if necessary)
 * 
 * \param game_data - holds the information on category names
 * \return cats - string array of category titles
 */
char** get_categories(board_t* game_data) {
  char** cats = (char**) malloc(sizeof(char*)*NUM_CATEGORIES);
  int max_title_len = 11;
  
  for(int cat = 0; cat < NUM_CATEGORIES; cat++) {
    //write upto max_title_len characters into the category title
    //(only that many in order to fit in printf UI)
    char* title = (char*) malloc(sizeof(char)*max_title_len);
    int written = snprintf(title, max_title_len-1, "%s", game_data->categories[cat]);
    //if snprintf fails, just put CATEGORY as a placeholder
    if(written < 0 || written > max_title_len) {
      strncpy(title, "CATEGORY", max_title_len);
    }
    cats[cat] = title;
  }
  
  return cats;
}

/**
 * Extract point values to display on the Jeopardy! game board UI from the 
 * game data sent from the server. Return extracted data
 *
 * \param game_data - data from server containing information on point values
 *                    and metadata for every question
 * \return question_point_values - string array of point values and crossed
 *                                 out completed questions
 */
char** get_board_vals(board_t* game_data) {
  char* question_done = "XXX";

  int num_questions = NUM_QUESTIONS_PER_CATEGORY;
  int num_categories = NUM_CATEGORIES;
  char** question_point_values = (char**)malloc(sizeof(char*)*(num_categories*num_questions));
  
  for(int cat = 0; cat < num_categories; cat++) {
    for(int q = 0; q < num_questions; q++) {

      int num_size = 4; //point string must hold 3 digits and null terminator
      char* points = (char*) malloc(sizeof(char)*num_size);
      
      //get question point value as a string
      int written = snprintf(points, num_size, "%d", game_data->points[q][cat]);
      //if snprintf fails, just put question_done as a placeholder
      if(written < 0 || written >= num_size) {
        strncpy(points, question_done, num_size);
      }

      //assign value if question hasn't been answerd
      question_point_values[cat*num_categories + q] = game_data->answered[q][cat] ? question_done : points;
    }
  }
  
  return question_point_values;
}

/**
 * Display the game board to the user with only valid, availble questions 
 * appearing on it.
 *
 * \param game_data - a struct that contains information about the game 
 *                    necessary for displaying the UI
 * \param categories - array of names of each category of questions
 */
void display_board(board_t* game_data) {
  int buffer_space = 13;
  //get category names of truncated length
  char** categories = get_categories(game_data);
  char** point_vals = get_board_vals(game_data);
  
  printf("+------------------------------------------------------------------------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n", buffer_space, categories[0], buffer_space, categories[1], buffer_space, categories[2], buffer_space, categories[3], buffer_space-1, categories[4], point_vals[0], point_vals[5], point_vals[10], point_vals[15], point_vals[20], point_vals[1], point_vals[6], point_vals[11], point_vals[16], point_vals[21], point_vals[2], point_vals[7], point_vals[12], point_vals[17], point_vals[22], point_vals[3], point_vals[8], point_vals[13], point_vals[18], point_vals[23], point_vals[4], point_vals[9], point_vals[14], point_vals[19], point_vals[24]);
}

void display_game(game_t* game) {
  
  for (int c=0; c<NUM_CATEGORIES; c++) {
    printf(" | %s | ", game->categories[c].title);
  }
  printf("\n");
  for (int c=0; c<NUM_CATEGORIES; c++) {
    for (int q=0; q<NUM_QUESTIONS_PER_CATEGORY; q++) {
      printf(" | %d | ", game->categories[q].questions[c].value);
    }
    printf("\n");
  }
  return;
}


/**
 * Detects if any data was entered to STDIN within the time_out
 * seconds of the function being called. Input must be entered
 * to be detected (not simply typed on command line).
 *
 * \param time_out - the number of seconds to wait for input
 * \return - boolean, True if STDIN read info, else False
 */
int timed_getchar(int time_out) {
  // timeout structure passed into select
  struct timeval tv;
  // fd_set passed into select
  fd_set fds;
  // Set up the timeout. wait up to time-out seconds
  tv.tv_sec = time_out;
  tv.tv_usec = 0;

  // Zero out the fd_set - make sure it's pristine
  FD_ZERO(&fds);
  // Set the FD that we want to read
  FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
  
  // select returns if an FD is ready or the timeout has occurred
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  // return 0 if STDIN was not read from
  return FD_ISSET(STDIN_FILENO, &fds);
}


/**
 * The "buzz-in" portion of the game; let the user provide input, and as soon as
 * they do, send the current system time to the server as a mark of
 * when the client buzzed in. This function inherently trusts the client
 * has a correctly set system time.
 *
 * \param server - communication info for the game server
 */
void buzz_in(input_t* server) {
  time_t buzz_time;
  int time_out = 3;
  
  printf("Buzz in if you know the answer!\n(Hit enter)\n");
  // blocking IO call (with timeout) to hold back client until
  // response or time-out
  if(timed_getchar(time_out)) {
    getchar(); //consume any commandline input
    //get system time of buzz in
    buzz_time = time(NULL);
  } else {
    //client didn't buzz in, set buzz_time to specific value
    buzz_time = -1;
  }

  // send buzz-in data to server
  while(write(server->socket_fd, &buzz_time, sizeof(time_t)) < 0) {
    //failed to write
  }
}



/**
 * Read all the data about the current state of the game from the server.
 *
 * \param server - communication info for the game server
 * \param game - the struct to write the read game into
 * \return board - the struct containing game data sent from the server  
 */
game_t* get_game(input_t* server, game_t* game) {
  // read game_t from server and save into parameter game
  if (read(server->socket_fd, game, sizeof(game_t)) != sizeof(game_t)) {
    perror("Reading in game_t didn't work");
    exit(2);
  }
  return game;
}

/**
 * Check if the user input coordinate of a question are valid: coordinates
 * are within range of the game board and the question has not already been
 * answered.
 * 
 * \param coords - string of 2 characters; a letter and number representing 
 *                 coordinates of a question on the game board
 * \return validity - boolean, whether or not the coords were valid 
 *                    (True if they are valid, else False) 
 */
int choice_valid(char* coords, board_t* board) {
  int validity = 0;
  // extract numeric coords within range of 0-4 
  int row = coords[0] - 'A';      //range A-E
  int col = coords[1] - '0' - 1;  //range 1-5
  
  // check coords are within range
  if(row > 4 || row < 0 || col > 4 || col < 0) return validity;
  
  // check question hasn't already been answered
  if(!board->answered[row][col]) {
    // coords are valid choice
    validity = 1;
  }
  
  return validity;
}

/**
 * Get user input, allowing the user to select the question they 
 * want to answer. Send to the server so that other clients can receive
 * the same information.
 *
 * \param server - communication info for the game server
 */
void select_question(input_t* server, board_t* board) {
  //read client question choice; input must take coordinate form
  //    letter row, number column (A1 through E5)
  int coord_size = 2;
  char coords[coord_size+1];
  printf("Congrats! You buzzed in first! What question do you choose?\n");
  printf("(Choice must be in coordinate form: letter row, number column (e.g. E5))\n");
  while(fgets(coords, coord_size, stdin) != NULL && choice_valid(coords, board)) {
    //read failed
    printf("Unfortunately, that is not a valid choice.\nPlease pick a different question.\n");
  }

  // send coords to server (until success)
  while(write(server->socket_fd, coords, sizeof(char)*coord_size+1) == -1) {}
}

/**
 * Get user input that is the answer to a displayed question. Send the
 * input answer to the server so it can be distributed to other clients.
 *
 * \param server - communication info for the game server
 */
void answer_question(input_t* server) {
  size_t max_ans_len = 255;
  char answer[max_ans_len];

  // get client's answer from standard input
  while(fgets(answer, max_ans_len, stdin) == NULL) {
    // error getting input
    printf("Sorry, we didn't quite catch that. What was your answer?\n");
  }

  // send answer to the server (+1 for null terminator)
  while(write(server->socket_fd, answer, strlen(answer)+1) == -1) {
    // write failed
  }
}

/**
 * Display the real and guessed answers to the ~beautiful~ UI for
 * a few seconds.
 *
 * \param real - string, the actual answer to the question
 * \param predicted - string, the answer guessed by the client who buzzed 
 * \param answerer - string, the username of the client who answered
 * \param is_correct - bool, True if the answer was correct, else False
 */
void display_answers(char* real, char* predicted, char* answerer, int is_correct) {
  printf("%s said:\n%s\n\n", answerer, predicted);
  
  // display the true answer if their guess was wrong
  if(is_correct) {
    printf("That was absolutely correct!\n");
  } else {
    printf("Unfortuneately, the correct answer was,\n%s\n\n", real);
  }
}

/**
 * Get the answer guessed by the client who buzzed in, the real answer
 * to the question, whether or not the answer was correct, and the 
 * answerer's username from the server.
 *
 * \param server - communication info for the game server
 */
void get_answers(input_t* server) {
  answer_sizes_t sizes;
  
  //read info on sizes of future reads from server
  while(read(server->socket_fd, &sizes, sizeof(answer_sizes_t)) == -1) {
    // try again while failing
  }

  // read predicted answer
  char* pred;
  while(read(server->socket_fd, pred, sizeof(char)*sizes.pred_ans_size) == -1) {
    // try again while failing
  }
 
  // read actual answer
  char* real;
  while(read(server->socket_fd, real, sizeof(char)*sizes.real_ans_size) == -1) {
    // try again while failing
  }
  
  // read username
  char* user;
  while(read(server->socket_fd, user, sizeof(char)*sizes.user_size) == -1) {
    // try again while failing
  }
  
  // read whether answer was correct
  int is_correct;
  while(read(server->socket_fd, &is_correct, sizeof(int)) == -1) {
    // try again while failing
  }
  
  //display correct answer and attempted answer w/ correctness to UI
  display_answers(real, pred, user, is_correct);
}

/**
 * Show the question that the client should answer to the UI.
 *
 * \param q - string question that needs to be displayed on UI
 */
void display_question(char* q) {
  printf("The question is:\n%s\n",q);
}

/**
 * Get the question that were selected by the client whose turn it was.
 * Serves to blocks client until the buzz-in period begins.
 *
 * \param server - communication info for the game server  
 */
void get_question(input_t* server) {
  // read size of question
  int size_of_question;
  while(read(server->socket_fd, &size_of_question, sizeof(int)) == -1) {
    // read failed, try again looping
  }
  
  // read question
  char* question;
  while(read(server->socket_fd, question, sizeof(char)*size_of_question) == -1) {
    // read failed, try again looping
  }
  
  // show on UI
  display_question(question);
}

/**
 * Get data from the server, telling this client if it is their turn.
 *
 * \param server - contains all info necessary for communicating with the server
 * \return my_turn - boolean, True if it is the client's turn, False otherwise
 */
int is_my_turn(input_t* server) {
  int my_turn;
  while(read(server->socket_fd, &my_turn, sizeof(int)) == -1) {
    // read failed, try again looping
  }
  return my_turn;
}

/**
 * Thread function to handle the UI updates and all the important high-level
 * events of the game.
 *
 * \param server_info - pointer to input_t struct that has communication info set up
 *                      for sending and recieving data from the game server. 
 */
void* ui_update(void* server_info) {
  input_t* server = (input_t*) server_info;
  game_t* game = malloc(sizeof(game_t));

  // update the UI until the main thread exits
  while(1) {
    // Get game data from the server
    get_game(server, game);

    // if game is over, end the game and the UI loop
    if(game->is_over) end_game();

    // show the game board 
    display_game(game);
    
    // (have function return server response??)
    if(is_my_turn(server)) {
      select_question(server, game);
    }

    // get the selected question from the server
    get_question(server);

    /*
      Everyone can buzz in and everyone can submit an answer if
      they buzzed in, but only the client who buzzed first will
      get the points for answering.
     */
    buzz_in(server); // buzz if you know the answer
    answer_question(server);
    
    // block until server responds with results of answering period 
    get_answers(server);
  }
  
  free(game);

  return NULL;
}


/**
 * The launching point of the game. Sets up communication with the game server and
 * begins the necessary threads for playing the game.
 *
 * \param argc - the number of command line inputs; must be 3
 * \param argv - command line input strings; must pass server name and the port it runs on
 * \return - the program exit status
 */
int main(int argc, char** argv) {
  if(argc != 4) {
    fprintf(stderr, "Usage: %s <username> <server name> <port>\n", argv[0]);
    exit(1);
  }
	
  // Read command line arguments
  my_username = argv[1]; 
  char* server_name = argv[2];
  unsigned short port = atoi(argv[3]);
	
  // Connect to the server
  int socket_fd = socket_connect(server_name, port);
  if(socket_fd == -1) {
    fprintf(stderr, "Failed to connect to game server\n");
    exit(2);
  }

  // Set up file streams
  FILE* to_server = fdopen(dup(socket_fd), "wb");
  if(to_server == NULL) {
    perror("Failed to open stream to server");
    exit(2);
  }
  
  FILE* from_server = fdopen(dup(socket_fd), "rb");
  if(from_server == NULL) {
    perror("Failed to open stream from server");
    exit(2);
  }

  // Init struct to contain server communication info
  input_t* server = (input_t*) malloc(sizeof(input_t));
  server->to = to_server;
  server->from = from_server;
  server->socket_fd = socket_fd;

  // Send username size to the server
  int len = strlen(my_username) + 1;
  while(write(socket_fd, &len, sizeof(int)) == -1) {
    //try again while failing
  }

  // Send the username to the server
  while(write(socket_fd, my_username, sizeof(char) * len) == -1) {
    //try again while failing
  }
  
  // Notify user that game has been joined
  wait_message();
  
  // Launch UI thread
  pthread_t ui_update_thread;
  pthread_create(&ui_update_thread, NULL, ui_update, server);

  // Block main thread while game is ongoing
  while(game_state) {}

  // Show game over screen
  game_over(server);

  // Close file streams
  fclose(to_server);
  fclose(from_server);
  
  // Close socket
  close(socket_fd);

  // Free malloced memory
  free(server);
	
  return 0;
}
