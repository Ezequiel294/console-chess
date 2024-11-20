/*
Ezequiel Buck
Programming Principles
Final Project
Console Chess Game
*/

#include <locale.h>
#include <string.h>
#include <wchar.h>

typedef struct
{
  wchar_t icon;
  char color;
  char position[2];
  char type[10];
} Piece_t;

// Linked list to store the player's captures
typedef struct
{
  Piece_t piece;
  struct Captures_node_t *p_next;
} Captures_node_t;

// Linked list to store the moves made
typedef struct
{
  char prev_pos[2];
  char next_pos[2];
  struct History_node_t *p_next;
} History_node_t;

void init_board(Piece_t board[8][8]);
void print_board(Piece_t board[8][8]);

int main(void)
{
  // Set to a different locale to display unicode characters
  setlocale(LC_ALL, "");

  // Welcome message and instructions
  wprintf(L"\nWelcome to Console Chess!\n");
  wprintf(L"2 Player Mode\n");
  // TODO: Add instructions

  // Initialize the board
  Piece_t board[8][8];
  init_board(board);
  print_board(board);

  // Initialize the linked lists
  Captures_node_t *p_captures_white_head = NULL;
  Captures_node_t *p_captures_black_head = NULL;
  History_node_t *p_history_head = NULL;

  // Start of the Main Menu
  wprintf(L"\n1. New Game\n");
  wprintf(L"2. Load Game\n");
  wprintf(L"Enter a number: ");
  int choice;
  // Input validation
  while (wscanf(L"%d", &choice) != 1 || (choice != 1 && choice != 2))
  {
    wprintf(L"Invalid input. Please enter 1 or 2: ");
    // Clear the input buffer
    while (getwchar() != '\n')
      ;
  }

  switch (choice)
  {
  case 1:
    wprintf(L"\nStarting a new game...\n");
    // TODO: Add code to start a new game
    break;
  case 2:
    wprintf(L"\nLoading a saved game...\n");
    // TODO: Add code to load a saved game
    break;
  default:
    wprintf(L"Unexpected error.\n");
    break;
  }

  return 0;
}

void print_history(History_node_t *p_history_head)
{
  wprintf(L"\nMove History:\n");
  wprintf(L"+-----------------+\n");
  wprintf(L"|  From  |   To   |\n");
  wprintf(L"+-----------------+\n");
  History_node_t *p_current = p_history_head;
  while (p_current != NULL)
  {
    wprintf(L"|  %s   |  %s   |\n", p_current->prev_pos, p_current->next_pos);
    p_current = p_current->p_next;
  }
  wprintf(L"+-----------------+\n");
}

void print_board(Piece_t board[8][8])
{
  wprintf(L"\n   a   b   c   d   e   f   g   h               h   g   f   e   d "
          L"  c   b   a\n");
  wprintf(L" +---+---+---+---+---+---+---+---+           "
          L"+---+---+---+---+---+---+---+---+\n");
  for (int i = 0; i < 8; i++)
  {
    wprintf(L"%d|", 8 - i);
    for (int j = 0; j < 8; j++)
    {
      wprintf(L" %lc |", board[i][j].icon);
    }
    wprintf(L" %d        %d|", 8 - i, i + 1);
    for (int j = 7; j >= 0; j--)
    {
      wprintf(L" %lc |", board[7 - i][j].icon);
    }
    wprintf(L" %d\n", i + 1);
    wprintf(L" +---+---+---+---+---+---+---+---+           "
            L"+---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   a   b   c   d   e   f   g   h               h   g   f   e   d   "
          L"c   b   a\n");
}

void init_board(Piece_t board[8][8])
{
  Piece_t temp_board[8][8] = {{{L'♖', 'w', "a1", "rook"},
                               {L'♘', 'w', "b1", "knight"},
                               {L'♗', 'w', "c1", "bishop"},
                               {L'♕', 'w', "d1", "queen"},
                               {L'♔', 'w', "e1", "king"},
                               {L'♗', 'w', "f1", "bishop"},
                               {L'♘', 'w', "g1", "knight"},
                               {L'♖', 'w', "h1", "rook"}},
                              {{L'♙', 'w', "a2", "pawn"},
                               {L'♙', 'w', "b2", "pawn"},
                               {L'♙', 'w', "c2", "pawn"},
                               {L'♙', 'w', "d2", "pawn"},
                               {L'♙', 'w', "e2", "pawn"},
                               {L'♙', 'w', "f2", "pawn"},
                               {L'♙', 'w', "g2", "pawn"},
                               {L'♙', 'w', "h2", "pawn"}},
                              {{L' ', ' ', "a3", "free"},
                               {L' ', ' ', "b3", "free"},
                               {L' ', ' ', "c3", "free"},
                               {L' ', ' ', "d3", "free"},
                               {L' ', ' ', "e3", "free"},
                               {L' ', ' ', "f3", "free"},
                               {L' ', ' ', "g3", "free"},
                               {L' ', ' ', "h3", "free"}},
                              {{L' ', ' ', "a4", "free"},
                               {L' ', ' ', "b4", "free"},
                               {L' ', ' ', "c4", "free"},
                               {L' ', ' ', "d4", "free"},
                               {L' ', ' ', "e4", "free"},
                               {L' ', ' ', "f4", "free"},
                               {L' ', ' ', "g4", "free"},
                               {L' ', ' ', "h4", "free"}},
                              {{L' ', ' ', "a5", "free"},
                               {L' ', ' ', "b5", "free"},
                               {L' ', ' ', "c5", "free"},
                               {L' ', ' ', "d5", "free"},
                               {L' ', ' ', "e5", "free"},
                               {L' ', ' ', "f5", "free"},
                               {L' ', ' ', "g5", "free"},
                               {L' ', ' ', "h5", "free"}},
                              {{L' ', ' ', "a6", "free"},
                               {L' ', ' ', "b6", "free"},
                               {L' ', ' ', "c6", "free"},
                               {L' ', ' ', "d6", "free"},
                               {L' ', ' ', "e6", "free"},
                               {L' ', ' ', "f6", "free"},
                               {L' ', ' ', "g6", "free"},
                               {L' ', ' ', "h6", "free"}},
                              {{L'♟', 'b', "a7", "pawn"},
                               {L'♟', 'b', "b7", "pawn"},
                               {L'♟', 'b', "c7", "pawn"},
                               {L'♟', 'b', "d7", "pawn"},
                               {L'♟', 'b', "e7", "pawn"},
                               {L'♟', 'b', "f7", "pawn"},
                               {L'♟', 'b', "g7", "pawn"},
                               {L'♟', 'b', "h7", "pawn"}},
                              {{L'♜', 'b', "a8", "rook"},
                               {L'♞', 'b', "b8", "knight"},
                               {L'♝', 'b', "c8", "bishop"},
                               {L'♛', 'b', "d8", "queen"},
                               {L'♚', 'b', "e8", "king"},
                               {L'♝', 'b', "f8", "bishop"},
                               {L'♞', 'b', "g8", "knight"},
                               {L'♜', 'b', "h8", "rook"}}};
  memcpy(board, temp_board, sizeof(temp_board));
}
