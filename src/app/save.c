#include "app/save.h"

#include "core/history.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* The file is a raw dump of the in-memory structs, so its layout is whatever
 * Piece_t and History_node_t happen to be. A magic number and a version make
 * that dependency explicit: when the structs change, the version changes, and
 * files written by the old layout are refused instead of being reinterpreted
 * as the new one.
 *
 * Version 1 is the original unversioned format, which had no header at all and
 * so fails the magic check. Version 2 is the current layout, written after
 * Piece_t lost its icon and position fields.
 */
#define SAVE_PATH "game_save.bin"
#define SAVE_MAGIC "CCHS"
#define SAVE_MAGIC_LEN 4
#define SAVE_VERSION 2u

/* Function: save_game
 * The save_game function saves the current state of a chess game to a binary file.
 *
 * Parameters:
 * - p_state: The game to write. Not modified.
 *
 * The function performs the following steps:
 * 1. Opens a binary file for writing. If the file cannot be opened, prints an error message and returns 0.
 * 2. Writes the magic number and format version.
 * 3. Writes the number of moves and the board state.
 * 4. Writes the captured white pieces, then the captured black pieces, each followed by an end marker.
 * 5. Writes the move history, followed by an end marker.
 * 6. Checks the stream for errors, closes the file and prints a success message.
 * 7. Returns 1 to indicate successful saving.
 */
int save_game(const GameState *p_state) {
  FILE *file = fopen(SAVE_PATH, "wb");
  if (file == NULL) {
    wprintf(L"Error opening file for saving.\n");
    return 0;
  }

  // Identify the format before anything that depends on struct layout
  uint32_t version = SAVE_VERSION;
  fwrite(SAVE_MAGIC, 1, SAVE_MAGIC_LEN, file);
  fwrite(&version, sizeof(version), 1, file);

  // Save the number of moves
  fwrite(&p_state->moves, sizeof(int), 1, file);

  // Save the board
  fwrite(p_state->board, sizeof(Piece_t), 64, file);

  // Save the captures for white
  const Captures_node_t *current_capture = p_state->p_captures_white_head;
  while (current_capture != NULL) {
    fwrite(&current_capture->piece, sizeof(Piece_t), 1, file);
    current_capture = current_capture->p_next;
  }
  Piece_t end_marker = {NONE, FREE}; // End marker for captures
  fwrite(&end_marker, sizeof(Piece_t), 1, file);

  // Save the captures for black
  current_capture = p_state->p_captures_black_head;
  while (current_capture != NULL) {
    fwrite(&current_capture->piece, sizeof(Piece_t), 1, file);
    current_capture = current_capture->p_next;
  }
  fwrite(&end_marker, sizeof(Piece_t), 1, file);

  // Save the move history
  const History_node_t *current_history = p_state->p_history_head;
  while (current_history != NULL) {
    fwrite(current_history, sizeof(History_node_t), 1, file);
    current_history = current_history->p_next;
  }
  History_node_t end_history_marker = {"", "", NULL}; // End marker for history
  fwrite(&end_history_marker, sizeof(History_node_t), 1, file);

  // One check covers every write above: the stream latches its error flag.
  if (ferror(file) || fclose(file) != 0) {
    wprintf(L"Error writing save file. The game was not saved.\n");
    return 0;
  }

  wprintf(L"Game saved successfully.\n");

  return 1;
}

/* Function: load_game
 * The load_game function loads the state of a chess game from a binary file.
 *
 * Parameters:
 * - p_state: The game to load into. Overwritten on success, untouched on failure.
 *
 * The function performs the following steps:
 * 1. Opens a binary file for reading. If the file cannot be opened, prints an error message and returns 0.
 * 2. Checks the magic number and format version, refusing anything it does not recognise.
 * 3. Reads the move count and the board state.
 * 4. Reads the captured white pieces, the captured black pieces and the move history, rebuilding each linked list.
 * 5. Commits the result to p_state and returns 1.
 *
 * Every read is checked. A file that ends early is rejected rather than
 * half-loaded: the work happens on a local GameState, and p_state is only
 * written once the whole file has been read successfully.
 */
int load_game(GameState *p_state) {
  FILE *file = fopen(SAVE_PATH, "rb");
  if (file == NULL) {
    wprintf(L"Error opening file for loading.\n");
    return 0;
  }

  // Check the magic number and the version before trusting any struct layout
  char magic[SAVE_MAGIC_LEN];
  uint32_t version = 0;
  if (fread(magic, 1, SAVE_MAGIC_LEN, file) != SAVE_MAGIC_LEN || memcmp(magic, SAVE_MAGIC, SAVE_MAGIC_LEN) != 0 || fread(&version, sizeof(version), 1, file) != 1 || version != SAVE_VERSION) {
    fclose(file);
    wprintf(L"Save file was written by an older version and cannot be loaded.\n");
    return 0;
  }

  GameState loaded = {0};
  Captures_node_t *current_capture = NULL;
  History_node_t *current_history = NULL;
  Piece_t piece;
  History_node_t history_node;

  // Load the number of moves
  if (fread(&loaded.moves, sizeof(int), 1, file) != 1) {
    goto truncated;
  }

  // Load the board
  if (fread(loaded.board, sizeof(Piece_t), 64, file) != 64) {
    goto truncated;
  }

  // Load the captures for white
  for (;;) {
    if (fread(&piece, sizeof(Piece_t), 1, file) != 1) {
      goto truncated;
    }
    if (piece.type == FREE) {
      break; // End marker
    }
    Captures_node_t *new_capture = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    if (new_capture == NULL) {
      goto truncated;
    }
    new_capture->piece = piece;
    new_capture->p_next = NULL;
    if (loaded.p_captures_white_head == NULL) {
      loaded.p_captures_white_head = new_capture;
    } else {
      current_capture->p_next = new_capture;
    }
    current_capture = new_capture;
  }

  // Load the captures for black
  current_capture = NULL;
  for (;;) {
    if (fread(&piece, sizeof(Piece_t), 1, file) != 1) {
      goto truncated;
    }
    if (piece.type == FREE) {
      break; // End marker
    }
    Captures_node_t *new_capture = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    if (new_capture == NULL) {
      goto truncated;
    }
    new_capture->piece = piece;
    new_capture->p_next = NULL;
    if (loaded.p_captures_black_head == NULL) {
      loaded.p_captures_black_head = new_capture;
    } else {
      current_capture->p_next = new_capture;
    }
    current_capture = new_capture;
  }

  // Load the move history
  for (;;) {
    if (fread(&history_node, sizeof(History_node_t), 1, file) != 1) {
      goto truncated;
    }
    if (history_node.prev_pos[0] == '\0') {
      break; // End marker
    }
    History_node_t *new_history = (History_node_t *)malloc(sizeof(History_node_t));
    if (new_history == NULL) {
      goto truncated;
    }
    *new_history = history_node;
    new_history->p_next = NULL;
    if (loaded.p_history_head == NULL) {
      loaded.p_history_head = new_history;
    } else {
      current_history->p_next = new_history;
    }
    current_history = new_history;
  }

  fclose(file);

  // Nothing can fail from here on, so it is safe to hand the game over
  *p_state = loaded;
  wprintf(L"Game loaded successfully.\n");

  return 1;

truncated:
  fclose(file);
  free_captures(loaded.p_captures_white_head);
  free_captures(loaded.p_captures_black_head);
  free_history(loaded.p_history_head);
  wprintf(L"Save file is incomplete or corrupted and cannot be loaded.\n");

  return 0;
}
