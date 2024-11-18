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
  char id[10];
  char position[2];
} Piece_t;

void init_board(Piece_t[8][8]);
void print_board(Piece_t board[8][8]);

int main(void) {
  // Set to a different locale to display unicode characters
  setlocale(LC_ALL, "");

  // Welcome message and instructions
  wprintf(L"\nWelcome to Console Chess!\n");
  // TODO: Add instructions

  // Initialize the board
  Piece_t board[8][8];
  init_board(board);
  print_board(board);

  return 0;
}

void print_board(Piece_t board[8][8]) {
  wprintf(L"\n   a   b   c   d   e   f   g   h\n");
  wprintf(L" +---+---+---+---+---+---+---+---+\n");
  for (int i = 0; i < 8; i++) {
    wprintf(L"%d|", 8 - i);
    for (int j = 0; j < 8; j++) {
      wprintf(L" %lc |", board[i][j].icon);
    }
    wprintf(L" %d\n", 8 - i);
    wprintf(L" +---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   a   b   c   d   e   f   g   h\n");
}

void init_board(Piece_t board[8][8]) {
  Piece_t temp_board[8][8] = {{{L'♖', 'w', "Ra1"},
                               {L'♘', 'w', "Nb1"},
                               {L'♗', 'w', "Bc1"},
                               {L'♕', 'w', "Qd1"},
                               {L'♔', 'w', "Ke1"},
                               {L'♗', 'w', "Bf1"},
                               {L'♘', 'w', "Ng1"},
                               {L'♖', 'w', "Rh1"}},
                              {{L'♙', 'w', "Pa2"},
                               {L'♙', 'w', "Pb2"},
                               {L'♙', 'w', "Pc2"},
                               {L'♙', 'w', "Pd2"},
                               {L'♙', 'w', "Pe2"},
                               {L'♙', 'w', "Pf2"},
                               {L'♙', 'w', "Pg2"},
                               {L'♙', 'w', "Ph2"}},
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
                              {{L'♟', 'b', "Pa7"},
                               {L'♟', 'b', "Pb7"},
                               {L'♟', 'b', "Pc7"},
                               {L'♟', 'b', "Pd7"},
                               {L'♟', 'b', "Pe7"},
                               {L'♟', 'b', "Pf7"},
                               {L'♟', 'b', "Pg7"},
                               {L'♟', 'b', "Ph7"}},
                              {{L'♜', 'b', "Ra8"},
                               {L'♞', 'b', "Nb8"},
                               {L'♝', 'b', "Bc8"},
                               {L'♛', 'b', "Qd8"},
                               {L'♚', 'b', "Ke8"},
                               {L'♝', 'b', "Bf8"},
                               {L'♞', 'b', "Ng8"},
                               {L'♜', 'b', "Rh8"}}};
  memcpy(board, temp_board, sizeof(temp_board));
}
