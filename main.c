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

// #define DEBUG

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
int save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves);
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int *moves);
int load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves);

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
  wprintf(L"\033[H\033[2J\033[3J");

  if (choice == 2)
  {
    if (load_game(board, &p_captures_white_head, &p_captures_black_head, &p_history_head, &moves))
    {
      print_history(p_history_head);
      wprintf(L"\n");
    }
    else
    {
      wprintf(L"Error loading game. Starting a new one.\n\n");
    }
  }

  // Main game loop
  game_loop(board, p_captures_white_head, p_captures_black_head, p_history_head, &moves);

  free_captures(p_captures_white_head);
  free_captures(p_captures_black_head);
  free_history(p_history_head);

  return 0;
}

/* Function: game_loop
 * The game_loop function manages the main loop of a chess game.
 * It alternates turns between white and black players, updating the board and capturing pieces.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - p_captures_white_head: Pointer to the head of the linked list of captured white pieces.
 * - p_captures_black_head: Pointer to the head of the linked list of captured black pieces.
 * - p_history_head: Pointer to the head of the linked list of move history.
 * - moves: Pointer to the integer tracking the number of moves made.
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
      wprintf(L"\033[H\033[2J\033[3J");
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
      wprintf(L"\033[H\033[2J\033[3J");
      print_board_black(board);
    }
    (*moves)++;

    if (!captured_king && (*moves) % 5 == 0)
    {
      // Ask the player if they want to save the game
      wprintf(L"\nDo you want to save the game? (y/n): ");
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
        wprintf(L"\033[H\033[2J\033[3J");
      }
    }

    if ((*moves) % 5 != 0 && !captured_king)
    {
      sleep(1);
      wprintf(L"\033[H\033[2J\033[3J");
    }
  } while (!captured_king);

  wprintf(L"\033[H\033[2J\033[3J");
  wprintf(L"\nCheckmate, ");
  if ((*moves) % 2 == 0)
  {
    wprintf(L"White wins!\n\n");
    print_board_white(board);
    print_history(p_history_head);
  }
  else
  {
    wprintf(L"Black wins!\n\n");
    print_board_black(board);
    print_history(p_history_head);
  }
}

/* Function: get_move
 * The get_move function handles the process of getting and validating a player's move in a chess game.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - pp_capture_color_head: Double pointer to the head of the linked list of captured pieces of the current player's color.
 * - pp_history_head: Double pointer to the head of the linked list of move history.
 * - captured_king: Pointer to an integer indicating if a king has been captured.
 * - moves: Pointer to the integer tracking the number of moves made.
 *
 * The function performs the following steps:
 * 1. Prompts the player to enter the position of the piece they want to move.
 * 2. Validates the input and checks if the selected piece is of the correct color.
 * 3. Prompts the player to enter the position where they want to move the piece.
 * 4. Validates the input and gets the coordinates of the next position.
 * 5. Checks if the move is valid.
 * 6. If the move is valid, updates the board, captures, and history.
 * 7. If the move captures a king, sets the captured_king flag to end the game.
 * 8. If the move is invalid, prompts the player to try again.
 */
void get_move(Piece_t board[8][8], Captures_node_t **pp_capture_color_head, History_node_t **pp_history_head, int *captured_king, int *moves)
{
  char prev_pos[3];
  char next_pos[3];
  int prev_i, prev_j, next_i, next_j;

  // Get the piece to move
  while (1)
  {
    wprintf(L"Enter the position of the piece you want to move: ");
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
    wprintf(L"Enter the position where you want to move the piece: ");
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

/* Function: is_valid_move
 * The is_valid_move function checks if a move in a chess game is valid based on the piece type and the rules of chess.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - prev_i: The row index of the piece's current position.
 * - prev_j: The column index of the piece's current position.
 * - next_i: The row index of the piece's target position.
 * - next_j: The column index of the piece's target position.
 *
 * The function performs the following steps:
 * 1. Retrieves the type of the piece being moved.
 * 2. Checks if the target position contains a piece of the same color, returning 0 if true.
 * 3. For each piece type (PAWN, ROOK, BISHOP, QUEEN, KING, KNIGHT), checks if the move follows the rules for that piece:
 *    - PAWN: Moves forward one or two squares (if it is its first move), or captures diagonally.
 *    - ROOK: Moves vertically or horizontally without jumping over other pieces.
 *    - BISHOP: Moves diagonally without jumping over other pieces.
 *    - QUEEN: Combines the movement rules of the rook and bishop.
 *    - KING: Moves one square in any direction.
 *    - KNIGHT: Moves in an L-shape.
 * 4. Returns 1 if the move is valid, otherwise returns 0.
 */
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

/* Function: save_game
 * The save_game function saves the current state of a chess game to a binary file.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - p_captures_white_head: Pointer to the head of the linked list of captured white pieces.
 * - p_captures_black_head: Pointer to the head of the linked list of captured black pieces.
 * - p_history_head: Pointer to the head of the linked list of move history.
 * - moves: The number of moves made in the game.
 *
 * The function performs the following steps:
 * 1. Opens a binary file for writing. If the file cannot be opened, prints an error message and returns 0.
 * 2. Writes the number of moves to the file.
 * 3. Writes the board state to the file.
 * 4. Writes the captured white pieces to the file, followed by an end marker.
 * 5. Writes the captured black pieces to the file, followed by an end marker.
 * 6. Writes the move history to the file, followed by an end marker.
 * 7. Closes the file and prints a success message.
 * 8. Returns 1 to indicate successful saving.
 */
int save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves)
{
  FILE *file = fopen("game_save.bin", "wb");
  if (file == NULL)
  {
    wprintf(L"Error opening file for saving.\n");
    return 0;
  }

  // Save the number of moves
  fwrite(&moves, sizeof(int), 1, file);

  // Save the board
  fwrite(board, sizeof(Piece_t), 64, file);

  // Save the captures for white
  Captures_node_t *current_capture = p_captures_white_head;
  while (current_capture != NULL)
  {
    fwrite(&current_capture->piece, sizeof(Piece_t), 1, file);
    current_capture = current_capture->p_next;
  }
  Piece_t end_marker = {L'\0', NONE, "", FREE}; // End marker for captures
  fwrite(&end_marker, sizeof(Piece_t), 1, file);

  // Save the captures for black
  current_capture = p_captures_black_head;
  while (current_capture != NULL)
  {
    fwrite(&current_capture->piece, sizeof(Piece_t), 1, file);
    current_capture = current_capture->p_next;
  }
  fwrite(&end_marker, sizeof(Piece_t), 1, file);

  // Save the move history
  History_node_t *current_history = p_history_head;
  while (current_history != NULL)
  {
    fwrite(current_history, sizeof(History_node_t), 1, file);
    current_history = current_history->p_next;
  }
  History_node_t end_history_marker = {"", "", NULL}; // End marker for history
  fwrite(&end_history_marker, sizeof(History_node_t), 1, file);

  fclose(file);
  wprintf(L"Game saved successfully.\n");

  return 1;
}

/* Function: load_game
 * The load_game function loads the state of a chess game from a binary file.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - p_captures_white_head: Double pointer to the head of the linked list of captured white pieces.
 * - p_captures_black_head: Double pointer to the head of the linked list of captured black pieces.
 * - p_history_head: Double pointer to the head of the linked list of move history.
 * - moves: Pointer to the integer tracking the number of moves made.
 *
 * The function performs the following steps:
 * 1. Opens a binary file for reading. If the file cannot be opened, prints an error message and returns 0.
 * 2. Reads the number of moves from the file.
 * 3. Reads the board state from the file.
 * 4. Reads the captured white pieces from the file and reconstructs the linked list.
 * 5. Reads the captured black pieces from the file and reconstructs the linked list.
 * 6. Reads the move history from the file and reconstructs the linked list.
 * 7. Closes the file and returns 1 to indicate successful loading.
 */
int load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves)
{
  FILE *file = fopen("game_save.bin", "rb");
  if (file == NULL)
  {
    wprintf(L"Error opening file for loading.\n");
    return 0;
  }

  // Load the number of moves
  fread(moves, sizeof(int), 1, file);

  // Load the board
  fread(board, sizeof(Piece_t), 64, file);

  // Load the captures for white
  Captures_node_t *current_capture = NULL;
  Piece_t piece;
  while (fread(&piece, sizeof(Piece_t), 1, file) && piece.type != FREE)
  {
    Captures_node_t *new_capture = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    new_capture->piece = piece;
    new_capture->p_next = NULL;
    if (*p_captures_white_head == NULL)
    {
      *p_captures_white_head = new_capture;
    }
    else
    {
      current_capture->p_next = new_capture;
    }
    current_capture = new_capture;
  }

  // Load the captures for black
  current_capture = NULL;
  while (fread(&piece, sizeof(Piece_t), 1, file) && piece.type != FREE)
  {
    Captures_node_t *new_capture = (Captures_node_t *)malloc(sizeof(Captures_node_t));
    new_capture->piece = piece;
    new_capture->p_next = NULL;
    if (*p_captures_black_head == NULL)
    {
      *p_captures_black_head = new_capture;
    }
    else
    {
      current_capture->p_next = new_capture;
    }
    current_capture = new_capture;
  }

  // Load the move history
  History_node_t *current_history = NULL;
  History_node_t history_node;
  while (fread(&history_node, sizeof(History_node_t), 1, file) && history_node.prev_pos[0] != '\0')
  {
    History_node_t *new_history = (History_node_t *)malloc(sizeof(History_node_t));
    *new_history = history_node;
    new_history->p_next = NULL;
    if (*p_history_head == NULL)
    {
      *p_history_head = new_history;
    }
    else
    {
      current_history->p_next = new_history;
    }
    current_history = new_history;
  }

  fclose(file);
  wprintf(L"Game loaded successfully.\n");

  return 1;
}

/* Function: update_captures
 * The update_captures function adds a captured piece to the linked list of captures.
 *
 * Parameters:
 * - pp_captures_head: Double pointer to the head of the linked list of captured pieces.
 * - piece: The piece that has been captured.
 *
 * The function performs the following steps:
 * 1. Allocates memory for a new capture node. If memory allocation fails, prints an error message and exits.
 * 2. Initializes the new capture node with the captured piece and sets its next pointer to NULL.
 * 3. If the captures list is empty, sets the head of the list to the new node.
 * 4. If the captures list is not empty, traverses to the end of the list and adds the new node.
 */
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

/* Function: free_captures
 * The free_captures function frees the memory allocated for the linked list of captured pieces.
 *
 * Parameters:
 * - head: Pointer to the head of the linked list of captured pieces.
 *
 * The function performs the following steps:
 * 1. Iterates through the linked list.
 * 2. For each node, stores the next node in a temporary pointer.
 * 3. Frees the current node.
 * 4. Moves to the next node using the temporary pointer.
 * 5. Continues until all nodes are freed.
 */
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

/* Function: free_history
 * The free_history function frees the memory allocated for the linked list of move history.
 *
 * Parameters:
 * - head: Pointer to the head of the linked list of move history.
 *
 * The function performs the following steps:
 * 1. Iterates through the linked list.
 * 2. For each node, stores the next node in a temporary pointer.
 * 3. Frees the current node.
 * 4. Moves to the next node using the temporary pointer.
 * 5. Continues until all nodes are freed.
 */
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

/* Function: update_board
 * The update_board function updates the chess board by moving a piece from one position to another.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - prev_i: The row index of the piece's current position.
 * - prev_j: The column index of the piece's current position.
 * - next_i: The row index of the piece's target position.
 * - next_j: The column index of the piece's target position.
 *
 * The function performs the following steps:
 * 1. Copies the piece from the previous position to the next position.
 * 2. Clears the previous position by setting it to an empty piece.
 */
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

/* Function: update_history
 * The update_history function adds a new move to the linked list of move history.
 *
 * Parameters:
 * - pp_history_head: Double pointer to the head of the linked list of move history.
 * - prev_pos: The previous position of the piece as a string (e.g., "e2").
 * - next_pos: The next position of the piece as a string (e.g., "e4").
 *
 * The function performs the following steps:
 * 1. Allocates memory for a new history node. If memory allocation fails, prints an error message and exits.
 * 2. Initializes the new history node with the previous and next positions and sets its next pointer to NULL.
 * 3. If the history list is empty, sets the head of the list to the new node.
 * 4. If the history list is not empty, traverses to the end of the list and adds the new node.
 */
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

/* Function: find_piece_coordinates
 * The find_piece_coordinates function finds the coordinates of a piece on the chess board based on its position string.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 * - pos: The position of the piece as a string (e.g., "e2").
 * - i: Pointer to an integer where the row index of the piece will be stored.
 * - j: Pointer to an integer where the column index of the piece will be stored.
 *
 * The function performs the following steps:
 * 1. Iterates through each cell of the board.
 * 2. Compares the position string of each piece with the given position string.
 * 3. If a match is found, stores the coordinates in the provided pointers and returns 1.
 * 4. If no match is found after checking all cells, returns 0.
 */
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

/* Function: print_history
 * The print_history function prints the move history of the chess game in a formatted table.
 *
 * Parameters:
 * - p_history_head: Pointer to the head of the linked list of move history.
 *
 * The function performs the following steps:
 * 1. Prints the header of the move history table.
 * 2. Iterates through the linked list of move history.
 * 3. For each node, prints the previous and next positions of the move.
 * 4. Continues until all moves in the history are printed.
 * 5. Prints the footer of the move history table.
 */
void print_history(History_node_t *p_history_head)
{
  wprintf(L"\nMove History:\n");
  wprintf(L"+-----------------+\n");
  wprintf(L"|  From  |   To   |\n");
  wprintf(L"+-----------------+\n");
  History_node_t *p_current = p_history_head;
  while (p_current != NULL)
  {
    wprintf(L"|  %-5s |  %-5s |\n", p_current->prev_pos, p_current->next_pos);
    p_current = p_current->p_next;
  }
  wprintf(L"+-----------------+\n");
}

/* Function: print_captures
 * The print_captures function prints the icons of captured pieces from a linked list.
 *
 * Parameters:
 * - p_captures_head: Pointer to the head of the linked list of captured pieces.
 *
 * The function performs the following steps:
 * 1. Checks if the captures list is not empty.
 * 2. Iterates through the linked list of captured pieces.
 * 3. For each node, prints the icon of the captured piece.
 * 4. Continues until all captured pieces are printed.
 * 5. Prints a newline character at the end.
 */
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

/* Function: print_board_white
 * The print_board_white function prints the chess board from the white player's perspective.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 *
 * The function performs the following steps:
 * 1. Prints the column labels (a to h).
 * 2. Prints the top border of the board.
 * 3. Iterates through each row of the board.
 * 4. For each row, prints the row number, the icons of the pieces in each column, and the row number again.
 * 5. Prints the border between rows.
 * 6. Prints the column labels (a to h) again at the bottom.
 */
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

/* Function: print_board_black
 * The print_board_black function prints the chess board from the black player's perspective.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 *
 * The function performs the following steps:
 * 1. Prints the column labels (h to a).
 * 2. Prints the top border of the board.
 * 3. Iterates through each row of the board in reverse order (from 7 to 0).
 * 4. For each row, prints the row number, the icons of the pieces in each column in reverse order, and the row number again.
 * 5. Prints the border between rows.
 * 6. Prints the column labels (h to a) again at the bottom.
 */
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

/* Function: print_board
 * The init_board function initializes the chess board with the standard starting positions for all pieces.
 *
 * Parameters:
 * - board: The 8x8 array representing the chess board.
 *
 * The function performs the following steps:
 * 1. Defines a temporary board with the initial positions of all pieces.
 *    - The first two rows are filled with black pieces.
 *    - The last two rows are filled with white pieces.
 *    - The middle rows are empty.
 * 2. Copies the temporary board to the actual board using memcpy.
 */
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
