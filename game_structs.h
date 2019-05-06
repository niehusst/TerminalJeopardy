#ifndef __GAME_STRUCTS__
#define __GAME_STRUCTS__
#include <stdio.h>
#include "deps/uthash.h"

#define NUM_QUESTIONS_PER_CATEGORY 5
#define NUM_CATEGORIES 5
#define MAX_NUM_PLAYERS 4
#define MAX_QUESTION_LENGTH 50
#define MAX_ANSWER_LENGTH 25

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
  char* username;
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
  char* name;
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
} game_t;

/**
 * Contains sizes of the strings required for sending answers to questions to a
 * client from the server.
 */
typedef struct answer_sizes {
  int pred_ans_size;
  int real_ans_size;
  int user_size;
} answer_sizes_t;


#endif
