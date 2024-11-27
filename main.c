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
#include <stdio.h>

#define DEBUG

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
void free_history(History_node_t *head);
void free_captures(Captures_node_t *head);
void print_board_white(Piece_t board[8][8]);
void print_board_black(Piece_t board[8][8]);
void print_history(History_node_t *p_history_head);
void print_captures(Captures_node_t *p_captures_head);
void update_captures(Captures_node_t **pp_captures_head, Piece_t piece);
int find_piece_coordinates(Piece_t board[8][8], char pos[3], int *i, int *j);
void update_board(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j);
int is_valid_move(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j);
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3]);
void get_move(Piece_t board[8][8], Captures_node_t **pp_capture_color_head, History_node_t **pp_history_head, int *captured_king, int *moves);
void save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves);
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int *moves);
void load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves);

int main(void)
{
  // Set to a different locale to display unicode characters
  setlocale(LC_ALL, "");

  int choice;
  int moves = 1;
  // Initialize the linked lists
  Captures_node_t *p_captures_white_head = NULL;
  Captures_node_t *p_captures_black_head = NULL;
  History_node_t *p_history_head = NULL;

  // Clear the screen
  wprintf(L"\033[H\033[J");

  // Welcome message and instructions
  wprintf(L"\nWelcome to Console Chess!\n");
  wprintf(L"2 Player Mode\n\n");
  // TODO: Add instructions

  // Initialize the board
  Piece_t board[8][8];
  init_board(board);
  print_board_white(board);

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

  // Clear the screen after main menu
  wprintf(L"\033[H\033[J");

  if (choice == 2)
  {
    load_game(board, &p_captures_white_head, &p_captures_black_head, &p_history_head, &moves);
    print_history(p_history_head);
    wprintf(L"\n");
  }

  // Main game loop
  game_loop(board, p_captures_white_head, p_captures_black_head, p_history_head, &moves);

  free_captures(p_captures_white_head);
  free_captures(p_captures_black_head);
  free_history(p_history_head);

  return 0;
}

void load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves)
{
  FILE *file = fopen("game_save.bin", "rb");
  if (file == NULL)
  {
    wprintf(L"Error opening file for loading.\n");
    return;
  }

  // Read the board array
  fread(board, sizeof(Piece_t), 8 * 8, file);

  // Read the white captures linked list
  Captures_node_t *current = NULL;
  Captures_node_t *prev = NULL;
  while (1)
  {
    current = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    fread(current, sizeof(Captures_node_t), 1, file);
    if (current->p_next == NULL)
    {
      free(current);
      break;
    }
    if (*p_captures_white_head == NULL)
    {
      *p_captures_white_head = current;
    }
    else
    {
      prev->p_next = current;
    }
    prev = current;
  }

  // Read the black captures linked list
  current = NULL;
  prev = NULL;
  while (1)
  {
    current = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    fread(current, sizeof(Captures_node_t), 1, file);
    if (current->p_next == NULL)
    {
      free(current);
      break;
    }
    if (*p_captures_black_head == NULL)
    {
      *p_captures_black_head = current;
    }
    else
    {
      prev->p_next = current;
    }
    prev = current;
  }

  // Read the history linked list
  History_node_t *current_history = NULL;
  History_node_t *prev_history = NULL;
  while (1)
  {
    current_history = (History_node_t *)malloc(sizeof(History_node_t));
    fread(current_history, sizeof(History_node_t), 1, file);
    if (current_history->p_next == NULL)
    {
      free(current_history);
      break;
    }
    if (*p_history_head == NULL)
    {
      *p_history_head = current_history;
    }
    else
    {
      prev_history->p_next = current_history;
    }
    prev_history = current_history;
  }

  // Read the number of moves
  fread(moves, sizeof(int), 1, file);

  fclose(file);
  wprintf(L"Game loaded successfully.\n");
}

void save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves)
{
  FILE *file = fopen("game_save.bin", "wb");
  if (file == NULL)
  {
    wprintf(L"Error opening file for saving.\n");
    return;
  }

  // Write the board array
  fwrite(board, sizeof(Piece_t), 8 * 8, file);

  // Write the white captures linked list
  Captures_node_t *current = p_captures_white_head;
  while (current != NULL)
  {
    fwrite(current, sizeof(Captures_node_t), 1, file);
    current = current->p_next;
  }
  // Write a NULL pointer to indicate the end of the list
  Captures_node_t *null_node = NULL;
  fwrite(&null_node, sizeof(Captures_node_t *), 1, file);

  // Write the black captures linked list
  current = p_captures_black_head;
  while (current != NULL)
  {
    fwrite(current, sizeof(Captures_node_t), 1, file);
    current = current->p_next;
  }
  // Write a NULL pointer to indicate the end of the list
  fwrite(&null_node, sizeof(Captures_node_t *), 1, file);

  // Write the history linked list
  History_node_t *current_history = p_history_head;
  while (current_history != NULL)
  {
    fwrite(current_history, sizeof(History_node_t), 1, file);
    current_history = current_history->p_next;
  }
  // Write a NULL pointer to indicate the end of the list
  History_node_t *null_history_node = NULL;
  fwrite(&null_history_node, sizeof(History_node_t *), 1, file);

  // Write the number of moves
  fwrite(&moves, sizeof(int), 1, file);

  fclose(file);
  wprintf(L"Game saved successfully.\n");
}

void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int *moves)
{
  int captured_king = 0;
  wchar_t save_choice;

  do
  {

    if ((*moves) % 2 != 0)
    {
      print_board_white(board);
      wprintf(L"\n");
      print_captures(p_captures_white_head);
      print_captures(p_captures_black_head);
      wprintf(L"\nWhite's turn\n");
      get_move(board, &p_captures_white_head, &p_history_head, &captured_king, moves);
      wprintf(L"\033[H\033[J");
      print_board_white(board);
    }
    else
    {
      print_board_black(board);
      wprintf(L"\n");
      print_captures(p_captures_white_head);
      print_captures(p_captures_black_head);
      wprintf(L"\nBlack's turn\n");
      get_move(board, &p_captures_black_head, &p_history_head, &captured_king, moves);
      wprintf(L"\033[H\033[J");
      print_board_black(board);
    }
    (*moves)++;

    if (!captured_king)
    {
      // Show the move for a second
      sleep(1);
      wprintf(L"\033[H\033[J");

      // Ask the player if they want to save the game
      wprintf(L"\nDo you want to save the game? (y/N): ");
      wscanf(L" %lc", &save_choice);
      while (getwchar() != '\n')
        ; // Clear the input buffer
      if (save_choice == L'y' || save_choice == L'Y')
      {
        save_game(board, p_captures_white_head, p_captures_black_head, p_history_head, *moves);
        exit(0);
      }
      else
      {
        wprintf(L"\033[H\033[J");
      }
    }
  } while (!captured_king);

  wprintf(L"\nCheckmate!\n");
  if ((*moves) % 2 == 0)
  {
    wprintf(L"White wins!\n");
  }
  else
  {
    wprintf(L"Black wins!\n");
  }
}

void get_move(Piece_t board[8][8], Captures_node_t **pp_capture_color_head, History_node_t **pp_history_head, int *captured_king, int *moves)
{
  char prev_pos[3];
  char next_pos[3];
  int prev_i, prev_j, next_i, next_j;

  // Get the piece to move
  while (1)
  {
    wprintf(L"Enter the position of the piece you want to move (e.g., e2): ");
    wscanf(L"%2s", prev_pos);
    if (strlen(prev_pos) == 2 && prev_pos[0] >= 'a' && prev_pos[0] <= 'h' && prev_pos[1] >= '1' && prev_pos[1] <= '8')
    {
      prev_pos[2] = '\0';
      // Check if the selected piece is of the correct color
      if (find_piece_coordinates(board, prev_pos, &prev_i, &prev_j) && board[prev_i][prev_j].type != FREE && board[prev_i][prev_j].color == ((*moves % 2 != 0) ? WHITE : BLACK))
      {
        break;
      }
      else
      {
        wprintf(L"Invalid selection. Please select a piece of the correct color.\n");
      }
    }
    else
    {
      wprintf(L"Invalid input. Please enter a valid position.\n");
    }
    while (getwchar() != '\n')
      ; // Clear the input buffer
  }

  // Get the next position
  while (1)
  {
    wprintf(L"Enter the position where you want to move the piece (e.g., e4): ");
    wscanf(L"%2s", next_pos);
    if (strlen(next_pos) == 2 && next_pos[0] >= 'a' && next_pos[0] <= 'h' && next_pos[1] >= '1' && next_pos[1] <= '8')
    {
      next_pos[2] = '\0';
      break;
    }
    wprintf(L"Invalid input. Please enter a valid position.\n");
    while (getwchar() != '\n')
      ; // Clear the input buffer
  }

  // Get the coordinates of the next position
  find_piece_coordinates(board, next_pos, &next_i, &next_j);

  // Check if the move is valid
  if (is_valid_move(board, prev_i, prev_j, next_i, next_j))
  {
    // Check if the move captures a piece
    if (board[next_i][next_j].type != FREE)
    {
      // Update the captures
      update_captures(pp_capture_color_head, board[next_i][next_j]);

      // Check if the piece being captured is a king
      if (board[next_i][next_j].type == KING)
      {
        // End the game
        *captured_king = 1;
      }
    }
    // Update the board
    update_board(board, prev_i, prev_j, next_i, next_j);
    // Update the history
    update_history(pp_history_head, prev_pos, next_pos);
  }
  else
  {
    wprintf(L"Invalid move. Please try again.\n");
    get_move(board, pp_capture_color_head, pp_history_head, captured_king, moves);
  }
}

int is_valid_move(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j)
{
  // Get the piece type being moved
  int piece_type = board[prev_i][prev_j].type;

  // Check if the player is not trying to capture their own piece
  if (board[next_i][next_j].color == board[prev_i][prev_j].color)
  {
    return 0;
  }

#ifdef DEBUG
  return 1;
#endif

  switch (piece_type)
  {
  case PAWN:
    switch (board[prev_i][prev_j].color)
    {
    case WHITE:
      // Moving one square forward
      if (next_i == prev_i - 1 && next_j == prev_j && board[next_i][next_j].type == FREE)
      {
        return 1;
      }
      // Moving two squares forward on the first move
      if (prev_i == 6 && next_i == prev_i - 2 && next_j == prev_j && board[next_i][next_j].type == FREE && board[prev_i - 1][next_j].type == FREE)
      {
        return 1;
      }
      // Capturing diagonally
      if (next_i == prev_i - 1 && (next_j == prev_j - 1 || next_j == prev_j + 1) && board[next_i][next_j].color == BLACK)
      {
        return 1;
      }
      break;
    case BLACK:
      // Moving one square forward
      if (next_i == prev_i + 1 && next_j == prev_j && board[next_i][next_j].type == FREE)
      {
        return 1;
      }
      // Moving two squares forward on the first move
      if (prev_i == 1 && next_i == prev_i + 2 && next_j == prev_j && board[next_i][next_j].type == FREE && board[prev_i + 1][next_j].type == FREE)
      {
        return 1;
      }
      // Capturing diagonally
      if (next_i == prev_i + 1 && (next_j == prev_j - 1 || next_j == prev_j + 1) && board[next_i][next_j].color == WHITE)
      {
        return 1;
      }
      break;
    }
    return 0;

  case ROOK:
    // Moving vertically
    if (next_j == prev_j)
    {
      int step = (next_i > prev_i) ? 1 : -1;
      for (int i = prev_i + step; i != next_i; i += step)
      {
        if (board[i][prev_j].type != FREE)
        {
          return 0;
        }
      }
      return 1;
    }
    // Moving horizontally
    if (next_i == prev_i)
    {
      int step = (next_j > prev_j) ? 1 : -1;
      for (int j = prev_j + step; j != next_j; j += step)
      {
        if (board[prev_i][j].type != FREE)
        {
          return 0;
        }
      }
      return 1;
    }
    return 0;

  case BISHOP:
    // Moving diagonally
    if (abs(next_i - prev_i) == abs(next_j - prev_j))
    {
      int step_i = (next_i > prev_i) ? 1 : -1;
      int step_j = (next_j > prev_j) ? 1 : -1;
      int i = prev_i + step_i;
      int j = prev_j + step_j;
      while (i != next_i && j != next_j)
      {
        if (board[i][j].type != FREE)
        {
          return 0;
        }
        i += step_i;
        j += step_j;
      }
      return 1;
    }
    return 0;

  case QUEEN:
    // Combine rook and bishop logic
    // Moving vertically
    if (next_j == prev_j)
    {
      int step = (next_i > prev_i) ? 1 : -1;
      for (int i = prev_i + step; i != next_i; i += step)
      {
        if (board[i][prev_j].type != FREE)
        {
          return 0;
        }
      }
      return 1;
    }
    // Moving horizontally
    if (next_i == prev_i)
    {
      int step = (next_j > prev_j) ? 1 : -1;
      for (int j = prev_j + step; j != next_j; j += step)
      {
        if (board[prev_i][j].type != FREE)
        {
          return 0;
        }
      }
      return 1;
    }
    // Moving diagonally
    if (abs(next_i - prev_i) == abs(next_j - prev_j))
    {
      int step_i = (next_i > prev_i) ? 1 : -1;
      int step_j = (next_j > prev_j) ? 1 : -1;
      int i = prev_i + step_i;
      int j = prev_j + step_j;
      while (i != next_i || j != next_j)
      {
        if (board[i][j].type != FREE)
        {
          return 0;
        }
        i += step_i;
        j += step_j;
      }
      return 1;
    }
    return 0;

  case KING:
    // Moving one square in any direction
    if (abs(next_i - prev_i) <= 1 && abs(next_j - prev_j) <= 1)
    {
      return 1;
    }
    return 0;

  case KNIGHT:
    // Moving in an L-shape
    if ((abs(next_i - prev_i) == 2 && abs(next_j - prev_j) == 1) || (abs(next_i - prev_i) == 1 && abs(next_j - prev_j) == 2))
    {
      return 1;
    }
    return 0;

  default:
    return 0;
  }
}

void update_captures(Captures_node_t **pp_captures_head, Piece_t piece)
{
  Captures_node_t *p_node = (Captures_node_t *)malloc(sizeof(Captures_node_t));
  if (p_node == NULL)
  {
    wprintf(L"Memory allocation failed.\n");
    exit(1);
  }

  p_node->piece = piece;
  p_node->p_next = NULL;

  if (*pp_captures_head == NULL)
  {
    *pp_captures_head = p_node;
  }
  else
  {
    Captures_node_t *p_current = *pp_captures_head;
    while (p_current->p_next != NULL)
    {
      p_current = p_current->p_next;
    }
    p_current->p_next = p_node;
  }
}

void free_captures(Captures_node_t *head)
{
  Captures_node_t *tmp;
  while (head != NULL)
  {
    tmp = head;
    head = head->p_next;
    free(tmp);
  }
}

void free_history(History_node_t *head)
{
  History_node_t *tmp;
  while (head != NULL)
  {
    tmp = head;
    head = head->p_next;
    free(tmp);
  }
}

void update_board(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j)
{
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

void print_captures(Captures_node_t *p_captures_head)
{
  if (p_captures_head != NULL)
  {
    Captures_node_t *p_current = p_captures_head;
    while (p_current != NULL)
    {
      wprintf(L"%lc ", p_current->piece.icon);
      p_current = p_current->p_next;
    }
    wprintf(L"\n");
  }
}

void print_board_white(Piece_t board[8][8])
{
  wprintf(L"   a   b   c   d   e   f   g   h\n");
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
  wprintf(L"   h   g   f   e   d   c   b   a\n");
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
