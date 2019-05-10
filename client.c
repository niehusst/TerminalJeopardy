#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "deps/socket.h"
#include "game_structs.h"

/*
chmod a+rx /home/niehusst/os213/project/
chmod a+rx /home/niehusst/os213/project/client
 */

// Global variable defining the state of the game as seen by user
enum game_status game_state = GAME_ONGOING;

int my_id;
char* my_username;

/**
 * Print a reassuring message to stdin to tell them they have connected 
 * before the game starts.
 */
void wait_message() {
  printf("You've joined the game!\nWaiting for all %d people to join the game...\n", MAX_NUM_PLAYERS);
}


/**
 * Inform the user that the game has officially ended, and display their final
 * score as well as the name of the winner of the game.
 *
 * \param server - communication info for the game server
 */
void game_over(game_t* game) { 
  // determine who was the winner (max score)
  int max = 0; //index of player with max score
  for(int score = 1; score < MAX_NUM_PLAYERS; score++) {
    if(game->players[max].score < game->players[score].score) {
      max = score;
    }
  }
  char* winner = game->players[max].name;
  
  // Show appropriate UI for end of game
  printf("\n%s has won the game!\n", winner);
  //print all scores and usernames
  for(int player = 0; player < MAX_NUM_PLAYERS; player++) {
    printf("Player %s scored: %d\n", game->players[player].name, game->players[player].score);
  }

  // Allow user to determine when they are done looking at the message
  printf("(Press Enter to exit)\n");
  getchar(); // wait until user input to finish
}

/**
 * Set the game_state global to end the game, and cause the main thread
 * to begin its exit procedures. Also send game-over notification and 
 * final scores to the UI so user knows game results.
 *
 * \param game - all information about the final state of the game
 */
void end_game(game_t* game) {
  // Show game over screen
  game_over(game);

  // set the game as over so that the program exit sequence will begin
  game_state = GAME_OVER;
}

/**
 * Get the list of category titles (truncated to maximum size if necessary).
 * The category title can be up to 26 characters across 2 rows of the UI.
 * 
 * \param game_data - data about the entire game, including question values
 * \return cats - string array of category titles
 */
char** get_categories(game_t* game_data) {
  /*
    TODO: make this do line wrap for 2 rows WITHOUT CRASHING

for some reason the first title of first row is pooP?? but then it fixes itself?? 
   */
  // double size of num_categories for twice the row space
  // for cat names
  char** cats = (char**) malloc(sizeof(char*)*2*NUM_CATEGORIES);
  // limit to size of portion of title that can fit in 1 row
  int max_title_len = 14; 
  
  for(int cat = 0; cat < NUM_CATEGORIES; cat++) {
    // check if this title needs to be split across 2 rows
    if(strlen(game_data->categories[cat].title) >= max_title_len) {
      // the title is too long to fit on one line, so split
      // in multiple rows

      // get the first part of the title
      char* title = (char*) malloc(sizeof(char)*max_title_len);
      int limit = cat==NUM_CATEGORIES-1 ? max_title_len-2 : max_title_len-1;
      title = strndup(game_data->categories[cat].title,sizeof(char)*limit);

      // write null terminator to cut in half title
      //title[strlen(title)+1] = '\0';

      // save first part of title on first row
      cats[cat] = title;
      // save second part of title on the second row
      cats[cat+NUM_CATEGORIES] = strndup(game_data->categories[cat].title+limit,sizeof(char)*limit);

    } else {
      // the title can fit in one line! 
      // save the title on the first line
      cats[cat] = strdup(game_data->categories[cat].title);
      // write empty string to second line
      cats[cat+NUM_CATEGORIES] = "";
    }  
  }
  
  return cats;
}

/**
 * Extract point values to display on the Jeopardy! game board UI from the 
 * game data sent from the server. Return extracted data
 *
 * \param game_data - data about the entire game, including question values
 * \return question_point_values - string array of point values and crossed
 *                                 out completed questions
 */
char** get_board_vals(game_t* game_data) {
  char* question_done = "XXXX";

  int num_questions = NUM_QUESTIONS_PER_CATEGORY;
  int num_categories = NUM_CATEGORIES;
  char** question_point_values = malloc(sizeof(char*)*(num_categories*num_questions));

  //points string can hold up to 4 digits and null terminator
  int num_size = 5;

  // loop through all questions, parsing values into a string array
  for(int cat = 0; cat < num_categories; cat++) {
    for(int q = 0; q < num_questions; q++) {
      // string to hold the point value of the question 
      char* points = (char*) malloc(sizeof(char)*num_size);
      
      //get question point value as a string
      int written = snprintf(points, num_size, "%d", game_data->categories[cat].questions[q].value);
      //if snprintf fails, just put question_done as a placeholder
      if(written < 0) {
        strncpy(points, question_done, num_size);
      }

      //assign value if question hasn't been answerd
      int already_answered = game_data->categories[cat].questions[q].is_answered;

      if(already_answered) {
        question_point_values[cat*num_categories + q] = question_done;
      } else {
        question_point_values[cat*num_categories + q] = points;
      }
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
void display_board(game_t* game_data) {
  int buffer_space = 13;
  //get category names of truncated length
  char** categories = get_categories(game_data);
  char** point_vals = get_board_vals(game_data);
  
  printf("+------------------------------------------------------------------------------+\n| %*s | %*s | %*s | %*s | %*s |\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n",
         // print the category titles
         buffer_space, categories[0],
         buffer_space, categories[1],
         buffer_space, categories[2],
         buffer_space, categories[3],
         buffer_space-1, categories[4],
         buffer_space, categories[5],
         buffer_space, categories[6],
         buffer_space, categories[7],
         buffer_space, categories[8],
         buffer_space-1, categories[9],
         // print the values
         buffer_space, point_vals[0],
         buffer_space, point_vals[5],
         buffer_space, point_vals[10],
         buffer_space, point_vals[15],
         buffer_space-1, point_vals[20],
         buffer_space, point_vals[1],
         buffer_space, point_vals[6],
         buffer_space, point_vals[11],
         buffer_space, point_vals[16],
         buffer_space-1, point_vals[21],
         buffer_space, point_vals[2],
         buffer_space, point_vals[7],
         buffer_space, point_vals[12],
         buffer_space, point_vals[17],
         buffer_space-1, point_vals[22],
         buffer_space, point_vals[3],
         buffer_space, point_vals[8],
         buffer_space, point_vals[13],
         buffer_space, point_vals[18],
         buffer_space-1, point_vals[23],
         buffer_space, point_vals[4],
         buffer_space, point_vals[9],
         buffer_space, point_vals[14],
         buffer_space, point_vals[19],
         buffer_space-1, point_vals[24]);

  // clean up
  //free(categories); // TODO: this causes fuckery
  free(point_vals);
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
 * \return buzz_time - the system time when user buzzed in, or
 *                     -1 if the client did not buzz in
 */
time_t buzz_in() {
  time_t buzz_time;
  int time_out = 4; //seconds to wait before timed_getchar exits
  
  printf("Buzz in if you know the answer! (Hit enter)\n");
  // blocking IO call (with timeout) to hold back client until
  // response or time-out
  if(timed_getchar(time_out)) {
    //getchar(); //consume any commandline input
    //get system time of buzz in
    buzz_time = time(NULL);
  } else {
    //client didn't buzz in, set buzz_time to specific value
    buzz_time = -1;
  }
  getchar();
  return buzz_time;
}



/**
 * Read all the data about the current state of the game from the server.
 *
 * \param server - communication info for the game server
 * \param game - the struct to write the read game into
 * \return game - the struct containing game data sent from the server  
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
int choice_valid(char* coords, game_t* board) {
  int validity = 0;
  // extract numeric coords within range of 0-4 
  int col = coords[0] - 'A';      //range A-E
  int row = coords[1] - '0' - 1;  //range 1-5
  
  // check coords are within range
  if(row > 4 || row < 0 || col > 4 || col < 0) return validity;
  
  // check question hasn't already been answered
  if(!board->categories[col].questions[row].is_answered) {
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
 * \param game - all data about the current state of the game
 */
void select_question(input_t* server, game_t* game) {
  //read client question choice; input must take coordinate form
  //    letter row, number column (A1 through E5)
  int coord_size = 3;
  char coords[coord_size+1];
  
  printf("It's your turn to pick the question. What question do you choose?\n");
  printf("(Choice must be in coordinate form: letter column, number row (e.g. E5))\n");
  while(fgets(coords, coord_size, stdin) == NULL || !choice_valid(coords, game)) {
    //read failed
    printf("Unfortunately, that is not a valid choice.\nPlease pick a different question.\n");
    //consume whitespace from invalid coord choice
    while(getchar() != '\n');
  }

  // send coords to server (until success)
  while(write(server->socket_fd, coords, sizeof(char)*coord_size+1) == -1) {}

}

/**
 * Get user input that is the answer to a displayed question. Return the
 * answer obtained from stdin.
 *
 * \return answer - string, the user input answer to the displayed question
 */
char* answer_question() {
  char* answer = malloc(sizeof(char)*MAX_ANSWER_LENGTH);

  printf("Thanks for buzzing in, %s.\nWhat is your answer?\n", my_username);

  // consume whitespace leftover in stdin
  while((getchar()) != '\n');
  
  // get client's answer from standard input
  while(fgets(answer, MAX_ANSWER_LENGTH, stdin) == NULL) {
    // error getting input
    printf("Sorry, we didn't quite catch that. What was your answer?\n");
  }
  
  return answer;
}

/**
 * Display the real and guessed answers to the ~beautiful~ UI.
 *
 * \param game - contains all current info about the game state
 * \param ans - contains all the information about the results of the
 *              most recent buzz/answer period
 */
void display_answers(game_t* game, answer_t* ans) {
  // if anybody answered correctly, show their username
  if(ans->did_answer) {
    // search through players in game struct for the username of answerer id
    char* answerer;
    for(int player = 0; player < MAX_NUM_PLAYERS; player++) {
      if(game->players[player].id == ans->id) {
        answerer = game->players[player].name;
      }
    }
    
    if(ans->id != my_id) {
      printf("Sorry, %s, looks like you didn't buzz in quick enough.\n", my_username);
    }
    
    printf("%s buzzed in first and said:\n%s\n\n", answerer, ans->answer);
    printf("That was absolutely right!\n");
  } else {
    // nobody buzzed in/nobody answered correctly
    printf("It appears nobody got the answer right. The correct answer was:\n%s\n",ans->answer);
  }
}

/**
 * Get the answer guessed by the client who buzzed in, the real answer
 * to the question, whether or not the answer was correct, and the 
 * answerer's username from the server.
 *
 * \param server - communication info for the game server
 */
void get_answers(input_t* server, game_t* game) {
  answer_t* correct_ans = malloc(sizeof(answer_t));
  
  //read question answer info from server
  if(read(server->socket_fd, correct_ans, sizeof(answer_t)) < sizeof(answer_t)) {
    perror("reading correct answers failed");
  }
  
  //display correct answer and attempted answer w/ correctness to UI
  display_answers(game, correct_ans);

  free(correct_ans);
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
 * \param game - all information about the current state of the game  
 */
void get_question(input_t* server, game_t* game) {
  // read question coordinates
  int coord_size = 3; //2 coord chars, null char
  char* q_coords = malloc(sizeof(char)*coord_size);

  if(read(server->socket_fd, q_coords, sizeof(char)*coord_size) <
     sizeof(char)*coord_size) {
    // read failed, try again looping
    perror("read less than expected\n");
  }

  // parse question string from received coords
  int col = q_coords[0] - 'A';      //range A-E
  int row = q_coords[1] - '0' - 1;  //range 1-5
  char* question = game->categories[col].questions[row].question;

  // free location allocated for now useless coords
  free(q_coords);
  // show question on UI
  display_question(question);
}

/**
 * Check the game state for the id of the client whose turn it currently is.
 *
 * \param game - contains all info about current game state
 * \return - boolean, True if it is the client's turn, False otherwise
 */
int is_my_turn(game_t* game) {
  return my_id == game->id_of_player_turn;
}

/**
 * Display on the UI the latest scores for all the players.
 *
 * \param game - contains all info about current game state
 */
void score_update(game_t* game) {
  printf("\n| CURRENT SCORES:\n");
  //print all scores and usernames
  for(int player = 0; player < MAX_NUM_PLAYERS; player++) {
    printf("| %s: %d\n", game->players[player].name, game->players[player].score);
  }
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
    if(game->is_over) {
      end_game(game);
      break;
    }

    // Show latest update of scores
    score_update(game);
    
    // show the game board 
    display_board(game); //TODO: make this show more of category names
    
    // if it is the clients turn, have them select the question
    if(is_my_turn(game)) {
      select_question(server, game);
    }
    
    // get the selected question from the server
    get_question(server, game);

    // provide some time for players to read the question
    sleep(3);

    /*
      Everyone can buzz in and everyone can submit an answer if
      they buzzed in (regardless of buzz order), but only the
      client who buzzed first will get the points for answering.
     */
    answer_t ans; // save data from buzz/answer period
    time_t buzz_time = buzz_in();
    // check if client buzzed in
    if(buzz_time != -1) {
      // copy the result of answer_question into answer array
      strncpy(ans.answer, answer_question(), MAX_ANSWER_LENGTH);
    } else {
      printf("Too late to buzz in!\n");
    }

    // send buzz/answer period data to server
    ans.did_answer = buzz_time != -1;
    ans.time = buzz_time;
    
    if(write(server->socket_fd, &ans, sizeof(answer_t)) < sizeof(answer_t)) {
      perror("Writing answer to server failed\n");
    }
    
    // block until server responds with results of answering period 
    get_answers(server, game);
    
    // provide a few moments for the user to read the scores
    sleep(3);
  }
  
  // clean up
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
  int len = MAX_ANSWER_LENGTH < (strlen(my_username) + 1) ? MAX_ANSWER_LENGTH : (strlen(my_username) + 1);
  while(write(socket_fd, &len, sizeof(int)) == -1) {
    //try again while failing
  }

  // Send the username to the server
  while(write(socket_fd, my_username, sizeof(char) * len) == -1) {
    //try again while failing
  }

  // Get your user number back from server
  while(read(socket_fd, &my_id, sizeof(int)) == -1) {
    //try again while failing
  }
  
  // Notify user that game has been joined
  wait_message();
  
  // Launch UI thread
  pthread_t ui_update_thread;
  pthread_create(&ui_update_thread, NULL, ui_update, server);

  // Block main thread while game is ongoing
  while(game_state) {}

  // Close file streams
  fclose(to_server);
  fclose(from_server);
  
  // Close socket
  close(socket_fd);

  // Free malloced memory
  free(server);
	
  return 0;
}

/*
TODO:
receipts for succesfully sent answer
 */
