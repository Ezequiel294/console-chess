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
  Piece_t temp_board[8][8] = {{{L'♖', 'w', "R1"},
                               {L'♘', 'w', "N1"},
                               {L'♗', 'w', "B1"},
                               {L'♕', 'w', "Q"},
                               {L'♔', 'w', "K"},
                               {L'♗', 'w', "B2"},
                               {L'♘', 'w', "N2"},
                               {L'♖', 'w', "R2"}},
                              {{L'♙', 'w', "P1"},
                               {L'♙', 'w', "P2"},
                               {L'♙', 'w', "P3"},
                               {L'♙', 'w', "P4"},
                               {L'♙', 'w', "P5"},
                               {L'♙', 'w', "P6"},
                               {L'♙', 'w', "P7"},
                               {L'♙', 'w', "P8"}},
                              {{L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""}},
                              {{L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""}},
                              {{L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""}},
                              {{L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""},
                               {L' ', ' ', ""}},
                              {{L'♟', 'b', "P1"},
                               {L'♟', 'b', "P2"},
                               {L'♟', 'b', "P3"},
                               {L'♟', 'b', "P4"},
                               {L'♟', 'b', "P5"},
                               {L'♟', 'b', "P6"},
                               {L'♟', 'b', "P7"},
                               {L'♟', 'b', "P8"}},
                              {{L'♜', 'b', "R1"},
                               {L'♞', 'b', "N1"},
                               {L'♝', 'b', "B1"},
                               {L'♛', 'b', "Q"},
                               {L'♚', 'b', "K"},
                               {L'♝', 'b', "B2"},
                               {L'♞', 'b', "N2"},
                               {L'♜', 'b', "R2"}}};
  memcpy(board, temp_board, sizeof(temp_board));
}
