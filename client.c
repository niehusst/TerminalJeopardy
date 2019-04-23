#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "socket.h"
//#include "game_structs.h"

#define GAME_ONGOING 1
#define GAME_OVER 0
#define BOARD_WIDTH 80
#define BOARD_HEIGHT 20

// Global variable defining the state of the game as seen by user
int game_state = GAME_ONGOING;

/**
 * Print a reassuring message to stdin to tell them they have connected 
 * before the game starts.
 */
void wait_message() {
  printf("You've joined the game!\nWaiting for all 4 people to join the game...\n");
}

/**
 * Set the game_state global to end the game, and cause the main thread
 * to begin its exit procedures.
 */
void end_game() {
  game_state = GAME_OVER;
}

/**
 * Inform the user that the game has officially ended, and display their final
 * score as well as the name of the winner of the game.
 */
void game_over() {
  // Show appropriate UI for end of game
  printf("Somebody has one the game!\n");

  // Allow some time to see it before program terminates
  sleep(4);
}

char* get_board_vals(game_t* game_data) {
  char* question_done = "XXX";

  int num_questions = 5;
  int num_categories = 5;
  char** question_point_values = (char**)malloc(sizeof(char*)*(num_categories*num_questions));
  
  for(int cat = 0; cat < num_categories; cat++) {
    for(int q = 0; q < num_questions; q++) {
      square_t cur_question = game_data->categories[cat]->questions[q];
      int num_size = 4;
      char points[num_size];
      
      //get question point value as a string
      int written = snprintf(points, num_size, "%d", cur_question.value);
      if(written < 0 || written >= 4) {
        printf("oops!!\n");
        strncpy(work, q, 4);
      }

      //assign value if question hasn't been answerd
      question_point_values[cat*num_categories + q] = cur_question.isanswered ? question_done : points;
    }
  }
  
  return question_point_values;
}

/**
 * Display the game board to the user with only valid, availble questions 
 * appearing on it.
 *
 * \param board - a game_t struct that contains information about the baord
 */
void display_board(game_t* game_data) {
  char* category1 = "c1";
  char* category2 = "c2";
  char* category3 = "c3";
  char* category4 = "c4";
  char* category5 = "c5";
  int buffer_space = 13;
  char** point_vals = get_board_vals(game_data);
  
  printf("+------------------------------------------------------------------------------+\n| %*s | %*s | %*s | %*s | %*s |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n|      %s      |      %s      |      %s      |      %s      |      %s     |\n+---------------+---------------+---------------+---------------+--------------+\n", buffer_space, category1, buffer_space, category2, buffer_space, category3, buffer_space, category4, buffer_space-1, category5, point_vals[0], point_vals[1], point_vals[2], point_vals[3], point_vals[4], point_vals[5], point_vals[6], point_vals[7], point_vals[8], point_vals[9], point_vals[10], point_vals[11], point_vals[12], point_vals[13], point_vals[14], point_vals[15], point_vals[16], point_vals[17], point_vals[18], point_vals[19], point_vals[20], point_vals[21], point_vals[22], point_vals[23], point_vals[24]);
}

/**
 * The "buzz-in" portion of the game; let the user provide input, and as soon as
 * they do, send the current system time to the server. Then, wait for the server's
 * response to see if this client was the first to buzz in.
 *
 * \return was_first - boolean, response from the server determination of whether or
 *                     not this client buzzed-in first.
 */
int buzz_in(int server_fd) {
  int was_first = 0;

  //TODO: getc from stdin and send to server, then get server response

  return was_first;
}

game_t* get_game(input_t* server) {

}

void select_question(input_t* server) {

}

void answer_question(input_t* server) {

}

void display_answers(char* real, char* predicted) {

}

void get_correct_answer(input_t* server) {
  //read from server

  //display correct answer and attempted answer to UI
  display_answers(...);
}


/**
 * Thread funciton to handle input from the user. Handle the imnput this way??
 */
void* input_handler(void* arg) {
  return NULL;
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
    game_t* game = get_game(server);

    // Check if game is over
    if(game->is_over) end_game();

    // Display game board
    display_board(game);

    // If client's turn, select a question (have function return server response??)
    if( ... ) {
      select_question(server);
    }

    // Buzz in period
    int first = buzz_in(server);

    // If buzzed in first, notify of Q answer opportunity
    if(first) {
      answer_question(server);
    } 
    // otherwise block until server response with answer result 
    get_correct_answer(server);
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
  
  // Launch game threads
  pthread input_thread;
  pthread ui_update_thread;

  pthread_create(&input_thread, NULL, input_handler, NULL);
  pthread_create(&ui_update_thread, NULL, ui_update, &server);

  // Block main thread while game is ongoing
  while(game_state) {}

  // Show game over screen
  game_over();

  // Close file streams
  fclose(to_server);
  fclose(from_server);
  
  // Close socket
  close(socket_fd);
	
  return 0;
}


















