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
#include <unistd.h>

typedef enum
{
  WHITE,
  BLACK,
  NONE
} Color;

typedef enum
{
  PAWN,
  ROOK,
  KNIGHT,
  BISHOP,
  QUEEN,
  KING,
  FREE
} Piece_type_t;

typedef struct
{
  wchar_t icon;
  Color color;
  char position[3];
  Piece_type_t type;
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
  char prev_pos[3];
  char next_pos[3];
  struct History_node_s *p_next;
} History_node_t;

void init_board(Piece_t board[8][8]);
void print_board_white(Piece_t board[8][8]);
void print_board_black(Piece_t board[8][8]);
void print_history(History_node_t *p_history_head);
int is_capture(Piece_t board[8][8], char next_pos[3]);
void update_captures(Captures_node_t **pp_captures_head, Piece_t piece);
int is_valid_move(Piece_t board[8][8], char prev_pos[3], char next_pos[3]);
void update_board(Piece_t board[8][8], char prev_pos[3], char next_pos[3]);
int find_piece_coordinates(Piece_t board[8][8], char pos[3], int *i, int *j);
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3]);
void get_move(Piece_t board[8][8], Captures_node_t *p_capture_color_head, History_node_t *p_history_head);
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves, int choice);

int main(void)
{
  // Set to a different locale to display unicode characters
  setlocale(LC_ALL, "");

  int choice;
  int moves = 1;

  // Clear the screen
  wprintf(L"\033[H\033[J");

  // Welcome message and instructions
  wprintf(L"\nWelcome to Console Chess!\n");
  wprintf(L"2 Player Mode\n");
  // TODO: Add instructions

  // Initialize the board
  Piece_t board[8][8];
  init_board(board);
  print_board_white(board);

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
  // Clear the screen after main menu
  wprintf(L"\033[H\033[J");

  // Game loop
  while (1)
  {
    if (moves % 2 != 0)
    {
      print_board_white(board);
      wprintf(L"\nWhite's turn\n");
      get_move(board, p_captures_white_head, p_history_head);
      wprintf(L"\033[H\033[J");
      print_board_white(board);
    }
    else
    {
      print_board_black(board);
      wprintf(L"\nBlack's turn\n");
      get_move(board, p_captures_black_head, p_history_head);
      wprintf(L"\033[H\033[J");
      print_board_black(board);
    }

    moves++;

    // Show the move for 3 seconds before clearing the screen
    sleep(1.5);
    wprintf(L"\033[H\033[J");
  }
}

void get_move(Piece_t board[8][8], Captures_node_t *p_capture_color_head, History_node_t *p_history_head)
{
  char prev_pos[3];
  char next_pos[3];

  wprintf(L"Enter the position of the piece you want to move: ");
  wscanf(L"%s", prev_pos);
  wprintf(L"Enter the position where you want to move the piece: ");
  wscanf(L"%s", next_pos);

  // Check if the move is valid
  if (is_valid_move(board, prev_pos, next_pos))
  {
    // Check if the move captures a piece
    if (is_capture(board, next_pos))
    {
      int i, j;
      find_piece_coordinates(board, next_pos, &i, &j);
      // Update the captures
      update_captures(&p_capture_color_head, board[i][j]);
    }
    // Update the board
    update_board(board, prev_pos, next_pos);
    // Update the history
    update_history(&p_history_head, prev_pos, next_pos);
  }
  else
  {
    wprintf(L"Invalid move. Please try again.\n");
    get_move(board, p_capture_color_head, p_history_head);
  }
}

int is_valid_move(Piece_t board[8][8], char prev_pos[3], char next_pos[3])
{
  return 1;
}

int is_capture(Piece_t board[8][8], char next_pos[3])
{
  int i, j;
  find_piece_coordinates(board, next_pos, &i, &j);
  if (board[i][j].type != FREE)
  {
    return 1;
  }
  return 0;
}

void update_captures(Captures_node_t **pp_captures_head, Piece_t piece)
{
  Captures_node_t *p_new_node = (Captures_node_t *)malloc(sizeof(Captures_node_t));
  if (p_new_node == NULL)
  {
    wprintf(L"Memory allocation failed.\n");
    exit(1);
  }

  p_new_node->piece = piece;
  p_new_node->p_next = NULL;

  if (*pp_captures_head == NULL)
  {
    *pp_captures_head = p_new_node;
  }
  else
  {
    Captures_node_t *p_current = *pp_captures_head;
    while (p_current->p_next != NULL)
    {
      p_current = p_current->p_next;
    }
    p_current->p_next = p_new_node;
  }
}

void update_board(Piece_t board[8][8], char prev_pos[3], char next_pos[3])
{
  int prev_i, prev_j;
  int next_i, next_j;

  // Find the coordinates of the previous and next positions
  find_piece_coordinates(board, prev_pos, &prev_i, &prev_j);
  find_piece_coordinates(board, next_pos, &next_i, &next_j);

  // Copy the piece from the previous position to the next position
  board[next_i][next_j].icon = board[prev_i][prev_j].icon;
  board[next_i][next_j].color = board[prev_i][prev_j].color;
  board[next_i][next_j].type = board[prev_i][prev_j].type;

  // Clear the previous position
  board[prev_i][prev_j].icon = L' ';
  board[prev_i][prev_j].color = NONE;
  board[prev_i][prev_j].type = FREE;
}

void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3])
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

int find_piece_coordinates(Piece_t board[8][8], char pos[3], int *i, int *j)
{
  for (int x = 0; x < 8; x++)
  {
    for (int y = 0; y < 8; y++)
    {
      if (board[x][y].position[0] == pos[0] && board[x][y].position[1] == pos[1])
      {
        *i = x;
        *j = y;
        return 1;
      }
    }
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

void print_board_white(Piece_t board[8][8])
{
  wprintf(L"\n   a   b   c   d   e   f   g   h\n");
  wprintf(L" +---+---+---+---+---+---+---+---+\n");
  for (int i = 0; i < 8; i++)
  {
    wprintf(L"%d|", 8 - i);
    for (int j = 0; j < 8; j++)
    {
      wprintf(L" %lc |", board[i][j].icon);
    }
    wprintf(L" %d\n", 8 - i);
    wprintf(L" +---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   a   b   c   d   e   f   g   h\n");
}

void print_board_black(Piece_t board[8][8])
{
  wprintf(L"\n   h   g   f   e   d   c   b   a\n");
  wprintf(L" +---+---+---+---+---+---+---+---+\n");
  for (int i = 7; i >= 0; i--)
  {
    wprintf(L"%d|", 8 - i);
    for (int j = 7; j >= 0; j--)
    {
      wprintf(L" %lc |", board[i][j].icon);
    }
    wprintf(L" %d\n", 8 - i);
    wprintf(L" +---+---+---+---+---+---+---+---+\n");
  }
  wprintf(L"   h   g   f   e   d   c   b   a\n");
}

void init_board(Piece_t board[8][8])
{
  Piece_t temp_board[8][8] = {
      {{L'♖', BLACK, "a8", ROOK},
       {L'♘', BLACK, "b8", KNIGHT},
       {L'♗', BLACK, "c8", BISHOP},
       {L'♕', BLACK, "d8", QUEEN},
       {L'♔', BLACK, "e8", KING},
       {L'♗', BLACK, "f8", BISHOP},
       {L'♘', BLACK, "g8", KNIGHT},
       {L'♖', BLACK, "h8", ROOK}},
      {{L'♙', BLACK, "a7", PAWN},
       {L'♙', BLACK, "b7", PAWN},
       {L'♙', BLACK, "c7", PAWN},
       {L'♙', BLACK, "d7", PAWN},
       {L'♙', BLACK, "e7", PAWN},
       {L'♙', BLACK, "f7", PAWN},
       {L'♙', BLACK, "g7", PAWN},
       {L'♙', BLACK, "h7", PAWN}},
      {{L' ', NONE, "a6", FREE},
       {L' ', NONE, "b6", FREE},
       {L' ', NONE, "c6", FREE},
       {L' ', NONE, "d6", FREE},
       {L' ', NONE, "e6", FREE},
       {L' ', NONE, "f6", FREE},
       {L' ', NONE, "g6", FREE},
       {L' ', NONE, "h6", FREE}},
      {{L' ', NONE, "a5", FREE},
       {L' ', NONE, "b5", FREE},
       {L' ', NONE, "c5", FREE},
       {L' ', NONE, "d5", FREE},
       {L' ', NONE, "e5", FREE},
       {L' ', NONE, "f5", FREE},
       {L' ', NONE, "g5", FREE},
       {L' ', NONE, "h5", FREE}},
      {{L' ', NONE, "a4", FREE},
       {L' ', NONE, "b4", FREE},
       {L' ', NONE, "c4", FREE},
       {L' ', NONE, "d4", FREE},
       {L' ', NONE, "e4", FREE},
       {L' ', NONE, "f4", FREE},
       {L' ', NONE, "g4", FREE},
       {L' ', NONE, "h4", FREE}},
      {{L' ', NONE, "a3", FREE},
       {L' ', NONE, "b3", FREE},
       {L' ', NONE, "c3", FREE},
       {L' ', NONE, "d3", FREE},
       {L' ', NONE, "e3", FREE},
       {L' ', NONE, "f3", FREE},
       {L' ', NONE, "g3", FREE},
       {L' ', NONE, "h3", FREE}},
      {{L'♟', WHITE, "a2", PAWN},
       {L'♟', WHITE, "b2", PAWN},
       {L'♟', WHITE, "c2", PAWN},
       {L'♟', WHITE, "d2", PAWN},
       {L'♟', WHITE, "e2", PAWN},
       {L'♟', WHITE, "f2", PAWN},
       {L'♟', WHITE, "g2", PAWN},
       {L'♟', WHITE, "h2", PAWN}},
      {{L'♜', WHITE, "a1", ROOK},
       {L'♞', WHITE, "b1", KNIGHT},
       {L'♝', WHITE, "c1", BISHOP},
       {L'♛', WHITE, "d1", QUEEN},
       {L'♚', WHITE, "e1", KING},
       {L'♝', WHITE, "f1", BISHOP},
       {L'♞', WHITE, "g1", KNIGHT},
       {L'♜', WHITE, "h1", ROOK}}};

  memcpy(board, temp_board, sizeof(temp_board));
}
