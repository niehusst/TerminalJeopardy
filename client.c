#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "socket.h"
#include "game_structs.h"

#define BOARD_WIDTH 80
#define BOARD_HEIGHT 20

// Global variable defining the state of the game as seen by user
enum game_status game_state = GAME_ONGOING;


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
void game_over(input_t* server) { //TODO
  // Read info on who has won and what final scores were
  
  // Show appropriate UI for end of game
  printf("%s has won the game!\n", ... );

  // Allow some time to see message before program terminates
  sleep(4);
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

/**
 * The "buzz-in" portion of the game; let the user provide input, and as soon as
 * they do, send the current system time to the server. Then, wait for the server's
 * response to see if this client was the first to buzz in.
 *
 * \return was_first - boolean, response from the server determination of whether or
 *                     not this client buzzed-in first.
 */
int buzz_in(input_t* server) {
  int was_first = 0;

  //TODO: getc from stdin and send to server, then get server response

  /*
    buzz + stdin respons
send to server

wait for server response
   */

  return was_first;
}



/**
 * Read all the data about the current state of the game from the server.
 *
 * \param server - communication info for the game server
 * \return board - the struct containing game data sent from the server  
 */
board_t* get_game(input_t* server) {//TODO
  //read the board struct from server
  board_t* board = (board_t*) malloc(sizeof(board_t));
  metadata_t* data = (metadata_t*) malloc(sizeof(metadata_t));
  int bytes_read = read(server->socket_fd, data, sizeof(metadata_t));
  //ensure the entire struct was read
  while(bytes_read < sizeof(board_t)) {
    bytes_read += read(server->socket_fd, data+bytes_read, sizeof(metadata_t)-bytes_read);
  }

  //copy data into board
  memcpy(board->answered, data->answered, sizeof(int)*NUM_QUESTIONS_PER_CATEGORY*NUM_CATEGORIES);
  memcpy(board->points, data->points, sizeof(int)*NUM_QUESTIONS_PER_CATEGORY*NUM_CATEGORIES);
  board->is_over = data->is_over;

  //read the category names
  for(int cat = 0; cat < NUM_CATEGORIES; cat++) {
    size_t size_of_cat = sizeof(char)*data->category_sizes[cat];
    char* cat_name = (char*)malloc(size_of_cat);
    int b_read = read(server->socket_fd, cat_name, size_of_cat);
    //check correct number of bytes were read from server
    if(b_read < size_of_cat) {
      //if failed, just fill in category name with a placeholder
      strncpy(cat_name, "???", size_of_cat);
    }

    //put cat name in board struct
    board->categories[cat] = cat_name;
  }

  return board;
}

/**
 * Get user input, allowing the user to select the question they 
 * want to answer. Send to the server so that other clients can receive
 * the same information.
 *
 * \param server - communication info for the game server
 */
void select_question(input_t* server) {//TODO
  //dont save time by returning chosen question so that all clients can get question at same time
}

/**
 * Get user input that is the answer to a displayed question. Send the
 * input answer to the server so it can be distributed to other clients.
 *
 * \param server - communication info for the game server
 */
void answer_question(input_t* server) {//TODO

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
 * Get the answer guessed by the client who buzzed in and the real answer
 * to the question from the server. Also get whether or not the answer
 * was correct from the server.
 *
 * \param server - communication info for the game server
 */
void get_answers(input_t* server) {//TODO
  //read from server
  //TODO: also send the username of the question answerer from the server

  //display correct answer and attempted answer w/ correctness to UI
  display_answers(...);
}

/**
 * Get the question coords that were selected by the client whose turn it was.
 * Blocks clients until the buzz-in period begins.
 *
 * \param server - communication info for the game server  
 */
void get_question(input_t* server) {//TODO
 
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
  
  // update the UI until the main thread exits
  while(1) {
    // Get game data from the server
    board_t* game = get_game(server);

    // Check if game is over
    if(game->is_over) end_game();

    // Display game board
    display_board(game);

    // If client's turn, select a question (have function return server response??)
    if( ... ) {//TODO
      select_question(server);
    }
    
    // wait for server confirmation of selected question
    get_question(server);

    // Buzz in period
    int first = buzz_in(server);

    // If buzzed in first, notify of Q answer opportunity
    if(first) {
      answer_question(server);
    } else {
      // Display "not your turn to answer"
      printf("Too slow! Not your turn to answer!\n");
    }
    
    // block until server responds with results of answering 
    get_answers(server);

    free(game); //malloced in get_game
  }

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
  if(argc != 3) {
    fprintf(stderr, "Usage: %s <server name> <port>\n", argv[0]);
    exit(1);
  }
	
  // Read command line arguments
  char* server_name = argv[1];
  unsigned short port = atoi(argv[2]);
	
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
  
  // Notify user that game has been joined
  wait_message();
  
  // Launch UI thread
  pthread_t ui_update_thread;
  pthread_create(&ui_update_thread, NULL, ui_update, &server);

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


















