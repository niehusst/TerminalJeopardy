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

/**
 * Display the game board to the user with only valid, availble questions 
 * appearing on it.
 *
 * \param board - a game_t struct that contains information about the baord
 */
void display_board(game_t* board) {
  //TODO: print a pretty board
}

/**
 * Thread funciton to handle input from the user. Handle the imnput this way??
 */
void* input_handler(void* arg) {

}

/**
 * Thread function to handle the UI updates and all the important high-level
 * events of the game.
 *
 * \param arg - something? maybe the server socket to communicate with
 */
void* ui_update(void* arg) {
  while(1) {
    // Get game data from the server

    // Check if game is over
    if(0) end_game();

    // Display game board

    // If client's turn, select a question (have function return server response)

    // Buzz in period
    int first = buzz_in();

    // If buzzed in first, notify of Q answer opportunity
    if(first) {

    } 
     // otherwise block until server response with answer result 
    get_correct_answer();
  }
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
  
  // Notify user that game has been joined
  wait_message();

  // Launch game threads
  pthread input_thread;
  pthread ui_update_thread;

  pthread_create(&input_thread, NULL, input_handler, NULL);
  pthread_create(&ui_update_thread, NULL, ui_update, NULL); //TODO: give parameter

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


















