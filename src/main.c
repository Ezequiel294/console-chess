
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
#include "version.h"

#include <locale.h>
#include <string.h>
#include <wchar.h>

#define PROGRAM_NAME "Console Chess"

static void print_version(void) {
  wprintf(L"%hs %hs\n", PROGRAM_NAME, chess_version());
}

// Lists every accepted option, including --version, so --help stays the
// single place usage has to be updated when an option is added.
static void print_usage(void) {
  wprintf(L"Usage: console-chess [OPTION]\n");
  wprintf(L"\n");
  wprintf(L"  -v, --version   print the version and exit\n");
  wprintf(L"  -h, --help      print this help and exit\n");
}

int main(int argc, char **argv) {
  // Handled before locale setup, terminal output, or file access, so a
  // broken environment cannot affect these options.
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
      print_version();
      return 0;
    }
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage();
      return 0;
    }
    wprintf(L"Unrecognised option: %hs\n\n", argv[i]);
    print_usage();
    return 1;
  }

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
  wprintf(L"\nWelcome to Console Chess %hs!\n", chess_version());
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
    if (load_game(&state, NULL)) {
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
