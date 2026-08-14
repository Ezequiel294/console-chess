
/*
Ezequiel Buck
Programming Principles
Final Project
Console Chess Game
*/


#include "core/board.h"
#include "ui/display.h"
#include "app/game.h"
#include "core/history.h"
#include "app/save.h"
#include "types.h"

#include <locale.h>
#include <wchar.h>

int main(void) {
  // Set to a different locale to display unicode characters
  setlocale(LC_ALL, "");

  int choice;

  // main owns the game. Everything below borrows it by address, so there is
  // never a second copy of a list head to get out of step.
  GameState state = {0};
  state.moves = 1;

  // Clear the screen
  wprintf(L"\033[H\033[2J\033[3J");

  // Welcome message and instructions
  wprintf(L"\nWelcome to Console Chess!\n");
  wprintf(L"2 Player Mode\n\n");
  wprintf(L"How to Play:\n");
  wprintf(L"• Enter moves using chess coordinates, letter first! (e.g., 'e2' to 'e4')\n");
  wprintf(L"• White pieces play first (♚), then Black (♔)\n");
  wprintf(L"• Save your game anytime by entering 'y' when prompted\n");
  wprintf(L"• Capture pieces to win - the game ends when a King is captured!\n\n");

  // Initialize the board
  init_board(state.board);
  print_board_white(state.board);

  // Start of the Main Menu
  wprintf(L"\n1. New Game\n");
  wprintf(L"2. Load Game\n");
  wprintf(L"Enter a number: ");
  // Input validation
  while (wscanf(L"%d", &choice) != 1 || (choice != 1 && choice != 2)) {
    wprintf(L"Invalid input. Please enter 1 or 2: ");
    // Clear the input buffer
    while (getwchar() != '\n')
      ;
  }

  // Clear the screen after main menu
  wprintf(L"\033[H\033[2J\033[3J");

  if (choice == 2) {
    if (load_game(&state)) {
      print_history(state.p_history_head);
      wprintf(L"\n");
    } else {
      wprintf(L"Error loading game. Starting a new one.\n\n");
    }
  }

  // Main game loop
  game_loop(&state);

  free_captures(state.p_captures_white_head);
  free_captures(state.p_captures_black_head);
  free_history(state.p_history_head);

  return 0;
}
