/*
Ezequiel Buck
Programming Principles
Final Project
Console Chess Game
*/

#include <locale.h>
#include <string.h>
#include <wchar.h>

typedef struct {
  wchar_t icon;
  char color;
  char position[2];
} Piece_t;

// Linked list to store the player's captures
typedef struct {
  Piece_t piece;
  struct Captures_node_t *next;
} Captures_node_t;

// Linked list to store the moves made
typedef struct {
  char prev_pos[2];
  char next_pos[2];
  struct History_node_t *next;
} History_node_t;

void init_board(Piece_t board[8][8]);
void print_board(Piece_t board[8][8]);

int main(void) {
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

  // TODO: Make 2 linked lists to store the player's captures
  // TODO: Make a linked list to store the moves made

  return 0;
}

void print_board(Piece_t board[8][8]) {
  wprintf(L"\n   a   b   c   d   e   f   g   h               h   g   f   e   d "
          L"  c   b   a\n");
  wprintf(L" +---+---+---+---+---+---+---+---+           "
          L"+---+---+---+---+---+---+---+---+\n");
  for (int i = 0; i < 8; i++) {
    wprintf(L"%d|", 8 - i);
    for (int j = 0; j < 8; j++) {
      wprintf(L" %lc |", board[i][j].icon);
    }
    wprintf(L" %d        %d|", 8 - i, i + 1);
    for (int j = 7; j >= 0; j--) {
      wprintf(L" %lc |", board[7 - i][j].icon);
    }
    wprintf(L" %d\n", i + 1);
    wprintf(L" +---+---+---+---+---+---+---+---+           "
            L"+---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   a   b   c   d   e   f   g   h               h   g   f   e   d   "
          L"c   b   a\n");
}

void init_board(Piece_t board[8][8]) {
  Piece_t temp_board[8][8] = {{{L'♖', 'w', "a1"},
                               {L'♘', 'w', "b1"},
                               {L'♗', 'w', "c1"},
                               {L'♕', 'w', "d1"},
                               {L'♔', 'w', "e1"},
                               {L'♗', 'w', "f1"},
                               {L'♘', 'w', "g1"},
                               {L'♖', 'w', "h1"}},
                              {{L'♙', 'w', "a2"},
                               {L'♙', 'w', "b2"},
                               {L'♙', 'w', "c2"},
                               {L'♙', 'w', "d2"},
                               {L'♙', 'w', "e2"},
                               {L'♙', 'w', "f2"},
                               {L'♙', 'w', "g2"},
                               {L'♙', 'w', "h2"}},
                              {{L' ', ' ', "a3"},
                               {L' ', ' ', "b3"},
                               {L' ', ' ', "c3"},
                               {L' ', ' ', "d3"},
                               {L' ', ' ', "e3"},
                               {L' ', ' ', "f3"},
                               {L' ', ' ', "g3"},
                               {L' ', ' ', "h3"}},
                              {{L' ', ' ', "a4"},
                               {L' ', ' ', "b4"},
                               {L' ', ' ', "c4"},
                               {L' ', ' ', "d4"},
                               {L' ', ' ', "e4"},
                               {L' ', ' ', "f4"},
                               {L' ', ' ', "g4"},
                               {L' ', ' ', "h4"}},
                              {{L' ', ' ', "a5"},
                               {L' ', ' ', "b5"},
                               {L' ', ' ', "c5"},
                               {L' ', ' ', "d5"},
                               {L' ', ' ', "e5"},
                               {L' ', ' ', "f5"},
                               {L' ', ' ', "g5"},
                               {L' ', ' ', "h5"}},
                              {{L' ', ' ', "a6"},
                               {L' ', ' ', "b6"},
                               {L' ', ' ', "c6"},
                               {L' ', ' ', "d6"},
                               {L' ', ' ', "e6"},
                               {L' ', ' ', "f6"},
                               {L' ', ' ', "g6"},
                               {L' ', ' ', "h6"}},
                              {{L'♟', 'b', "a7"},
                               {L'♟', 'b', "b7"},
                               {L'♟', 'b', "c7"},
                               {L'♟', 'b', "d7"},
                               {L'♟', 'b', "e7"},
                               {L'♟', 'b', "f7"},
                               {L'♟', 'b', "g7"},
                               {L'♟', 'b', "h7"}},
                              {{L'♜', 'b', "a8"},
                               {L'♞', 'b', "b8"},
                               {L'♝', 'b', "c8"},
                               {L'♛', 'b', "d8"},
                               {L'♚', 'b', "e8"},
                               {L'♝', 'b', "f8"},
                               {L'♞', 'b', "g8"},
                               {L'♜', 'b', "h8"}}};
  memcpy(board, temp_board, sizeof(temp_board));
}
