#include "app/game.h"

#include "app/promotion.h"
#include "app/save.h"
#include "core/board.h"
#include "core/history.h"
#include "core/movegen.h"
#include "core/notation.h"
#include "core/outcome.h"
#include "core/position.h"
#include "ui/glyphs.h"
#include "ui/interaction.h"
#include "ui/layout.h"
#include "ui/render.h"
#include "ui/term.h"
#include "version.h"

#include <stdio.h>
#include <string.h>

/* --- Palette -------------------------------------------------------------
 *
 * xterm-256 indices. Squares carry no colour of their own — a checkerboard of
 * filled cells reads as a wall of colour in a terminal and fights whatever
 * theme it already has; the grid rules are enough to say where a square ends.
 * Background is spent only on what is worth spotting instantly: the piece
 * picked up, the move just played, where a selected piece may capture, and a
 * king under attack.
 *
 * All four are deep, unsaturated shades rather than bright ones: a highlight
 * should read as the square having been tinted, not as a block of colour laid
 * over the board. Dark enough, too, that the piece colours below work on top
 * of them and need no special case.
 *
 * Every tint has a shape fallback alongside it, used instead of the tint
 * whenever term_supports_color() says colour is not worth spending — a corner
 * glyph in one of the two cells of padding every square has around its piece,
 * which is exactly wide enough for a marker without ever touching the piece
 * itself. Check is the one state that gets both at once, in either mode: it
 * is the one worth over-signalling.
 */
#define C_SQUARE_LAST 22      /* dark green: the move just played */
#define C_SQUARE_SELECTED 58  /* dark olive: the piece picked up */
#define C_SQUARE_CAPTURE 52   /* dark red: a legal capture for the selection */
#define C_SQUARE_CHECK 130    /* dark amber: the checked king's square */
#define C_MARK_CHECK_FG 208   /* orange: the check corner mark, both modes */
#define C_PIECE_WHITE 231
/* Black on an unbackgrounded square would be invisible on a dark terminal, so
 * it sits at a grey that reads against either. */
#define C_PIECE_BLACK 245
#define C_RULE 244
#define C_LABEL 246
#define C_HINT 244

/* Legal, empty destination: centred, shape-based already, so it is identical
 * in colour and monochrome mode. */
#define MARK_DOT 0x25CFu /* ● */
/* Monochrome fallbacks, left padding cell — mutually exclusive with each
 * other, since at most one of selected/capture/last-move ever applies to a
 * given square. */
#define MARK_SELECTED 0x00BBu /* » */
#define MARK_CAPTURE 0x00D7u  /* × */
#define MARK_LAST 0x00B7u     /* · */
/* Check, right padding cell — additive: drawn whenever the king is in check,
 * in both colour and monochrome mode, regardless of what else the square is
 * showing. */
#define MARK_CHECK '!'

typedef struct {
  GameState *state;

  int flipped; /* the board is drawn from black's side */

  /* The square picked up, and the move just played. Both -1 when there is none.
   * The last move is kept and drawn every frame, which is what replaces the
   * one-second pause the old loop used: seeing your move is no longer a race
   * against a timer, because the move stays on the board. */
  int sel_i, sel_j;
  int last_from_i, last_from_j;
  int last_to_i, last_to_j;

  /* The square being typed, shown in the status bar as it is entered. */
  char typed[3];
  int typed_len;

  /* The keyboard cursor: a second, independent way to name a square, moved by
   * the arrow keys and read on Enter when nothing has been typed. Held in
   * screen space (0-7, top-left of what is currently drawn) rather than board
   * indices, so an arrow key moves the cursor where it visibly points
   * regardless of orientation — the same reasoning point_to_square applies to
   * a click. */
  int cursor_row, cursor_col;

  /* The layout the most recent frame was drawn with, so a click is hit-tested
   * against the geometry it actually saw rather than one recomputed — possibly
   * differently — after the fact. */
  Layout layout;
  int layout_valid;

  /* Whether this terminal is worth spending colour on, decided once at entry
   * from the environment; see term_supports_color(). */
  int use_color;

  /* Between turns: the board has flipped and the next player has not yet said
   * they are looking. In pass-and-play that gesture is the handoff, and a human
   * paces it better than a constant does. */
  int awaiting_handoff;

  int game_over;
  char message[96];
} Game_t;

static Game_t g_game;

/* The move a promotion overlay is waiting on: everything finish_move() needs
 * once the player answers which piece to become. Global for the same reason
 * g_game is: the app owns exactly one game screen and, transitively, at most
 * one promotion overlay at a time. */
typedef struct {
  Game_t *game;
  Move choices[4];
  int count;
} Promotion_request_t;

static Promotion_request_t g_promo_request;

static Color side_to_move(const Game_t *g) { return g->state->position.side_to_move; }

static void clear_entry(Game_t *g) {
  g->typed[0] = '\0';
  g->typed_len = 0;
  g->sel_i = -1;
  g->sel_j = -1;
}

/* --- Saving and loading, temporarily -------------------------------------
 *
 * Both bindings belong to app-shell-and-persistence, which replaces the file
 * format, saves after every move, and offers a resume prompt on launch. They
 * exist here only so the capability is not dark between the two changes.
 * Delete this section, the app/save.h include, and the hint-line entries when
 * that change lands.
 */

/* A game with nothing in it yet: the opening position, no moves played. */
static int game_is_untouched(const Game_t *g) { return g->state->p_history_head == NULL; }

/* Loading is offered only on an untouched game.
 *
 * A load replaces everything, and `l` sits one key away from the squares a
 * player spends the game typing. Restricting it to a game with no moves in it
 * means a mistyped key can cost at most a game that had not started. */
static int can_load(const Game_t *g) {
  return !g->game_over && game_is_untouched(g);
}

/* Saving is the mirror image, and refused on an untouched game.
 *
 * There is nothing in the opening position worth keeping, and writing it would
 * destroy a real save — the one the player is one keystroke away from loading.
 * The two commands are therefore never available at the same time. */
static int can_save(const Game_t *g) {
  return !game_is_untouched(g);
}

static const char *load_message(Load_result_t result) {
  switch (result) {
  case LOAD_OK:
    return "Game loaded.";
  case LOAD_NO_FILE:
    return "No saved game found.";
  case LOAD_WRONG_FORMAT:
    return "That save was written by an incompatible build.";
  case LOAD_CORRUPT:
    return "The save file is incomplete or corrupted.";
  }
  return "Could not load the saved game.";
}

/* The loaded game's last move, so a resumed game shows the same highlight a
 * played one would. */
static void highlight_last_move(Game_t *g) {
  g->last_from_i = -1;
  g->last_from_j = -1;
  g->last_to_i = -1;
  g->last_to_j = -1;

  History_node_t *last = NULL;
  for (History_node_t *p = g->state->p_history_head; p != NULL; p = p->p_next) {
    last = p;
  }
  if (last == NULL) {
    return;
  }
  square_to_index(last->prev_pos, &g->last_from_i, &g->last_from_j);
  square_to_index(last->next_pos, &g->last_to_i, &g->last_to_j);
}

static void load_into(Game_t *g) {
  /* load_game overwrites the state wholesale on success, so the lists it
   * replaces are reachable only through heads taken beforehand. */
  Captures_node_t *old_white = g->state->p_captures_white_head;
  Captures_node_t *old_black = g->state->p_captures_black_head;
  History_node_t *old_history = g->state->p_history_head;
  Hash_node_t *old_hashes = g->state->p_hash_history_head;

  Load_result_t result = load_game(g->state, NULL);
  snprintf(g->message, sizeof(g->message), "%s", load_message(result));
  if (result != LOAD_OK) {
    return;
  }

  free_captures(old_white);
  free_captures(old_black);
  free_history(old_history);
  free_hash_history(old_hashes);
  /* A FEN save carries no move history, so repetition is tracked fresh from
   * the loaded position: any repetition claim from before the save is lost,
   * a limitation of this interim format. */
  g->state->p_hash_history_head = NULL;
  push_hash(&g->state->p_hash_history_head, g->state->position.hash);

  clear_entry(g);
  g->awaiting_handoff = 0;
  g->game_over = 0;
  g->flipped = (side_to_move(g) == BLACK);
  highlight_last_move(g);
}

/* --- Rendering ---------------------------------------------------------- */

/* The one square per side that can be "in check", found by scanning: nothing
 * in core/ hands this back directly, and an 8x8 scan is cheap enough to do
 * every frame rather than worth threading through movegen's contract for. */
static int find_king(const Position *pos, Color side, int *i, int *j) {
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (pos->board[r][c].type == KING && pos->board[r][c].color == side) {
        *i = r;
        *j = c;
        return 1;
      }
    }
  }
  return 0;
}

/* Which of the three mutually exclusive background states a square is in.
 * Priority breaks the one case where two could apply at once: reselecting the
 * piece that was itself the destination of the last move. Selection is the
 * more urgent thing to see. */
typedef enum { SQ_NONE, SQ_SELECTED, SQ_CAPTURE_DEST, SQ_LAST_MOVE } Square_priority_t;

static Square_priority_t square_priority(const Game_t *g, int i, int j, int is_capture_dest) {
  if (i == g->sel_i && j == g->sel_j) {
    return SQ_SELECTED;
  }
  if (is_capture_dest) {
    return SQ_CAPTURE_DEST;
  }
  if ((i == g->last_from_i && j == g->last_from_j) ||
      (i == g->last_to_i && j == g->last_to_j)) {
    return SQ_LAST_MOVE;
  }
  return SQ_NONE;
}

static int priority_bg(Square_priority_t p) {
  switch (p) {
  case SQ_SELECTED:
    return C_SQUARE_SELECTED;
  case SQ_CAPTURE_DEST:
    return C_SQUARE_CAPTURE;
  case SQ_LAST_MOVE:
    return C_SQUARE_LAST;
  case SQ_NONE:
    break;
  }
  return COLOR_DEFAULT;
}

/* The monochrome fallback for priority_bg: a shape in the square's left
 * padding cell instead of a tint, since the three states above are as
 * mutually exclusive as the tints they replace. 0 means nothing to draw. */
static uint32_t priority_mark(Square_priority_t p) {
  switch (p) {
  case SQ_SELECTED:
    return MARK_SELECTED;
  case SQ_CAPTURE_DEST:
    return MARK_CAPTURE;
  case SQ_LAST_MOVE:
    return MARK_LAST;
  case SQ_NONE:
    break;
  }
  return 0;
}

static int piece_fg(Color color) {
  return (color == BLACK) ? C_PIECE_BLACK : C_PIECE_WHITE;
}

static void draw_board(const Game_t *g, Rect r, const Layout *lay) {
  int gw = glyphs_width();
  int sq_w = lay->square_w;
  int grid_x = lay->grid_x;
  int grid_y = lay->grid_y;
  int grid_w = 8 * (sq_w + 1) + 1;

  /* Legal destinations of the selected piece, queried fresh every frame
   * rather than cached: cheap at human speed, and it makes a stale-cache bug
   * — a square that looks available but is refused — structurally
   * impossible. The set drawn is exactly the generator's output. */
  MoveList legal;
  int has_legal = g->sel_i >= 0;
  if (has_legal) {
    generate_legal_moves_from(&g->state->position, g->sel_i, g->sel_j, &legal);
  }

  int king_i = -1, king_j = -1;
  int check_active = find_king(&g->state->position, side_to_move(g), &king_i, &king_j) &&
                      in_check(&g->state->position, side_to_move(g));

  /* File labels above and below, centred over their columns. The lower row sits
   * one line below the closing rule rather than on it — most of the moves a
   * player types are for pieces near the bottom of the board, so that is the
   * copy they will actually read. */
  for (int f = 0; f < 8; f++) {
    int file = g->flipped ? 7 - f : f;
    char label[2] = {(char)('a' + file), '\0'};
    int x = grid_x + f * (sq_w + 1) + 1 + sq_w / 2;
    /* Reversed video on the cursor's own column, in both rows — a shape-based
     * highlight, distinct from any tint, that touches only the labels and
     * never the board's interior. */
    uint8_t attr = (f == g->cursor_col) ? ATTR_REVERSE : ATTR_NONE;
    draw_text(r, x, 0, label, C_LABEL, COLOR_DEFAULT, attr);
    draw_text(r, x, grid_y + 8 * 2 + 1, label, C_LABEL, COLOR_DEFAULT, attr);
  }

  /* Horizontal rules, one above each rank and one below the last. */
  for (int row = 0; row <= 8; row++) {
    int y = grid_y + row * 2;
    for (int x = 0; x < grid_w; x++) {
      int on_junction = ((x % (sq_w + 1)) == 0);
      uint32_t ch = 0x2500u; /* ─ */
      if (on_junction) {
        int left = (x == 0);
        int right = (x == grid_w - 1);
        if (row == 0) {
          ch = left ? 0x250Cu : right ? 0x2510u : 0x252Cu;
        } else if (row == 8) {
          ch = left ? 0x2514u : right ? 0x2518u : 0x2534u;
        } else {
          ch = left ? 0x251Cu : right ? 0x2524u : 0x253Cu;
        }
      }
      draw_glyph(r, grid_x + x, y, ch, 1, C_RULE, COLOR_DEFAULT, ATTR_NONE);
    }
  }

  for (int row = 0; row < 8; row++) {
    int i = g->flipped ? 7 - row : row;
    int y = grid_y + row * 2 + 1;

    char rank[2] = {(char)('8' - i), '\0'};
    uint8_t rank_attr = (row == g->cursor_row) ? ATTR_REVERSE : ATTR_NONE;
    draw_text(r, 0, y, rank, C_LABEL, COLOR_DEFAULT, rank_attr);
    draw_text(r, grid_x + grid_w + 1, y, rank, C_LABEL, COLOR_DEFAULT, rank_attr);

    for (int col = 0; col < 8; col++) {
      int j = g->flipped ? 7 - col : col;
      int x = grid_x + col * (sq_w + 1);

      draw_glyph(r, x, y, 0x2502u, 1, C_RULE, COLOR_DEFAULT, ATTR_NONE);

      int is_capture_dest = 0, is_quiet_dest = 0;
      if (has_legal) {
        for (int k = 0; k < legal.count; k++) {
          if (legal.moves[k].to_i == i && legal.moves[k].to_j == j) {
            /* move.captured covers en passant too — set to PAWN even though
             * the destination square itself is empty — so this needs no
             * special case to mark it as a capture rather than a quiet move. */
            if (legal.moves[k].captured != FREE) {
              is_capture_dest = 1;
            } else {
              is_quiet_dest = 1;
            }
            break;
          }
        }
      }

      Square_priority_t prio = square_priority(g, i, j, is_capture_dest);
      int is_check_sq = check_active && i == king_i && j == king_j;

      int bg = COLOR_DEFAULT;
      if (g->use_color) {
        bg = priority_bg(prio);
        if (prio == SQ_NONE && is_check_sq) {
          bg = C_SQUARE_CHECK;
        }
      }

      Piece_t piece = g->state->position.board[i][j];
      int fg = piece_fg(piece.color);

      for (int k = 0; k < sq_w; k++) {
        draw_glyph(r, x + 1 + k, y, ' ', 1, fg, bg, ATTR_NONE);
      }
      if (piece.type != FREE) {
        /* Centred on the square, at whatever width the terminal was measured
         * to draw the glyph, so the columns line up either way. */
        draw_glyph(r, x + 1 + (sq_w - gw) / 2, y, piece_glyph(piece.type, piece.color),
                   gw, fg, bg, ATTR_NONE);
      }

      /* Legal-quiet-destination dot: shape-based, so it is identical in
       * colour and monochrome mode, and independent of whichever background
       * state this square is otherwise in — a vacated square can be both the
       * last move and a legal destination at once. */
      if (is_quiet_dest) {
        int dot_x = x + 1 + (sq_w - 1) / 2;
        int mark_fg = g->use_color ? C_HINT : COLOR_DEFAULT;
        draw_glyph(r, dot_x, y, MARK_DOT, 1, mark_fg, bg, ATTR_NONE);
      }

      /* Monochrome fallback for the background state: a shape in the left
       * padding cell, which the piece glyph never reaches. */
      if (!g->use_color) {
        uint32_t mark = priority_mark(prio);
        if (mark != 0) {
          draw_glyph(r, x + 1, y, mark, 1, COLOR_DEFAULT, bg, ATTR_NONE);
        }
      }

      /* Check: additive, in both modes, in the right padding cell — the one
       * marking that is never only a fallback. */
      if (is_check_sq) {
        int mark_fg = g->use_color ? C_MARK_CHECK_FG : COLOR_DEFAULT;
        draw_glyph(r, x + sq_w, y, MARK_CHECK, 1, mark_fg, bg, ATTR_BOLD);
      }
    }
    draw_glyph(r, grid_x + grid_w - 1, y, 0x2502u, 1, C_RULE, COLOR_DEFAULT, ATTR_NONE);
  }
}

static int draw_capture_row(Rect r, int y, const char *label, Captures_node_t *head) {
  draw_text(r, 0, y, label, C_LABEL, COLOR_DEFAULT, ATTR_NONE);
  int x = 0;
  int gw = glyphs_width();
  for (Captures_node_t *p = head; p != NULL; p = p->p_next) {
    if (x + gw > r.w) {
      break; /* the rest is cut off rather than spilling onto the next line */
    }
    draw_glyph(r, x, y + 1, piece_glyph(p->piece.type, p->piece.color), gw,
               piece_fg(p->piece.color), COLOR_DEFAULT, ATTR_NONE);
    x += gw + 1;
  }
  return y + 2;
}

static void draw_panel(const Game_t *g, Rect r) {
  if (r.w < 8 || r.h < 4) {
    return;
  }
  Rect inner = rect_sub(r, 1, 0, r.w - 1, r.h);

  int y = 0;
  y = draw_capture_row(inner, y, "Taken by White", g->state->p_captures_white_head);
  y = draw_capture_row(inner, y, "Taken by Black", g->state->p_captures_black_head);

  y++;
  draw_text(inner, 0, y, "Moves", C_LABEL, COLOR_DEFAULT, ATTR_NONE);
  y++;

  /* The history list is oldest first and the interesting end is the newest, so
   * the tail that fits is what is shown. The scrollable full list is a screen
   * of its own in a later change. */
  int room = inner.h - y;
  if (room < 1) {
    return;
  }
  int total = 0;
  for (History_node_t *p = g->state->p_history_head; p != NULL; p = p->p_next) {
    total++;
  }
  int skip = total > room ? total - room : 0;

  int n = 0;
  char line[32];
  for (History_node_t *p = g->state->p_history_head; p != NULL; p = p->p_next, n++) {
    if (n < skip) {
      continue;
    }
    snprintf(line, sizeof(line), "%3d. %s-%s", n + 1, p->prev_pos, p->next_pos);
    draw_text(inner, 0, y + (n - skip), line, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);
  }
}

static void draw_status(const Game_t *g, Rect r) {
  char line[160];
  char mover_label[24];
  const char *mover = (side_to_move(g) == WHITE) ? "White" : "Black";
  int checked = !g->game_over && in_check(&g->state->position, side_to_move(g));
  snprintf(mover_label, sizeof(mover_label), "%s%s", mover, checked ? " (check)" : "");

  draw_hline(r, 0, 0, r.w, 0x2500u, C_RULE, COLOR_DEFAULT, ATTR_NONE);

  if (g->game_over) {
    snprintf(line, sizeof(line), "%s", g->message);
  } else if (g->awaiting_handoff) {
    snprintf(line, sizeof(line), "%s to move — press SPACE", mover_label);
  } else if (g->sel_i >= 0) {
    char from[3];
    index_to_square(g->sel_i, g->sel_j, from);
    snprintf(line, sizeof(line), "%s: %s → %-2s_   %s", mover_label, from, g->typed, g->message);
  } else {
    snprintf(line, sizeof(line), "%s: %-2s_   %s", mover_label, g->typed, g->message);
  }
  draw_text(r, 0, 1, line, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  /* Each hint appears only while its key does anything, which is also what
   * documents when it stops. Loading and saving are mutually exclusive, so the
   * line never has to carry both. Every variant fits the narrowest terminal the
   * game runs in: the status bar clips rather than wraps, but a clipped hint is
   * still a hint nobody can read. */
  const char *hints;
  if (g->game_over) {
    hints = "s save  ·  q quit";
  } else if (can_load(g)) {
    hints = "square+Enter · Esc clear · F flip · l load · q quit";
  } else {
    hints = "square+Enter · Esc clear · F flip · s save · q quit";
  }
  draw_text(r, 0, 2, hints, C_HINT, COLOR_DEFAULT, ATTR_DIM);
}

static void game_render(void *ctx, Rect r) {
  Game_t *g = (Game_t *)ctx;
  Layout lay;

  /* Laid out from the region handed in, every frame. Nothing here remembers a
   * size, so there is no stale layout for a resize to leave behind. */
  if (!layout_compute(r, glyphs_width(), &lay)) {
    return;
  }

  /* Cached for hit-testing: a click is resolved in game_handle against
   * whatever this frame actually drew, never against a layout recomputed on
   * the spot, so the two can never disagree about where the board is. */
  g->layout = lay;
  g->layout_valid = 1;

  char title[80];
  snprintf(title, sizeof(title), "Console Chess %s", chess_version());
  draw_text(lay.title, 1, 0, title, C_LABEL, COLOR_DEFAULT, ATTR_BOLD);

  draw_board(g, lay.board, &lay);
  draw_panel(g, lay.panel);
  draw_status(g, lay.status);
}

/* --- Turn flow ---------------------------------------------------------- */

static const char *outcome_message(Outcome_t oc) {
  switch (oc.reason) {
  case OUTCOME_CHECKMATE:
    return (oc.winner == WHITE) ? "Checkmate — White wins!" : "Checkmate — Black wins!";
  case OUTCOME_STALEMATE:
    return "Draw — stalemate.";
  case OUTCOME_DRAW_FIFTY_MOVE:
    return "Draw — fifty moves without a capture or pawn move.";
  case OUTCOME_DRAW_INSUFFICIENT_MATERIAL:
    return "Draw — insufficient material.";
  case OUTCOME_DRAW_REPETITION:
    return "Draw — threefold repetition.";
  case OUTCOME_IN_PROGRESS:
    break;
  }
  return "";
}

/* Applies a legal move chosen by the player — directly from submit(), or from
 * the promotion overlay once it knows which piece — and settles whatever
 * follows: captures, history, check-repetition bookkeeping, and the outcome
 * that decides whether the game just ended. */
static void finish_move(Game_t *g, Move move) {
  Color mover = side_to_move(g);
  Captures_node_t **captures = (mover == WHITE) ? &g->state->p_captures_white_head
                                                 : &g->state->p_captures_black_head;

  if (move.captured != FREE) {
    Color captured_color = (mover == WHITE) ? BLACK : WHITE;
    update_captures(captures, (Piece_t){.color = captured_color, .type = move.captured});
  }

  char from[3];
  char to[3];
  index_to_square(move.from_i, move.from_j, from);
  index_to_square(move.to_i, move.to_j, to);

  make(&g->state->position, move);
  update_history(&g->state->p_history_head, from, to);

  g->last_from_i = move.from_i;
  g->last_from_j = move.from_j;
  g->last_to_i = move.to_i;
  g->last_to_j = move.to_j;

  clear_entry(g);
  g->message[0] = '\0';

  /* hash_history excludes the position just reached, per outcome()'s
   * contract, so the lookup happens before this move's hash is pushed. */
  int hist_len = hash_history_length(g->state->p_hash_history_head);
  uint64_t hashes[hist_len > 0 ? hist_len : 1];
  hash_history_to_array(g->state->p_hash_history_head, hashes);
  Outcome_t oc = outcome(&g->state->position, hashes, hist_len);

  push_hash(&g->state->p_hash_history_head, g->state->position.hash);

  if (oc.reason != OUTCOME_IN_PROGRESS) {
    g->game_over = 1;
    snprintf(g->message, sizeof(g->message), "%s", outcome_message(oc));
  } else {
    g->awaiting_handoff = 1;
  }
}

static void on_promotion_choice(void *ctx, Piece_type_t choice) {
  Promotion_request_t *req = (Promotion_request_t *)ctx;
  for (int k = 0; k < req->count; k++) {
    if (req->choices[k].promotion == choice) {
      finish_move(req->game, req->choices[k]);
      return;
    }
  }
}

/* The SelectSquare event: naming a square, by whichever of the three
 * producers — a click, typed coordinates, or the keyboard cursor plus Enter —
 * arrives here identically. This state machine cannot tell which produced it
 * and must not be able to: that is what keeps the keyboard path from decaying
 * into a second-class path that quietly breaks, a real risk given that the
 * game must stay playable over SSH. */
static void select_square(Game_t *g, int i, int j, Cmd_t *cmd) {
  if (g->sel_i < 0) {
    Piece_t piece = g->state->position.board[i][j];
    if (piece.type == FREE || piece.color != side_to_move(g)) {
      snprintf(g->message, sizeof(g->message), "Pick one of your own pieces.");
      return;
    }
    g->sel_i = i;
    g->sel_j = j;
    g->message[0] = '\0';
    return;
  }

  if (i == g->sel_i && j == g->sel_j) {
    /* Naming the selected square again cancels it, same as Escape. */
    g->sel_i = -1;
    g->sel_j = -1;
    g->message[0] = '\0';
    return;
  }

  Piece_t target = g->state->position.board[i][j];
  if (target.type != FREE && target.color == side_to_move(g)) {
    /* Another piece of the side to move: the selection moves, nothing else. */
    g->sel_i = i;
    g->sel_j = j;
    g->message[0] = '\0';
    return;
  }

  MoveList legal;
  generate_legal_moves_from(&g->state->position, g->sel_i, g->sel_j, &legal);

  Move matches[4];
  int match_count = 0;
  for (int k = 0; k < legal.count && match_count < 4; k++) {
    if (legal.moves[k].to_i == i && legal.moves[k].to_j == j) {
      matches[match_count++] = legal.moves[k];
    }
  }

  if (match_count == 0) {
    snprintf(g->message, sizeof(g->message), "That piece cannot move there.");
    return; /* the selection stays; an illegal square never clears it */
  }

  if (match_count == 1) {
    finish_move(g, matches[0]);
    return;
  }

  /* More than one match happens only for promotion, one candidate per piece
   * choice: ask which, and finish the move once the overlay answers. */
  g_promo_request.game = g;
  g_promo_request.count = match_count;
  for (int k = 0; k < match_count; k++) {
    g_promo_request.choices[k] = matches[k];
  }
  *cmd = (Cmd_t){CMD_PUSH, promotion_screen(side_to_move(g), on_promotion_choice,
                                             &g_promo_request)};
}

/* The typed-coordinate producer: parses the status bar's buffer into a square
 * and hands it to the same event every other producer uses. */
static void submit_typed(Game_t *g, Cmd_t *cmd) {
  int i, j;
  if (g->typed_len != 2 || !square_to_index(g->typed, &i, &j)) {
    snprintf(g->message, sizeof(g->message), "Enter a square, e.g. e2.");
    return;
  }
  g->typed[0] = '\0';
  g->typed_len = 0;
  select_square(g, i, j, cmd);
}

/* The keyboard-cursor producer: Enter with nothing typed names the square the
 * cursor sits over. */
static void submit_cursor(Game_t *g, Cmd_t *cmd) {
  int i = g->flipped ? 7 - g->cursor_row : g->cursor_row;
  int j = g->flipped ? 7 - g->cursor_col : g->cursor_col;
  select_square(g, i, j, cmd);
}

static void type_char(Game_t *g, uint32_t ch) {
  /* A square is a file then a rank, so each position accepts only what can
   * legally be there. Nothing else reaches the buffer. */
  if (g->typed_len == 0 && ch >= 'a' && ch <= 'h') {
    g->typed[0] = (char)ch;
    g->typed[1] = '\0';
    g->typed_len = 1;
  } else if (g->typed_len == 1 && ch >= '1' && ch <= '8') {
    g->typed[1] = (char)ch;
    g->typed[2] = '\0';
    g->typed_len = 2;
  }
}

/* Moves the keyboard cursor one square, in screen space, clamped to the
 * board — so an arrow key always moves the cursor where it visibly points,
 * regardless of orientation, the same way a click already does. */
static void move_cursor(Game_t *g, int drow, int dcol) {
  g->cursor_row += drow;
  g->cursor_col += dcol;
  if (g->cursor_row < 0) {
    g->cursor_row = 0;
  } else if (g->cursor_row > 7) {
    g->cursor_row = 7;
  }
  if (g->cursor_col < 0) {
    g->cursor_col = 0;
  } else if (g->cursor_col > 7) {
    g->cursor_col = 7;
  }
}

static Cmd_t game_handle(void *ctx, const Event_t *ev) {
  Game_t *g = (Game_t *)ctx;
  Cmd_t quit = {CMD_QUIT, NULL};

  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_WHEEL) {
    /* Deliberately discarded. Alternate scroll is already off (see term.c),
     * so this is the only place wheel motion can still reach — and the board
     * has nothing to scroll. It must not alter the selection, make a move, or
     * be mistaken for keyboard input; the history screen elsewhere is the
     * consumer these events are actually for. */
    return CMD_STAY;
  }
  if (ev->type == EV_MOUSE && (ev->mouse.kind != MOUSE_PRESS || ev->mouse.button != 0)) {
    /* Click-click, not drag-and-drop: only a left press can ever produce a
     * SelectSquare event. Release and motion reports are read by the parser
     * but have nothing to do here. */
    return CMD_STAY;
  }
  if (ev->type != EV_KEY && ev->type != EV_MOUSE) {
    return CMD_STAY; /* paste and anything else: not a producer of SelectSquare */
  }

  if (ev->type == EV_KEY && ev->key.name == KEY_CHAR &&
      (ev->key.ch == 'q' || ev->key.ch == 'Q')) {
    return quit;
  }
  /* Ctrl-L forces a full repaint. It exists so a display that looks wrong can
   * be told apart from state that is wrong: if this fixes it, the frame was
   * composed correctly and the diff was at fault. */
  if (ev->type == EV_KEY &&
      ((ev->key.name == KEY_CHAR && ev->key.ch == 12) || ev->key.name == KEY_F5)) {
    render_force_repaint();
    return CMD_STAY;
  }
  /* Shift-F, not f: the files are a-h, so a lowercase f belongs to the move
   * field. A binding that swallowed it would make the f-file unreachable. */
  if (ev->type == EV_KEY && ev->key.name == KEY_CHAR && ev->key.ch == 'F') {
    g->flipped = !g->flipped;
    return CMD_STAY;
  }
  /* Temporary; see the saving and loading section above. */
  if (ev->type == EV_KEY && ev->key.name == KEY_CHAR &&
      (ev->key.ch == 's' || ev->key.ch == 'S')) {
    if (!can_save(g)) {
      snprintf(g->message, sizeof(g->message),
               "Nothing to save yet — play a move first.");
    } else {
      snprintf(g->message, sizeof(g->message), "%s",
               save_game(g->state) ? "Game saved." : "Could not save the game.");
    }
    return CMD_STAY;
  }
  if (ev->type == EV_KEY && ev->key.name == KEY_CHAR &&
      (ev->key.ch == 'l' || ev->key.ch == 'L')) {
    if (can_load(g)) {
      load_into(g);
    } else {
      snprintf(g->message, sizeof(g->message),
               "Loading is offered only before the first move.");
    }
    return CMD_STAY;
  }

  if (g->game_over) {
    return CMD_STAY;
  }

  if (g->awaiting_handoff) {
    if (ev->type == EV_KEY &&
        (ev->key.name == KEY_ENTER || (ev->key.name == KEY_CHAR && ev->key.ch == ' '))) {
      g->awaiting_handoff = 0;
      g->flipped = (side_to_move(g) == BLACK);
    }
    return CMD_STAY;
  }

  if (ev->type == EV_MOUSE) {
    if (!g->layout_valid) {
      return CMD_STAY; /* nothing has been drawn yet to hit-test against */
    }
    Square_hit_t hit = point_to_square(&g->layout, ev->mouse.col, ev->mouse.row, g->flipped);
    if (!hit.valid) {
      return CMD_STAY; /* outside the board: no square, no selection change */
    }
    Cmd_t cmd = CMD_STAY;
    select_square(g, hit.i, hit.j, &cmd);
    return cmd;
  }

  switch (ev->key.name) {
  case KEY_ENTER: {
    Cmd_t cmd = CMD_STAY;
    if (g->typed_len > 0) {
      submit_typed(g, &cmd);
    } else {
      submit_cursor(g, &cmd);
    }
    return cmd;
  }
  case KEY_ESCAPE:
    clear_entry(g);
    g->message[0] = '\0';
    break;
  case KEY_BACKSPACE:
    if (g->typed_len > 0) {
      g->typed_len--;
      g->typed[g->typed_len] = '\0';
    } else if (g->sel_i >= 0) {
      g->sel_i = -1;
      g->sel_j = -1;
    }
    break;
  case KEY_CHAR:
    type_char(g, ev->key.ch);
    break;
  case KEY_UP:
    move_cursor(g, -1, 0);
    break;
  case KEY_DOWN:
    move_cursor(g, 1, 0);
    break;
  case KEY_LEFT:
    move_cursor(g, 0, -1);
    break;
  case KEY_RIGHT:
    move_cursor(g, 0, 1);
    break;
  default:
    break;
  }
  return CMD_STAY;
}

/* --- Construction ------------------------------------------------------- */

static void game_on_enter(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  clear_entry(g);
  g->message[0] = '\0';
  g->flipped = (side_to_move(g) == BLACK);
  g->use_color = term_supports_color();
}

Screen *game_screen(GameState *state) {
  static Screen screen;

  memset(&g_game, 0, sizeof(g_game));
  g_game.state = state;
  g_game.sel_i = -1;
  g_game.sel_j = -1;
  g_game.last_from_i = -1;
  g_game.last_from_j = -1;
  g_game.last_to_i = -1;
  g_game.last_to_j = -1;
  g_game.cursor_row = 6; /* screen-space e2, a reasonable place to start */
  g_game.cursor_col = 4;

  screen.on_enter = game_on_enter;
  screen.on_exit = NULL;
  screen.handle = game_handle;
  screen.render = game_render;
  screen.ctx = &g_game;
  screen.opaque = 1;
  return &screen;
}
