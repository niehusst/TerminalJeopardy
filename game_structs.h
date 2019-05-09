#ifndef __GAME_STRUCTS__
#define __GAME_STRUCTS__
#include <stdio.h>
#include <time.h>
#include "deps/uthash.h"

#define NUM_QUESTIONS_PER_CATEGORY 5
#define NUM_CATEGORIES 5
#define MAX_NUM_PLAYERS 1
#define MAX_QUESTION_LENGTH 300
#define MAX_ANSWER_LENGTH 40

// Definitions for the run status of the game
enum game_status{GAME_OVER = 0, GAME_ONGOING = 1};

/**
 * All data necessary for communicating with a machine over a network
 * with the C POSIX TCP API
 */
typedef struct input{
  FILE* from;
  FILE* to;
  int socket_fd;
  int id;
}input_t;

/**
 * Info and metadata for a single question (contained within a square 
 * on the game board).
 */
typedef struct square{
  int value;
  int is_answered;
  char question[MAX_QUESTION_LENGTH];
  char answer[MAX_ANSWER_LENGTH];
} square_t;

/**
 * A category's title and list of questions (and their metadata)
 */
typedef struct category{
  square_t questions[NUM_QUESTIONS_PER_CATEGORY];
  char title[MAX_ANSWER_LENGTH];
  int num_questions;
  UT_hash_handle hh;
}category_t;

/**
 * Information about a player; useful for display information
 */
typedef struct player{
  char name[MAX_ANSWER_LENGTH];
  int id;
  int score;
} player_t;

/**
 * Contains all the info on players and questions and the status of the game
 */
typedef struct game{
  int is_over;
  category_t categories[NUM_CATEGORIES];
  player_t players[MAX_NUM_PLAYERS];
  int num_players;
  int id_of_player_turn;
} game_t;

/**
 * Contains information on buzz in time and an answer to a question (if they
 * did buzz in and they did answer). Also has linked list capability for server
 * to keep track of all the answers from clients in the question answering 
 * period.
 */
typedef struct answer {
  time_t time;
  char answer[MAX_ANSWER_LENGTH];
  int did_answer; // boolean
  int id;
  struct answer* next;
} answer_t;

/**
 * Contains the information for 
 */

#endif
