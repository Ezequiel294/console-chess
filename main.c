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
#include <termios.h>

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

// Game state for tracking special move conditions
typedef struct
{
  int white_king_moved;
  int black_king_moved;
  int white_rook_a_moved; // queenside (a1)
  int white_rook_h_moved; // kingside (h1)
  int black_rook_a_moved; // queenside (a8)
  int black_rook_h_moved; // kingside (h8)
  int en_passant_col;     // column where en passant capture is possible, -1 if none
} GameState_t;

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

char read_single_char(void);
void enable_mouse_tracking(void);
void disable_mouse_tracking(void);
void init_board(Piece_t board[8][8]);
void init_game_state(GameState_t *state);
void show_valid_moves(Piece_t board[8][8], int piece_i, int piece_j, GameState_t *state);
void clear_valid_moves(Piece_t board[8][8]);
void free_history(History_node_t *head);
void free_captures(Captures_node_t *head);
void enable_raw_mode(struct termios *orig);
void print_board_white(Piece_t board[8][8]);
void print_board_black(Piece_t board[8][8]);
void disable_raw_mode(struct termios *orig);
void print_history(History_node_t *p_history_head);
void print_captures(Captures_node_t *p_captures_head);
void update_captures(Captures_node_t **pp_captures_head, Piece_t piece);
int find_piece_coordinates(Piece_t board[8][8], char pos[3], int *i, int *j);
void update_board(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j);
int is_valid_move(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j, GameState_t *state);
void update_history(History_node_t **pp_history_head, char prev_pos[3], char next_pos[3]);
void redraw_screen(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, int is_white, const char *error_msg);
int read_position_from_input(char *buf, int buf_size, int is_white_perspective);
void handle_promotion(Piece_t board[8][8], int row, int col, Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, int is_white);
int save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves, GameState_t *state);
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int *moves, GameState_t *state);
int load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves, GameState_t *state);
void get_move(Piece_t board[8][8], Captures_node_t **pp_capture_color_head, Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t **pp_history_head, int *captured_king, int *moves, GameState_t *state);


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
  GameState_t game_state;
  init_game_state(&game_state);

  // Clear the screen
  wprintf(L"\033[H\033[2J\033[3J");

  // Welcome message and instructions
  wprintf(L"\nWelcome to Console Chess!\n");
  wprintf(L"2 Player Mode\n\n");
  wprintf(L"How to Play:\n");
  wprintf(L"• Enter moves using chess coordinates, letter first! (e.g., 'e2' to 'e4')\n");
  wprintf(L"• You can also click on the board to select and move pieces!\n");
  wprintf(L"• White pieces play first (♚), then Black (♔)\n");
  wprintf(L"• Type 'save' at any prompt to save the game and exit\n");
  wprintf(L"• Type 'history' at any prompt to view move history\n");
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
    if (!load_game(board, &p_captures_white_head, &p_captures_black_head, &p_history_head, &moves, &game_state))
    {
      wprintf(L"Error loading game. Starting a new one.\n\n");
    }
  }

  // Main game loop
  game_loop(board, p_captures_white_head, p_captures_black_head, p_history_head, &moves, &game_state);

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
 * 1. Initializes variables for tracking if the king is captured.
 * 2. Enters a loop that continues until a king is captured.
 * 3. Alternates turns between white and black players based on the move count.
 * 4. Prints the board and captures, prompts the current player for their move, and updates the board.
 * 5. Players can type 'save' or 'history' at any move prompt.
 * 6. Clears the screen after each move.
 * 7. If a king is captured, declares the winner and prints the final board and move history.
 */
void game_loop(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int *moves, GameState_t *state)
{
  int captured_king = 0;

  do
  {
    // Clear screen at the start of each iteration so the board is always at row 1
    wprintf(L"\033[H\033[2J\033[3J");

    if ((*moves) % 2 != 0)
    {
      print_board_white(board);
      wprintf(L"\n");
      print_captures(p_captures_white_head);
      print_captures(p_captures_black_head);
      wprintf(L"\nWhite's turn\n");
      get_move(board, &p_captures_white_head, p_captures_white_head, p_captures_black_head, &p_history_head, &captured_king, moves, state);
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
      get_move(board, &p_captures_black_head, p_captures_white_head, p_captures_black_head, &p_history_head, &captured_king, moves, state);
      wprintf(L"\033[H\033[2J\033[3J");
      print_board_black(board);
    }
    (*moves)++;

    if (!captured_king)
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
 * 8. If the move is invalid, redraws the screen and prompts the player to try again.
 */

/* Function: read_single_char
 * Reads a single character from stdin using raw mode (no echo, no line buffering).
 * Used for simple y/n prompts to avoid mixing wscanf with raw mode read().
 */
char read_single_char(void)
{
  struct termios orig;
  enable_raw_mode(&orig);
  char c = 0;
  read(STDIN_FILENO, &c, 1);
  disable_raw_mode(&orig);
  return c;
}

/* Function: redraw_screen
 * Clears the screen and redraws the board, captures, turn indicator,
 * and an optional error message. This ensures the board is always at a
 * predictable terminal position so mouse click coordinates stay correct.
 */
void redraw_screen(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, int is_white, const char *error_msg)
{
  wprintf(L"\033[H\033[2J\033[3J");
  if (is_white)
  {
    print_board_white(board);
  }
  else
  {
    print_board_black(board);
  }
  wprintf(L"\n");
  print_captures(p_captures_white_head);
  print_captures(p_captures_black_head);
  wprintf(L"\n%ls's turn\n", is_white ? L"White" : L"Black");
  if (error_msg)
  {
    wprintf(L"%s\n", error_msg);
  }
}

/* Function: enable_mouse_tracking
 * Enables xterm mouse click tracking using SGR extended mode.
 */
void enable_mouse_tracking(void)
{
  wprintf(L"\033[?1000h\033[?1006h");
  fflush(stdout);
}

/* Function: disable_mouse_tracking
 * Disables xterm mouse click tracking.
 */
void disable_mouse_tracking(void)
{
  wprintf(L"\033[?1006l\033[?1000l");
  fflush(stdout);
}

/* Function: enable_raw_mode
 * Switches the terminal to raw mode so individual characters and escape
 * sequences (including mouse reports) can be read without waiting for Enter.
 * Saves the original terminal settings into *orig so they can be restored later.
 */
void enable_raw_mode(struct termios *orig)
{
  tcgetattr(STDIN_FILENO, orig);
  struct termios raw = *orig;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* Function: disable_raw_mode
 * Restores the terminal settings saved by enable_raw_mode.
 */
void disable_raw_mode(struct termios *orig)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, orig);
}

/* Function: read_position_from_input
 * Reads a chess board position from the user, accepting either:
 *   - A typed two-character coordinate (e.g. "e2") followed by Enter, OR
 *   - A mouse click on a board square.
 *
 * Parameters:
 * - pos: Output buffer (at least 3 bytes) to store the position string.
 * - is_white_perspective: 1 if the board is displayed from white's perspective, 0 for black's.
 *
 * Returns 1 on success (buf is filled with a valid coordinate like "e2",
 * or a command like "save" or "history"),
 * or 0 if the input was invalid / could not be parsed.
 *
 * Board layout from white's perspective (terminal 1-indexed):
 *   Row header line: line 1 = column labels
 *   Border line:     line 2
 *   Board row i=0:   line 3  (rank 8)
 *   Border:          line 4
 *   Board row i=1:   line 5  (rank 7)
 *   ...
 *   Board row i=7:   line 17 (rank 1)
 *
 *   Column j=0 piece char at terminal column ~4, j=1 at ~8, etc.
 *   General: piece at column 2 + j*4 .. 4 + j*4 (the ' X ' part after '|')
 *
 * For black's perspective rows and columns are reversed.
 */
int read_position_from_input(char *buf, int buf_size, int is_white_perspective)
{
  struct termios orig;
  enable_raw_mode(&orig);
  enable_mouse_tracking();

  char kbd_buf[16] = {0};
  int kbd_len = 0;
  int result = 0;

  while (1)
  {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
      continue;

    // Detect escape sequence (mouse report or other)
    if (c == '\033')
    {
      char seq[32];
      int seq_len = 0;
      // Read the '[' character
      if (read(STDIN_FILENO, &seq[seq_len], 1) == 1)
      {
        seq_len++;
        if (seq[0] == '[')
        {
          // Read until we get 'M' or 'm' (SGR mouse) or run out of buffer
          while (seq_len < (int)sizeof(seq) - 1)
          {
            if (read(STDIN_FILENO, &seq[seq_len], 1) != 1)
              break;
            seq_len++;
            char last = seq[seq_len - 1];
            if (last == 'M' || last == 'm')
              break;
            // Also break on other final bytes (letters) that aren't part of mouse
            if ((last >= 'A' && last <= 'Z' && last != 'M') ||
                (last >= 'a' && last <= 'z' && last != 'm'))
              break;
          }
          seq[seq_len] = '\0';

          // Parse SGR mouse: \033[<button;col;row M/m
          // 'M' = press, 'm' = release. We act on press (M).
          if (seq[1] == '<' && seq[seq_len - 1] == 'M')
          {
            int button, term_col, term_row;
            if (sscanf(seq + 2, "%d;%d;%d", &button, &term_col, &term_row) == 3)
            {
              // Only handle left-click (button 0)
              if (button == 0)
              {
                // Convert terminal coordinates to board indices
                // Board content lines are at term_row = 3,5,7,...,17 (rows 0-7)
                // term_row must be odd and >= 3 and <= 17
                if (term_row >= 3 && term_row <= 17 && (term_row % 2 == 1))
                {
                  int display_row = (term_row - 3) / 2; // 0..7
                  // Columns: the cell area for display col k is at term_col 3+k*4 to 5+k*4
                  // (after the '|' at 2+k*4, the content is at 3+k*4, 4+k*4 (icon may be wide), then '|' at 6+k*4)
                  // More precisely: '|' at 2, content 3-4-5 area, '|' at 6 for k=0
                  // Actually let's look: "%d|" takes 2 chars (e.g. "8|"), then " X |" repeats
                  // col 1: row_num, col 2: '|', col 3: ' ', col 4: piece, col 5: ' ', col 6: '|'
                  // col 7: ' ', col 8: piece, col 9: ' ', col 10: '|', ...
                  // So cell k (0-indexed) spans term_col 3+k*4 to 5+k*4, with piece at 4+k*4
                  // Valid click range: term_col >= 3 and term_col <= 34
                  if (term_col >= 3 && term_col <= 34)
                  {
                    int display_col = (term_col - 3) / 4; // 0..7
                    if (display_col >= 0 && display_col <= 7)
                    {
                      int board_row, board_col;
                      if (is_white_perspective)
                      {
                        board_row = display_row;
                        board_col = display_col;
                      }
                      else
                      {
                        board_row = 7 - display_row;
                        board_col = 7 - display_col;
                      }

                      // Convert board indices to chess notation
                      buf[0] = 'a' + board_col;
                      buf[1] = '8' - board_row;
                      buf[2] = '\0';
                      // Echo the coordinate next to the prompt
                      wprintf(L"%c%c", buf[0], buf[1]);
                      fflush(stdout);
                      result = 1;
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
      // Ignore other escape sequences and continue
      continue;
    }

    // Handle Enter key - submit typed input
    if (c == '\n' || c == '\r')
    {
      if (kbd_len > 0)
      {
        kbd_buf[kbd_len] = '\0';
        int copy_len = kbd_len < buf_size - 1 ? kbd_len : buf_size - 1;
        memcpy(buf, kbd_buf, copy_len);
        buf[copy_len] = '\0';
        result = 1;
        break;
      }
      // Empty input
      kbd_len = 0;
      result = 0;
      break;
    }

    // Handle backspace
    if (c == 127 || c == '\b')
    {
      if (kbd_len > 0)
      {
        kbd_len--;
        // Erase the character on screen
        wprintf(L"\b \b");
        fflush(stdout);
      }
      continue;
    }

    // Accumulate regular characters for keyboard input
    if (kbd_len < (int)sizeof(kbd_buf) - 1)
    {
      kbd_buf[kbd_len++] = c;
      // Echo the character
      wprintf(L"%c", c);
      fflush(stdout);
    }
  }

  disable_mouse_tracking();
  disable_raw_mode(&orig);
  return result;
}

void get_move(Piece_t board[8][8], Captures_node_t **pp_capture_color_head, Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t **pp_history_head, int *captured_king, int *moves, GameState_t *state)
{
  char prev_pos[16];
  char next_pos[16];
  int prev_i, prev_j, next_i, next_j;
  int is_white = (*moves % 2 != 0);

  // Get the piece to move
  while (1)
  {
    wprintf(L"Enter the position of the piece you want to move: ");
    fflush(stdout);
    if (read_position_from_input(prev_pos, sizeof(prev_pos), is_white))
    {
      wprintf(L"\n");

      // Handle "save" command
      if (strcmp(prev_pos, "save") == 0)
      {
        save_game(board, p_captures_white_head, p_captures_black_head, *pp_history_head, *moves, state);
        exit(0);
      }

      // Handle "history" command
      if (strcmp(prev_pos, "history") == 0)
      {
        wprintf(L"\033[H\033[2J\033[3J");
        print_history(*pp_history_head);
        wprintf(L"\nPress Enter to return to the game...");
        fflush(stdout);
        read_single_char();
        redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, NULL);
        continue;
      }

      if (strlen(prev_pos) == 2 && prev_pos[0] >= 'a' && prev_pos[0] <= 'h' && prev_pos[1] >= '1' && prev_pos[1] <= '8')
      {
        // Check if the selected piece is of the correct color
        if (find_piece_coordinates(board, prev_pos, &prev_i, &prev_j) && board[prev_i][prev_j].type != FREE && board[prev_i][prev_j].color == (is_white ? WHITE : BLACK))
        {
          // Show valid move indicators on the board
          show_valid_moves(board, prev_i, prev_j, state);
          redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, NULL);
          clear_valid_moves(board);
          break;
        }
        else
        {
          redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, "Invalid selection. Please select a piece of the correct color.");
        }
      }
      else
      {
        redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, "Invalid input. Please enter a valid position, 'save', or 'history'.");
      }
    }
    else
    {
      redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, NULL);
    }
  }

  // Get the next position
  while (1)
  {
    wprintf(L"Enter the position where you want to move the piece: ");
    fflush(stdout);
    if (read_position_from_input(next_pos, sizeof(next_pos), is_white))
    {
      wprintf(L"\n");

      // Handle "save" command
      if (strcmp(next_pos, "save") == 0)
      {
        save_game(board, p_captures_white_head, p_captures_black_head, *pp_history_head, *moves, state);
        exit(0);
      }

      // Handle "history" command
      if (strcmp(next_pos, "history") == 0)
      {
        wprintf(L"\033[H\033[2J\033[3J");
        print_history(*pp_history_head);
        wprintf(L"\nPress Enter to return to the game...");
        fflush(stdout);
        read_single_char();
        show_valid_moves(board, prev_i, prev_j, state);
        redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, NULL);
        clear_valid_moves(board);
        continue;
      }

      if (strlen(next_pos) == 2 && next_pos[0] >= 'a' && next_pos[0] <= 'h' && next_pos[1] >= '1' && next_pos[1] <= '8')
      {
        break;
      }
      show_valid_moves(board, prev_i, prev_j, state);
      redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, "Invalid input. Please enter a valid position, 'save', or 'history'.");
      clear_valid_moves(board);
    }
    else
    {
      show_valid_moves(board, prev_i, prev_j, state);
      redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, NULL);
      clear_valid_moves(board);
    }
  }

  // Get the coordinates of the next position
  find_piece_coordinates(board, next_pos, &next_i, &next_j);

  // Check if the move is valid
  if (is_valid_move(board, prev_i, prev_j, next_i, next_j, state))
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

      // If a rook is captured, disable castling for that rook
      if (board[next_i][next_j].type == ROOK)
      {
        if (next_i == 7 && next_j == 0) state->white_rook_a_moved = 1;
        if (next_i == 7 && next_j == 7) state->white_rook_h_moved = 1;
        if (next_i == 0 && next_j == 0) state->black_rook_a_moved = 1;
        if (next_i == 0 && next_j == 7) state->black_rook_h_moved = 1;
      }
    }

    // Handle en passant capture
    if (board[prev_i][prev_j].type == PAWN && next_j != prev_j && board[next_i][next_j].type == FREE)
    {
      // This is an en passant capture - remove the captured pawn
      update_captures(pp_capture_color_head, board[prev_i][next_j]);
      board[prev_i][next_j].icon = L' ';
      board[prev_i][next_j].color = NONE;
      board[prev_i][next_j].type = FREE;
    }

    // Track castling state changes
    if (board[prev_i][prev_j].type == KING)
    {
      if (board[prev_i][prev_j].color == WHITE)
        state->white_king_moved = 1;
      else
        state->black_king_moved = 1;

      // Handle castling move - also move the rook
      if (abs(next_j - prev_j) == 2)
      {
        if (next_j > prev_j)
        {
          // Kingside castling - move rook from h to f
          update_board(board, prev_i, 7, prev_i, 5);
        }
        else
        {
          // Queenside castling - move rook from a to d
          update_board(board, prev_i, 0, prev_i, 3);
        }
      }
    }
    if (board[prev_i][prev_j].type == ROOK)
    {
      if (prev_i == 7 && prev_j == 0) state->white_rook_a_moved = 1;
      if (prev_i == 7 && prev_j == 7) state->white_rook_h_moved = 1;
      if (prev_i == 0 && prev_j == 0) state->black_rook_a_moved = 1;
      if (prev_i == 0 && prev_j == 7) state->black_rook_h_moved = 1;
    }

    // Track en passant eligibility for next turn
    if (board[prev_i][prev_j].type == PAWN && abs(next_i - prev_i) == 2)
    {
      state->en_passant_col = prev_j;
    }
    else
    {
      state->en_passant_col = -1;
    }

    // Update the board
    update_board(board, prev_i, prev_j, next_i, next_j);

    // Handle pawn promotion
    if (board[next_i][next_j].type == PAWN && (next_i == 0 || next_i == 7))
    {
      handle_promotion(board, next_i, next_j, p_captures_white_head, p_captures_black_head, is_white);
    }

    // Update the history
    update_history(pp_history_head, prev_pos, next_pos);
  }
  else
  {
    redraw_screen(board, p_captures_white_head, p_captures_black_head, is_white, "Invalid move. Please try again.");
    get_move(board, pp_capture_color_head, p_captures_white_head, p_captures_black_head, pp_history_head, captured_king, moves, state);
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
int is_valid_move(Piece_t board[8][8], int prev_i, int prev_j, int next_i, int next_j, GameState_t *state)
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
      // En passant capture for white (white moves up, so row 3 = index 3)
      if (prev_i == 3 && next_i == 2 && (next_j == prev_j - 1 || next_j == prev_j + 1) &&
          board[next_i][next_j].type == FREE && state->en_passant_col == next_j &&
          board[prev_i][next_j].type == PAWN && board[prev_i][next_j].color == BLACK)
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
      // En passant capture for black (black moves down, so row 4 = index 4)
      if (prev_i == 4 && next_i == 5 && (next_j == prev_j - 1 || next_j == prev_j + 1) &&
          board[next_i][next_j].type == FREE && state->en_passant_col == next_j &&
          board[prev_i][next_j].type == PAWN && board[prev_i][next_j].color == WHITE)
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
    // Castling
    if (next_i == prev_i && abs(next_j - prev_j) == 2)
    {
      Color color = board[prev_i][prev_j].color;
      int king_moved = (color == WHITE) ? state->white_king_moved : state->black_king_moved;
      if (king_moved)
        return 0;

      if (next_j > prev_j)
      {
        // Kingside castling
        int rook_moved = (color == WHITE) ? state->white_rook_h_moved : state->black_rook_h_moved;
        if (rook_moved)
          return 0;
        // Check rook is present
        if (board[prev_i][7].type != ROOK || board[prev_i][7].color != color)
          return 0;
        // Check squares between king and rook are empty
        if (board[prev_i][5].type != FREE || board[prev_i][6].type != FREE)
          return 0;
        return 1;
      }
      else
      {
        // Queenside castling
        int rook_moved = (color == WHITE) ? state->white_rook_a_moved : state->black_rook_a_moved;
        if (rook_moved)
          return 0;
        // Check rook is present
        if (board[prev_i][0].type != ROOK || board[prev_i][0].color != color)
          return 0;
        // Check squares between king and rook are empty
        if (board[prev_i][1].type != FREE || board[prev_i][2].type != FREE || board[prev_i][3].type != FREE)
          return 0;
        return 1;
      }
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
int save_game(Piece_t board[8][8], Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, History_node_t *p_history_head, int moves, GameState_t *state)
{
  FILE *file = fopen("game_save.bin", "wb");
  if (file == NULL)
  {
    wprintf(L"Error opening file for saving.\n");
    return 0;
  }

  // Save the number of moves
  fwrite(&moves, sizeof(int), 1, file);

  // Save the game state (castling rights, en passant)
  fwrite(state, sizeof(GameState_t), 1, file);

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
int load_game(Piece_t board[8][8], Captures_node_t **p_captures_white_head, Captures_node_t **p_captures_black_head, History_node_t **p_history_head, int *moves, GameState_t *state)
{
  FILE *file = fopen("game_save.bin", "rb");
  if (file == NULL)
  {
    wprintf(L"Error opening file for loading.\n");
    return 0;
  }

  // Load the number of moves
  fread(moves, sizeof(int), 1, file);

  // Load the game state (castling rights, en passant)
  fread(state, sizeof(GameState_t), 1, file);

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
/* Function: show_valid_moves
 * Temporarily places a dot marker () on all empty squares that the selected
 * piece can legally move to. Squares occupied by enemy pieces are left as-is
 * since the player can already see them as capturable targets.
 */
void show_valid_moves(Piece_t board[8][8], int piece_i, int piece_j, GameState_t *state)
{
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (i == piece_i && j == piece_j)
        continue;
      if (board[i][j].type == FREE && is_valid_move(board, piece_i, piece_j, i, j, state))
      {
        board[i][j].icon = L''; //  Black Circle
      }
    }
  }
}

/* Function: clear_valid_moves
 * Removes the dot markers placed by show_valid_moves, restoring empty squares
 * back to a space character.
 */
void clear_valid_moves(Piece_t board[8][8])
{
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      if (board[i][j].type == FREE && board[i][j].icon != L' ')
      {
        board[i][j].icon = L' ';
      }
    }
  }
}

void init_game_state(GameState_t *state)
{
  state->white_king_moved = 0;
  state->black_king_moved = 0;
  state->white_rook_a_moved = 0;
  state->white_rook_h_moved = 0;
  state->black_rook_a_moved = 0;
  state->black_rook_h_moved = 0;
  state->en_passant_col = -1;
}

/* Function: handle_promotion
 * Prompts the player to choose a piece for pawn promotion.
 * The pawn at board[row][col] is replaced with the chosen piece.
 */
void handle_promotion(Piece_t board[8][8], int row, int col, Captures_node_t *p_captures_white_head, Captures_node_t *p_captures_black_head, int is_white)
{
  Color color = board[row][col].color;
  wchar_t queen_icon, rook_icon, bishop_icon, knight_icon;

  if (color == WHITE)
  {
    queen_icon = L'󰡚';
    rook_icon = L'󰡛';
    bishop_icon = L'󰡜';
    knight_icon = L'󰡘';
  }
  else
  {
    queen_icon = L'\uED65';
    rook_icon = L'\uED66';
    bishop_icon = L'\uED60';
    knight_icon = L'\uED63';
  }

  // Redraw the board to show the pawn at the promotion square
  wprintf(L"\033[H\033[2J\033[3J");
  if (is_white)
    print_board_white(board);
  else
    print_board_black(board);
  wprintf(L"\n");
  print_captures(p_captures_white_head);
  print_captures(p_captures_black_head);

  wprintf(L"\nPawn promotion! Choose a piece:\n");
  wprintf(L"  1. Queen  (%lc)\n", queen_icon);
  wprintf(L"  2. Rook   (%lc)\n", rook_icon);
  wprintf(L"  3. Bishop (%lc)\n", bishop_icon);
  wprintf(L"  4. Knight (%lc)\n", knight_icon);
  wprintf(L"Enter 1-4: ");
  fflush(stdout);

  char choice;
  while (1)
  {
    choice = read_single_char();
    if (choice >= '1' && choice <= '4')
    {
      wprintf(L"%c\n", choice);
      fflush(stdout);
      break;
    }
  }

  switch (choice)
  {
  case '1':
    board[row][col].type = QUEEN;
    board[row][col].icon = queen_icon;
    break;
  case '2':
    board[row][col].type = ROOK;
    board[row][col].icon = rook_icon;
    break;
  case '3':
    board[row][col].type = BISHOP;
    board[row][col].icon = bishop_icon;
    break;
  case '4':
    board[row][col].type = KNIGHT;
    board[row][col].icon = knight_icon;
    break;
  }
}

void init_board(Piece_t board[8][8])
{
  Piece_t temp_board[8][8] = {
      {{L'', BLACK, "a8", ROOK},
       {L'', BLACK, "b8", KNIGHT},
       {L'', BLACK, "c8", BISHOP},
       {L'', BLACK, "d8", QUEEN},
       {L'', BLACK, "e8", KING},
       {L'', BLACK, "f8", BISHOP},
       {L'', BLACK, "g8", KNIGHT},
       {L'', BLACK, "h8", ROOK}},
      {{L'', BLACK, "a7", PAWN},
       {L'', BLACK, "b7", PAWN},
       {L'', BLACK, "c7", PAWN},
       {L'', BLACK, "d7", PAWN},
       {L'', BLACK, "e7", PAWN},
       {L'', BLACK, "f7", PAWN},
       {L'', BLACK, "g7", PAWN},
       {L'', BLACK, "h7", PAWN}},
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
      {{L'󰡙', WHITE, "a2", PAWN},
       {L'󰡙', WHITE, "b2", PAWN},
       {L'󰡙', WHITE, "c2", PAWN},
       {L'󰡙', WHITE, "d2", PAWN},
       {L'󰡙', WHITE, "e2", PAWN},
       {L'󰡙', WHITE, "f2", PAWN},
       {L'󰡙', WHITE, "g2", PAWN},
       {L'󰡙', WHITE, "h2", PAWN}},
      {{L'󰡛', WHITE, "a1", ROOK},
       {L'󰡘', WHITE, "b1", KNIGHT},
       {L'󰡜', WHITE, "c1", BISHOP},
       {L'󰡚', WHITE, "d1", QUEEN},
       {L'󰡗', WHITE, "e1", KING},
       {L'󰡜', WHITE, "f1", BISHOP},
       {L'󰡘', WHITE, "g1", KNIGHT},
       {L'󰡛', WHITE, "h1", ROOK}}};

  memcpy(board, temp_board, sizeof(temp_board));
}
