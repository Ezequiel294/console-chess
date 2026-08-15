#include "app/game.h"

#include "core/board.h"
#include "ui/display.h"
#include "core/history.h"
#include "core/rules.h"
#include "app/save.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

/* Function: game_loop
 * The game_loop function manages the main loop of a chess game. It alternates turns between white and black players, updating the board and capturing pieces.
 *
 * Parameters:
 * - p_state: The game in progress. Mutated in place.
 *
 * The function performs the following steps:
 * 1. Initializes variables for tracking if the king is captured and for saving the game.
 * 2. Enters a loop that continues until a king is captured.
 * 3. Alternates turns between white and black players based on the move count.
 * 4. Prints the board and captures, prompts the current player for their move, and updates the board.
 * 5. Every 5 moves, prompts the player to save the game.
 * 6. Clears the screen after each move.
 * 7. If a king is captured, declares the winner and prints the final board and move history.
 */
void game_loop(GameState *p_state) {
  int captured_king = 0;
  wchar_t save_choice;

  do {
    if (p_state->moves % 2 != 0) {
      print_board_white(p_state->board);
      wprintf(L"\n");
      print_captures(p_state->p_captures_white_head);
      print_captures(p_state->p_captures_black_head);
      wprintf(L"\nWhite's turn\n");
      get_move(p_state, &captured_king);
      wprintf(L"\033[H\033[2J\033[3J");
      print_board_white(p_state->board);
    } else {
      print_board_black(p_state->board);
      wprintf(L"\n");
      print_captures(p_state->p_captures_white_head);
      print_captures(p_state->p_captures_black_head);
      wprintf(L"\nBlack's turn\n");
      get_move(p_state, &captured_king);
      wprintf(L"\033[H\033[2J\033[3J");
      print_board_black(p_state->board);
    }

    p_state->moves++;

    if (!captured_king && p_state->moves % 5 == 0) {
      // Ask the player if they want to save the game
      wprintf(L"\nDo you want to save the game? (y/n): ");
      wscanf(L" %lc", &save_choice);
      while (getwchar() != '\n')
        ; // Clear the input buffer
      if (save_choice == L'y' || save_choice == L'Y') {
        save_game(p_state);
        exit(0);
      } else {
        wprintf(L"\033[H\033[2J\033[3J");
      }
    }

    if (p_state->moves % 5 != 0 && !captured_king) {
      // Hold the finished move on screen long enough for the player who made
      // it to see it before the board flips to the other side.
      sleep(1);
      wprintf(L"\033[H\033[2J\033[3J");
    }
  } while (!captured_king);

  wprintf(L"\033[H\033[2J\033[3J");
  wprintf(L"\nCheckmate, ");
  if (p_state->moves % 2 == 0) {
    wprintf(L"White wins!\n\n");
    print_board_white(p_state->board);
    print_history(p_state->p_history_head);
  } else {
    wprintf(L"Black wins!\n\n");
    print_board_black(p_state->board);
    print_history(p_state->p_history_head);
  }
}

/* Function: get_move
 * The get_move function handles the process of getting and validating a player's move in a chess game.
 *
 * Parameters:
 * - p_state: The game in progress. The board, captures and history are updated in place.
 * - captured_king: Pointer to an integer indicating if a king has been captured.
 *
 * The function performs the following steps:
 * 1. Prompts the player to enter the position of the piece they want to move.
 * 2. Validates the input and checks if the selected piece is of the correct color.
 * 3. Prompts the player to enter the position where they want to move the piece.
 * 4. Validates the input and gets the coordinates of the next position.
 * 5. Checks if the move is valid.
 * 6. If the move is valid, updates the board, captures, and history and returns.
 * 7. If the move is invalid, says so and asks again.
 */
void get_move(GameState *p_state, int *captured_king) {
  char prev_pos[3];
  char next_pos[3];
  int prev_i, prev_j, next_i, next_j;

  // The side to move owns the list its captures go onto
  Captures_node_t **pp_capture_color_head = (p_state->moves % 2 != 0)
                                                ? &p_state->p_captures_white_head
                                                : &p_state->p_captures_black_head;

  // Ask until a legal move is entered. This used to be a recursive call to
  // get_move, so a player who kept picking illegal moves grew the stack
  // instead of just being asked again.
  while (1) {
    // Get the piece to move
    while (1) {
      wprintf(L"Enter the position of the piece you want to move: ");
      wscanf(L"%2s", prev_pos);
      if (strlen(prev_pos) == 2 && prev_pos[0] >= 'a' && prev_pos[0] <= 'h' && prev_pos[1] >= '1' && prev_pos[1] <= '8') {
        prev_pos[2] = '\0';
        // Check if the selected piece is of the correct color
        if (square_to_index(prev_pos, &prev_i, &prev_j) && p_state->board[prev_i][prev_j].type != FREE && p_state->board[prev_i][prev_j].color == ((p_state->moves % 2 != 0) ? WHITE : BLACK)) {
          break;
        } else {
          wprintf(L"Invalid selection. Please select a piece of the correct color.\n");
        }
      } else {
        wprintf(L"Invalid input. Please enter a valid position.\n");
      }
      while (getwchar() != '\n')
        ; // Clear the input buffer
    }

    // Get the next position
    while (1) {
      wprintf(L"Enter the position where you want to move the piece: ");
      wscanf(L"%2s", next_pos);
      if (strlen(next_pos) == 2 && next_pos[0] >= 'a' && next_pos[0] <= 'h' &&
          next_pos[1] >= '1' && next_pos[1] <= '8') {
        next_pos[2] = '\0';
        break;
      }
      wprintf(L"Invalid input. Please enter a valid position.\n");
      while (getwchar() != '\n')
        ; // Clear the input buffer
    }

    // Get the coordinates of the next position
    square_to_index(next_pos, &next_i, &next_j);

    // Check if the move is valid
    if (!is_valid_move(p_state->board, prev_i, prev_j, next_i, next_j)) {
      wprintf(L"Invalid move. Please try again.\n");
      continue;
    }

    // Check if the move captures a piece
    if (p_state->board[next_i][next_j].type != FREE) {
      // Update the captures
      update_captures(pp_capture_color_head, p_state->board[next_i][next_j]);

      // Check if the piece being captured is a king
      if (p_state->board[next_i][next_j].type == KING) {
        // End the game
        *captured_king = 1;
      }
    }
    // Update the board
    update_board(p_state->board, prev_i, prev_j, next_i, next_j);
    // Update the history
    update_history(&p_state->p_history_head, prev_pos, next_pos);

    return;
  }
}
