/*
Ezequiel Buck
Programming Principles
Final Project
Console Chess Game
*/

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef enum
{
  WHITE,
  BLACK,
  NONE
} Color;

typedef struct
{
  wchar_t icon;
  Color color;
  char position[2];
  char type[10];
} Piece_t;

// Linked list to store the player's captures
typedef struct Captures_node_s
{
  Piece_t piece;
  struct Captures_node_s *p_next;
} Captures_node_t;

// Linked list to store the moves made
typedef struct History_node_s
{
  char prev_pos[2];
  char next_pos[2];
  struct History_node_s *p_next;
} History_node_t;

void init_board(Piece_t board[8][8]);
void print_board(Piece_t board[8][8]);
void print_history(History_node_t *p_history_head);
int is_valid_move();
void update_board(Piece_t board[8][8], char prev_pos[2], char next_pos[2]);
void update_history(History_node_t **pp_history_head, char prev_pos[2], char next_pos[2]);
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves, int choice);

int main(void)
{
  // Set to a different locale to display unicode characters
  setlocale(LC_ALL, "");

  int choice;
  int moves = 0;

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
    print_history(p_history_head);
    // TODO: Add code to load a saved game
    break;
  default:
    wprintf(L"Unexpected error.\n");
    break;
  }

  // Main game loop
  game_loop(board, p_captures_white_head, p_captures_black_head, p_history_head, moves, choice);

  return 0;
}

void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves, int choice)
{
  // Game loop
  while (1)
  {
    print_board(board);

    if (moves % 2 != 0)
      wprintf(L"\nWhite's turn\n");
    else
      wprintf(L"\nBlack's turn\n");

    moves++;
  }
}

void get_move(Piece_t board[8][8], Captures_node_t *p_caputer_color_head, History_node_t *p_history_head)
{
    char prev_pos[2];
    char next_pos[2];
    
    wprintf(L"Enter the position of the piece you want to move: ");
    wscanf(L"%s", prev_pos);
    wprintf(L"Enter the position where you want to move the piece: ");
    wscanf(L"%s", next_pos);
    
    // Check if the move is valid
    if (is_valid_move())
    {
        // Update the board
        update_board(board, prev_pos, next_pos);
        // Update the history
        update_history(&p_history_head, prev_pos, next_pos);
    }
    else
    {
        wprintf(L"Invalid move. Please try again.\n");
        get_move(board, p_caputer_color_head, p_history_head);
    }
}

int is_valid_move()
{
  return 0;
}

void update_board(Piece_t board[8][8], char prev_pos[2], char next_pos[2]) {}

void update_history(History_node_t **pp_history_head, char prev_pos[2], char next_pos[2])
{
  History_node_t *p_new_node = (History_node_t *)malloc(sizeof(History_node_t));
  if (p_new_node == NULL)
  {
    wprintf(L"Memory allocation failed.\n");
    exit(1);
  }

  strcpy(p_new_node->prev_pos, prev_pos);
  strcpy(p_new_node->next_pos, next_pos);
  p_new_node->p_next = NULL;

  if (*pp_history_head == NULL)
  {
    *pp_history_head = p_new_node;
  }
  else
  {
    History_node_t *p_current = *pp_history_head;
    while (p_current->p_next != NULL)
    {
      p_current = p_current->p_next;
    }
    p_current->p_next = p_new_node;
  }
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
  Piece_t temp_board[8][8] = {
      {{L'♖', WHITE, "a1", "rook"},
       {L'♘', WHITE, "b1", "knight"},
       {L'♗', WHITE, "c1", "bishop"},
       {L'♕', WHITE, "d1", "queen"},
       {L'♔', WHITE, "e1", "king"},
       {L'♗', WHITE, "f1", "bishop"},
       {L'♘', WHITE, "g1", "knight"},
       {L'♖', WHITE, "h1", "rook"}},
      {{L'♙', WHITE, "a2", "pawn"},
       {L'♙', WHITE, "b2", "pawn"},
       {L'♙', WHITE, "c2", "pawn"},
       {L'♙', WHITE, "d2", "pawn"},
       {L'♙', WHITE, "e2", "pawn"},
       {L'♙', WHITE, "f2", "pawn"},
       {L'♙', WHITE, "g2", "pawn"},
       {L'♙', WHITE, "h2", "pawn"}},
      {{L' ', NONE, "a3", "free"},
       {L' ', NONE, "b3", "free"},
       {L' ', NONE, "c3", "free"},
       {L' ', NONE, "d3", "free"},
       {L' ', NONE, "e3", "free"},
       {L' ', NONE, "f3", "free"},
       {L' ', NONE, "g3", "free"},
       {L' ', NONE, "h3", "free"}},
      {{L' ', NONE, "a4", "free"},
       {L' ', NONE, "b4", "free"},
       {L' ', NONE, "c4", "free"},
       {L' ', NONE, "d4", "free"},
       {L' ', NONE, "e4", "free"},
       {L' ', NONE, "f4", "free"},
       {L' ', NONE, "g4", "free"},
       {L' ', NONE, "h4", "free"}},
      {{L' ', NONE, "a5", "free"},
       {L' ', NONE, "b5", "free"},
       {L' ', NONE, "c5", "free"},
       {L' ', NONE, "d5", "free"},
       {L' ', NONE, "e5", "free"},
       {L' ', NONE, "f5", "free"},
       {L' ', NONE, "g5", "free"},
       {L' ', NONE, "h5", "free"}},
      {{L' ', NONE, "a6", "free"},
       {L' ', NONE, "b6", "free"},
       {L' ', NONE, "c6", "free"},
       {L' ', NONE, "d6", "free"},
       {L' ', NONE, "e6", "free"},
       {L' ', NONE, "f6", "free"},
       {L' ', NONE, "g6", "free"},
       {L' ', NONE, "h6", "free"}},
      {{L'♟', BLACK, "a7", "pawn"},
       {L'♟', BLACK, "b7", "pawn"},
       {L'♟', BLACK, "c7", "pawn"},
       {L'♟', BLACK, "d7", "pawn"},
       {L'♟', BLACK, "e7", "pawn"},
       {L'♟', BLACK, "f7", "pawn"},
       {L'♟', BLACK, "g7", "pawn"},
       {L'♟', BLACK, "h7", "pawn"}},
      {{L'♜', BLACK, "a8", "rook"},
       {L'♞', BLACK, "b8", "knight"},
       {L'♝', BLACK, "c8", "bishop"},
       {L'♛', BLACK, "d8", "queen"},
       {L'♚', BLACK, "e8", "king"},
       {L'♝', BLACK, "f8", "bishop"},
       {L'♞', BLACK, "g8", "knight"},
       {L'♜', BLACK, "h8", "rook"}}};

  memcpy(board, temp_board, sizeof(temp_board));
}
