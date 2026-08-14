#include "app/save.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

/* Function: save_game
 * The save_game function saves the current state of a chess game to a binary file.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - p_captures_white_head: Pointer to the head of the linked list of captured white pieces.
 * - p_captures_black_head: Pointer to the head of the linked list of captured black pieces.
 * - p_history_head: Pointer to the head of the linked list of move history.
 * - moves: The number of moves made in the game.
 *
 * The function performs the following steps:
 * 1. Opens a binary file for writing. If the file cannot be opened, prints an error message and returns 0.
 * 2. Writes the number of moves to the file.
 * 3. Writes the board state to the file.
 * 4. Writes the captured white pieces to the file, followed by an end marker.
 * 5. Writes the captured black pieces to the file, followed by an end marker.
 * 6. Writes the move history to the file, followed by an end marker.
 * 7. Closes the file and prints a success message.
 * 8. Returns 1 to indicate successful saving.
 */
int save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves) {
  FILE *file = fopen("game_save.bin", "wb");
  if (file == NULL) {
    wprintf(L"Error opening file for saving.\n");
    return 0;
  }

  // Save the number of moves
  fwrite(&moves, sizeof(int), 1, file);

  // Save the board
  fwrite(board, sizeof(Piece_t), 64, file);

  // Save the captures for white
  Captures_node_t *current_capture = p_captures_white_head;
  while (current_capture != NULL) {
    fwrite(&current_capture->piece, sizeof(Piece_t), 1, file);
    current_capture = current_capture->p_next;
  }
  Piece_t end_marker = {L'\0', NONE, "", FREE}; // End marker for captures
  fwrite(&end_marker, sizeof(Piece_t), 1, file);

  // Save the captures for black
  current_capture = p_captures_black_head;
  while (current_capture != NULL) {
    fwrite(&current_capture->piece, sizeof(Piece_t), 1, file);
    current_capture = current_capture->p_next;
  }
  fwrite(&end_marker, sizeof(Piece_t), 1, file);

  // Save the move history
  History_node_t *current_history = p_history_head;
  while (current_history != NULL) {
    fwrite(current_history, sizeof(History_node_t), 1, file);
    current_history = current_history->p_next;
  }
  History_node_t end_history_marker = {"", "", NULL}; // End marker for history
  fwrite(&end_history_marker, sizeof(History_node_t), 1, file);

  fclose(file);
  wprintf(L"Game saved successfully.\n");

  return 1;
}

/* Function: load_game
 * The load_game function loads the state of a chess game from a binary file.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - p_captures_white_head: Double pointer to the head of the linked list of captured white pieces.
 * - p_captures_black_head: Double pointer to the head of the linked list of captured black pieces.
 * - p_history_head: Double pointer to the head of the linked list of move history.
 * - moves: Pointer to the integer tracking the number of moves made.
 *
 * The function performs the following steps:
 * 1. Opens a binary file for reading. If the file cannot be opened, prints an error message and returns 0.
 * 2. Reads the number of moves from the file.
 * 3. Reads the board state from the file.
 * 4. Reads the captured white pieces from the file and reconstructs the linked list.
 * 5. Reads the captured black pieces from the file and reconstructs the linked list.
 * 6. Reads the move history from the file and reconstructs the linked list.
 * 7. Closes the file and returns 1 to indicate successful loading.
 */
int load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves) {
  FILE *file = fopen("game_save.bin", "rb");
  if (file == NULL) {
    wprintf(L"Error opening file for loading.\n");
    return 0;
  }

  // Load the number of moves
  fread(moves, sizeof(int), 1, file);

  // Load the board
  fread(board, sizeof(Piece_t), 64, file);

  // Load the captures for white
  Captures_node_t *current_capture = NULL;
  Piece_t piece;
  while (fread(&piece, sizeof(Piece_t), 1, file) && piece.type != FREE) {
    Captures_node_t *new_capture = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    new_capture->piece = piece;
    new_capture->p_next = NULL;
    if (*p_captures_white_head == NULL) {
      *p_captures_white_head = new_capture;
    } else {
      current_capture->p_next = new_capture;
    }
    current_capture = new_capture;
  }

  // Load the captures for black
  current_capture = NULL;
  while (fread(&piece, sizeof(Piece_t), 1, file) && piece.type != FREE) {
    Captures_node_t *new_capture = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    new_capture->piece = piece;
    new_capture->p_next = NULL;
    if (*p_captures_black_head == NULL) {
      *p_captures_black_head = new_capture;
    } else {
      current_capture->p_next = new_capture;
    }
    current_capture = new_capture;
  }

  // Load the move history
  History_node_t *current_history = NULL;
  History_node_t history_node;
  while (fread(&history_node, sizeof(History_node_t), 1, file) && history_node.prev_pos[0] != '\0') {
    History_node_t *new_history = (History_node_t *)malloc(sizeof(History_node_t));
    *new_history = history_node;
    new_history->p_next = NULL;
    if (*p_history_head == NULL) {
      *p_history_head = new_history;
    } else {
      current_history->p_next = new_history;
    }
    current_history = new_history;
  }

  fclose(file);
  wprintf(L"Game loaded successfully.\n");

  return 1;
}
