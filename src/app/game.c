#include "app/game.h"

#include "app/confirm.h"
#include "app/gameover.h"
#include "app/help.h"
#include "app/history_view.h"
#include "app/promotion.h"
#include "app/save.h"
#include "app/settings.h"
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
#include <stdlib.h>
#include <string.h>

/* --- Palette ---------------------------------------------------------------
 *
 * Piece and rule colours are fixed; the four square tints and the check mark
 * come from settings_palette(), so the Settings overlay's colour-scheme
 * option can switch them at runtime. See settings.h for the presets.
 */
#define C_PIECE_WHITE 231
/* Black on an unbackgrounded square would be invisible on a dark terminal, so
 * it sits at a grey that reads against either. */
#define C_PIECE_BLACK 245
#define C_RULE 244
#define C_LABEL 246
#define C_HINT 244
#define C_HINT_DIM 240

/* Legal, empty destination: centred, shape-based already, so it is identical
 * in colour and monochrome mode. */
#define MARK_DOT 0x2022u /* • smaller than the U+25CF full circle */
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

  /* The board cursor: a third way to name a square, moved by the arrow keys
   * and read on Enter when nothing has been typed. Held in screen space (0-7,
   * top-left of what is currently drawn) rather than board indices, so an
   * arrow key moves the cursor where it visibly points regardless of
   * orientation — the same reasoning point_to_square applies to a click.
   *
   * It is only there when it is being used. Every turn starts with
   * cursor_active clear and nothing pointed at; the first arrow press brings
   * it into existence at the centre of the board, and naming a square by any
   * means (click, typed coordinates, or the cursor itself) puts it on that
   * square. Completing a move or cancelling the selection takes it away
   * again. The cursor is what the highlighted rank and file labels report, so
   * a highlighted label always means "this is the square in hand" rather than
   * being a permanent fixture the player has to learn to ignore. */
  int cursor_row, cursor_col;
  int cursor_active;

  /* The layout the most recent frame was drawn with, so a click is hit-tested
   * against the geometry it actually saw rather than one recomputed — possibly
   * differently — after the fact. */
  Layout layout;
  int layout_valid;

  /* Whether this terminal is worth spending colour on, decided once at entry
   * from the environment; see term_supports_color(). */
  int use_color;

  /* Between turns: the board is about to flip and the next player has not yet
   * said they are looking. In pass-and-play that gesture is the handoff, and
   * a human paces it better than a constant does — this is the only way the
   * board changes hands, deliberately: it doubles as the "I'm ready" signal a
   * future timed mode needs, so it is never skipped or made optional. */
  int awaiting_handoff;

  int has_draw_offer;
  Color draw_offer_by;

  /* Set once a game-ending outcome is reached. The transition to the result
   * screen happens on this screen's next handled event (see the top of
   * game_handle): a screen can only return one Cmd_t, and several endings
   * (resignation, a draw acceptance, a promotion that also mates) are
   * discovered while a different overlay is on top of this one, where
   * replacing this screen out from under it is not expressible in one step. */
  int game_over;
  Outcome_t pending_outcome;

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

/* Saving the opening position with no moves played would create a file with
 * nothing worth loading. */
static int can_save(const Game_t *g) { return g->state->p_history_head != NULL; }

static void clear_entry(Game_t *g) {
  g->typed[0] = '\0';
  g->typed_len = 0;
  g->sel_i = -1;
  g->sel_j = -1;
  g->cursor_active = 0;
}

/* Puts the cursor on a board square, in whichever screen position that square
 * currently occupies. */
static void cursor_to_square(Game_t *g, int i, int j) {
  g->cursor_row = g->flipped ? 7 - i : i;
  g->cursor_col = g->flipped ? 7 - j : j;
  g->cursor_active = 1;
}

/* The loaded/resumed/undone/redone game's last move, so the highlight always
 * matches whatever is actually on top of the history list. */
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

/* Which of the four mutually exclusive background states a square is in.
 * Priority breaks the cases where two could apply at once: reselecting the
 * piece that was itself the destination of the last move, or the cursor
 * sitting on a square that is already marked for some other reason. Selection
 * outranks everything, and the cursor outranks the rest — it is the one thing
 * the player is actively moving, so it must never be the marking that loses. */
typedef enum {
  SQ_NONE,
  SQ_SELECTED,
  SQ_CURSOR,
  SQ_CAPTURE_DEST,
  SQ_LAST_MOVE
} Square_priority_t;

static Square_priority_t square_priority(const Game_t *g, int i, int j, int is_capture_dest) {
  if (i == g->sel_i && j == g->sel_j) {
    return SQ_SELECTED;
  }
  if (g->cursor_active) {
    int cursor_i = g->flipped ? 7 - g->cursor_row : g->cursor_row;
    int cursor_j = g->flipped ? 7 - g->cursor_col : g->cursor_col;
    if (i == cursor_i && j == cursor_j) {
      return SQ_CURSOR;
    }
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
  const Palette_t *pal = settings_palette();
  switch (p) {
  case SQ_SELECTED:
  /* The cursor is drawn as a selection because that is what it is about to
   * become: pressing Enter on it either picks that piece up or plays the move
   * to it. One highlight for "the square in hand", however it was named. */
  case SQ_CURSOR:
    return pal->square_selected;
  case SQ_CAPTURE_DEST:
    return pal->square_capture;
  case SQ_LAST_MOVE:
    return pal->square_last;
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
  case SQ_CURSOR:
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
  const Palette_t *pal = settings_palette();

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
     * never the board's interior. Nothing is highlighted while there is no
     * cursor, so a highlighted label always names the square in hand. */
    uint8_t attr = (g->cursor_active && f == g->cursor_col) ? ATTR_REVERSE : ATTR_NONE;
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
    uint8_t rank_attr = (g->cursor_active && row == g->cursor_row) ? ATTR_REVERSE : ATTR_NONE;
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
          bg = pal->square_check;
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
        int mark_fg = g->use_color ? pal->mark_check_fg : COLOR_DEFAULT;
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
   * the tail that fits is what is shown. The full scrollable list is the
   * History screen (Shift-H). */
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

/* One command key and whether it currently does anything — the status bar's
 * unit of display. Shown dimmed rather than omitted when unavailable, so a
 * key that does nothing right now is still visibly a key (task: available
 * commands are visible). */
typedef struct {
  const char *key;
  const char *label;
  int available;
} Hint_t;

static void draw_hints(Rect r, int y, const Hint_t *hints, int n) {
  int x = 0;
  for (int i = 0; i < n && x < r.w; i++) {
    char buf[24];
    snprintf(buf, sizeof(buf), "%s%s %s", i == 0 ? "" : "  ", hints[i].key, hints[i].label);
    uint8_t attr = hints[i].available ? ATTR_NONE : ATTR_DIM;
    int fg = hints[i].available ? C_HINT : C_HINT_DIM;
    x += draw_text(r, x, y, buf, fg, COLOR_DEFAULT, attr);
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
    snprintf(line, sizeof(line), "%s  ·  press any key to continue", g->message);
  } else if (g->awaiting_handoff) {
    snprintf(line, sizeof(line), "%s to move — press SPACE   %s", mover_label, g->message);
  } else if (g->has_draw_offer) {
    const char *offerer = (g->draw_offer_by == WHITE) ? "White" : "Black";
    snprintf(line, sizeof(line), "%s: %s offered a draw   %s", mover_label, offerer, g->message);
  } else if (g->sel_i >= 0) {
    char from[3];
    index_to_square(g->sel_i, g->sel_j, from);
    snprintf(line, sizeof(line), "%s: %s → %-2s_   %s", mover_label, from, g->typed, g->message);
  } else {
    snprintf(line, sizeof(line), "%s: %-2s_   %s", mover_label, g->typed, g->message);
  }
  draw_text(r, 0, 1, line, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  if (g->game_over) {
    return; /* the result screen replaces this one on the next event */
  }

  Hint_t hints[] = {
      {"s", "save", can_save(g)},
      {"H", "history", 1},
      {"x", "resign", 1},
      {"o", "draw", 1},
      {"?", "help", 1},
      {"q", "quit", 1},
  };
  draw_hints(r, 2, hints, (int)(sizeof(hints) / sizeof(hints[0])));
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
  case OUTCOME_RESIGNATION:
    return (oc.winner == WHITE) ? "Black resigns — White wins!" : "White resigns — Black wins!";
  case OUTCOME_DRAW_AGREEMENT:
    return "Draw — by agreement.";
  case OUTCOME_IN_PROGRESS:
    break;
  }
  return "";
}

/* Every move begins the handover gesture: the board is about to flip and the
 * next player confirms with Space before it does. */
static void begin_turn(Game_t *g) { g->awaiting_handoff = 1; }

static void apply_outcome(Game_t *g, Outcome_t oc) {
  g->game_over = 1;
  g->pending_outcome = oc;
  g->awaiting_handoff = 0;
  g->has_draw_offer = 0;
  snprintf(g->message, sizeof(g->message), "%s", outcome_message(oc));
}

/* Applies a legal move chosen by the player — directly from submit(), or from
 * the promotion overlay once it knows which piece — and settles whatever
 * follows: captures, history, check-repetition bookkeeping, and the outcome
 * that decides whether the game just ended. */
static void finish_move(Game_t *g, Move move) {
  GameState *state = g->state;
  Color mover = side_to_move(g);

  /* Playing a move rather than responding to a pending offer is a decline:
   * the game continues, and the offer is gone either way. (Side to move
   * cannot itself distinguish "the offerer moved on" from "the other side
   * declined by moving" in pass-and-play, since nothing changes side_to_move
   * except a move — either reading clears a stale offer correctly.) */
  g->has_draw_offer = 0;

  Captures_node_t **captures = (mover == WHITE) ? &state->p_captures_white_head
                                                 : &state->p_captures_black_head;
  if (move.captured != FREE) {
    Color captured_color = (mover == WHITE) ? BLACK : WHITE;
    update_captures(captures, (Piece_t){.color = captured_color, .type = move.captured});
  }

  char from[3];
  char to[3];
  index_to_square(move.from_i, move.from_j, from);
  index_to_square(move.to_i, move.to_j, to);

  make(&state->position, move);
  update_history(&state->p_history_head, from, to, move);

  g->last_from_i = move.from_i;
  g->last_from_j = move.from_j;
  g->last_to_i = move.to_i;
  g->last_to_j = move.to_j;

  clear_entry(g);
  g->message[0] = '\0';

  /* hash_history excludes the position just reached, per outcome()'s
   * contract, so the lookup happens before this move's hash is pushed. */
  int hist_len = hash_history_length(state->p_hash_history_head);
  uint64_t hashes[hist_len > 0 ? hist_len : 1];
  hash_history_to_array(state->p_hash_history_head, hashes);
  Outcome_t oc = outcome(&state->position, hashes, hist_len);

  push_hash(&state->p_hash_history_head, state->position.hash);

  if (oc.reason != OUTCOME_IN_PROGRESS) {
    apply_outcome(g, oc);
  } else {
    begin_turn(g);
  }
}

/* Undo and redo are deliberately not offered here: chess does not allow
 * taking back a move you have already made, only reviewing a finished game
 * move by move. The move list is still kept as full Move structs (not just
 * square pairs) and core/history.c still carries history_pop_last,
 * history_push_node, captures_pop_last, and hash_history_pop_last precisely
 * so a future "replay a finished game" mode can unmake/make through it —
 * against a copy of a finished game's state, never the game in progress. */

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
static void select_square_core(Game_t *g, int i, int j, Cmd_t *cmd) {
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
    /* The common case: no overlay was involved, so this event's Cmd_t can
     * take the player straight to the result screen instead of waiting for
     * one more keypress (see game_over's handling at the top of handle()). */
    if (g->game_over) {
      *cmd = (Cmd_t){CMD_REPLACE, gameover_screen(g->state, g->pending_outcome, g->flipped)};
    }
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

/* Naming a square also moves the cursor onto it, whichever producer named it,
 * so the highlighted rank and file labels report the same square the board is
 * highlighting. A square named while nothing ends up selected — a completed
 * move, a cancelled selection, a click on an empty square with nothing in
 * hand — leaves nothing to point at, and the cursor goes away with it. */
static void select_square(Game_t *g, int i, int j, Cmd_t *cmd) {
  select_square_core(g, i, j, cmd);
  if (g->sel_i >= 0) {
    cursor_to_square(g, i, j);
  } else {
    g->cursor_active = 0;
  }
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

/* Where the cursor appears when an arrow key summons it: screen-space centre,
 * on the near side of the four middle squares. No square is more than four
 * steps away from it, and the player's own back ranks — where most of the
 * pieces they are reaching for sit — are the closer half. */
#define CURSOR_HOME_ROW 4
#define CURSOR_HOME_COL 4

/* Moves the board cursor one square, in screen space, clamped to the board —
 * so an arrow key always moves the cursor where it visibly points, regardless
 * of orientation, the same way a click already does.
 *
 * The first arrow press of a turn only summons the cursor, at the centre; it
 * does not also step. Stepping from wherever the cursor happened to be left
 * last turn would mean the same key does something different depending on
 * history the player can no longer see. */
static void move_cursor(Game_t *g, int drow, int dcol) {
  if (!g->cursor_active) {
    g->cursor_row = CURSOR_HOME_ROW;
    g->cursor_col = CURSOR_HOME_COL;
    g->cursor_active = 1;
    return;
  }
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

/* --- Commands ------------------------------------------------------------- */

/* Once a game has been saved once, it is assigned a path (GameState.save_path)
 * that every later save of the same game reuses, so saving repeatedly
 * updates one file instead of collecting a new one each time; loading a game
 * carries its path over the same way (see savedgames.c), so continuing a
 * loaded game and saving it again still updates that same file. */
static void save_game_now(Game_t *g) {
  if (!can_save(g)) {
    snprintf(g->message, sizeof(g->message), "Nothing to save yet — play a move first.");
    return;
  }
  if (g->state->save_path[0] == '\0') {
    if (!save_new_game_path(g->state->save_path, sizeof(g->state->save_path))) {
      snprintf(g->message, sizeof(g->message), "Could not save.");
      return;
    }
  }
  int ok = save_write(g->state->save_path, g->state);
  snprintf(g->message, sizeof(g->message), "%s", ok ? "Saved." : "Could not save.");
}

/* The quit picker: replaces a plain yes/no confirmation now that there is no
 * autosave to fall back on — leaving without saving is a real, permanent
 * loss, so quitting is the moment saving is offered directly rather than
 * assumed. Only two options when there is nothing to save yet. */
#define QUIT_ITEM_COUNT 3

typedef struct {
  Game_t *game;
  int selected;
  int option_count;
  int row_y[QUIT_ITEM_COUNT];
} Quit_ctx_t;

static Quit_ctx_t g_quit_ctx;

static void quit_on_enter(void *ctx) {
  Quit_ctx_t *qc = (Quit_ctx_t *)ctx;
  qc->option_count = can_save(qc->game) ? 3 : 2;
  /* Cancel — the last option in both label sets. Two of the three choices
   * here end the program, one of them discarding the game, so the overlay
   * opens on the one that does nothing; leaving is a deliberate arrow key
   * away rather than the thing a stray Enter does. */
  qc->selected = qc->option_count - 1;
}

static const char *const QUIT_LABELS_WITH_SAVE[QUIT_ITEM_COUNT] = {
    "Save and quit",
    "Quit without saving",
    "Cancel",
};
static const char *const QUIT_LABELS_NO_SAVE[QUIT_ITEM_COUNT] = {
    "Quit",
    "Cancel",
    NULL,
};

static void quit_render(void *ctx, Rect r) {
  Quit_ctx_t *qc = (Quit_ctx_t *)ctx;
  const char *const *labels = (qc->option_count == 3) ? QUIT_LABELS_WITH_SAVE : QUIT_LABELS_NO_SAVE;

  int w = 30, h = 4 + qc->option_count;
  int x = (r.w - w) / 2, y = (r.h - h) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  Rect box = rect_sub(r, x, y, w, h);
  draw_fill(box, ' ', COLOR_DEFAULT, 236, ATTR_NONE);
  draw_box(box, 250, 236, ATTR_NONE);
  Rect inner = rect_inset(box, 2, 1);
  draw_text(inner, 0, 0, "Quit the game?", 250, 236, ATTR_BOLD);

  for (int k = 0; k < qc->option_count; k++) {
    uint8_t attr = (k == qc->selected) ? ATTR_REVERSE : ATTR_NONE;
    draw_text(inner, 0, 2 + k, labels[k], 250, 236, attr);
    qc->row_y[k] = inner.y + 2 + k;
  }

  app_draw_bottom_hint(r, "↑/↓ + Enter, or click to select  ·  Esc cancel");
}

/* Option indices are the same in both label sets: 0 acts, 1 or (no-save)
 * cancels or quits, whichever the option count implies. */
static Cmd_t quit_activate(Quit_ctx_t *qc, int k) {
  Game_t *g = qc->game;
  if (qc->option_count == 3) {
    switch (k) {
    case 0:
      save_game_now(g);
      return (Cmd_t){CMD_QUIT, NULL};
    case 1:
      return (Cmd_t){CMD_QUIT, NULL};
    default:
      return (Cmd_t){CMD_POP, NULL};
    }
  }
  switch (k) {
  case 0:
    return (Cmd_t){CMD_QUIT, NULL};
  default:
    return (Cmd_t){CMD_POP, NULL};
  }
}

static Cmd_t quit_handle(void *ctx, const Event_t *ev) {
  Quit_ctx_t *qc = (Quit_ctx_t *)ctx;

  /* A click only moves the highlight; Enter is the one way to act on it. The
   * stakes here are the whole game, so a stray click must not be able to end
   * it. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    for (int k = 0; k < qc->option_count; k++) {
      if (ev->mouse.row == qc->row_y[k]) {
        qc->selected = k;
        break;
      }
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return (Cmd_t){CMD_POP, NULL};
  }
  if (ev->key.name == KEY_UP) {
    qc->selected = (qc->selected - 1 + qc->option_count) % qc->option_count;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_DOWN) {
    qc->selected = (qc->selected + 1) % qc->option_count;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ENTER) {
    return quit_activate(qc, qc->selected);
  }
  return CMD_STAY;
}

static Screen *quit_screen(Game_t *g) {
  static Screen screen;
  g_quit_ctx.game = g;
  screen.on_enter = quit_on_enter;
  screen.on_exit = NULL;
  screen.handle = quit_handle;
  screen.render = quit_render;
  screen.ctx = &g_quit_ctx;
  screen.opaque = 0;
  return &screen;
}

/* Resigning is the side to move's own choice, so the only question worth
 * asking is "are you sure" — the confirmation names what is being given up
 * rather than asking which player is speaking.
 *
 * It used to ask which side resigns instead, on the grounds that
 * pass-and-play shares one keyboard and side_to_move only changes on an
 * actual move, so the program cannot tell the two players apart. That is
 * true, and it does mean resigning strictly out of turn is no longer
 * expressible; but the handover gesture already establishes whose turn it is
 * before either player touches a key, and making everyone answer "who" every
 * time to preserve a case that essentially never comes up was the worse
 * trade. */
static Cmd_t on_resign_confirm(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  Outcome_t oc = {.reason = OUTCOME_RESIGNATION,
                  .winner = (side_to_move(g) == WHITE) ? BLACK : WHITE};
  apply_outcome(g, oc);
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t on_resign_cancel(void *ctx) {
  (void)ctx;
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t resign(Game_t *g) {
  const char *winner = (side_to_move(g) == WHITE) ? "Black" : "White";
  char msg[96];
  snprintf(msg, sizeof(msg), "Resign? %s will win.", winner);
  return (Cmd_t){CMD_PUSH, confirm_screen(msg, on_resign_confirm, on_resign_cancel, g)};
}

/* A draw offer is one keypress, not two. It used to take two — the first
 * recorded the offer and the second opened the response — because in
 * pass-and-play both players share a keyboard and nothing but an actual move
 * changes side_to_move, so 'o' pressed twice could not be told apart from two
 * different people pressing it. But nothing on screen said a second press was
 * what came next, which made the first press look like it had simply failed.
 * The two players are in the same room: the offer and the answer are one
 * exchange, and the prompt names both sides explicitly so whoever is holding
 * the keyboard knows which of them it is addressed to. */
static Cmd_t on_draw_accept(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  Outcome_t oc = {.reason = OUTCOME_DRAW_AGREEMENT, .winner = NONE};
  apply_outcome(g, oc);
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t on_draw_decline(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  g->has_draw_offer = 0;
  g->message[0] = '\0';
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t offer_draw(Game_t *g) {
  g->has_draw_offer = 1;
  g->draw_offer_by = side_to_move(g);
  g->message[0] = '\0';

  const char *offerer = (g->draw_offer_by == WHITE) ? "White" : "Black";
  const char *other = (g->draw_offer_by == WHITE) ? "Black" : "White";
  char msg[80];
  snprintf(msg, sizeof(msg), "%s offered a draw — does %s accept?", offerer, other);
  return (Cmd_t){CMD_PUSH, confirm_screen(msg, on_draw_accept, on_draw_decline, g)};
}

/* --- Input ---------------------------------------------------------------- */

static Cmd_t game_handle(void *ctx, const Event_t *ev) {
  Game_t *g = (Game_t *)ctx;

  /* A game-ending action discovered while some other screen (promotion,
   * resignation) was on top could not replace this screen directly; it is
   * caught up on here, the moment this screen is handling an event again.
   * g->flipped carries over unchanged, so the result screen shows the board
   * in the same orientation the player was already looking at rather than
   * recomputing one — nothing here should look like the board just flipped. */
  if (g->game_over) {
    return (Cmd_t){CMD_REPLACE, gameover_screen(g->state, g->pending_outcome, g->flipped)};
  }

  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_WHEEL) {
    /* Deliberately discarded. Alternate scroll is already off (see term.c),
     * so this is the only place wheel motion can still reach — and the board
     * has nothing to scroll. It must not alter the selection, make a move, or
     * be mistaken for keyboard input; the history screen is the consumer
     * these events are actually for. */
    return CMD_STAY;
  }

  /* Commands that do not alter the position: available on either side's
   * turn, and even while awaiting the handoff — checked before that gate. */
  if (ev->type == EV_KEY && ev->key.name == KEY_CHAR) {
    switch (ev->key.ch) {
    case 'q':
    case 'Q':
      return (Cmd_t){CMD_PUSH, quit_screen(g)};
    case 12: /* Ctrl-L: forces a full repaint, to tell a display bug from a
              * state bug apart — if this fixes it, the frame was composed
              * correctly and the diff was at fault. */
      render_force_repaint();
      return CMD_STAY;
    case 's':
      save_game_now(g);
      return CMD_STAY;
    case 'H':
      /* Shift-H, not h: h is a file name and belongs to the move field. */
      return (Cmd_t){CMD_PUSH, history_view_screen(g->state)};
    case '?':
      return (Cmd_t){CMD_PUSH, help_screen()};
    case 'x':
    case 'X':
      return resign(g);
    case 'o':
    case 'O':
      return offer_draw(g);
    default:
      break;
    }
  }
  if (ev->type == EV_KEY && ev->key.name == KEY_F5) {
    render_force_repaint();
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
    } else if (g->cursor_active) {
      submit_cursor(g, &cmd);
    }
    /* Enter with nothing typed and no cursor names no square at all — there
     * is nothing on screen it could be pointing at. */
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
      g->cursor_active = 0;
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
  highlight_last_move(g);
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
  /* No cursor until an arrow key asks for one; see move_cursor. */
  g_game.cursor_row = CURSOR_HOME_ROW;
  g_game.cursor_col = CURSOR_HOME_COL;
  g_game.cursor_active = 0;

  screen.on_enter = game_on_enter;
  screen.on_exit = NULL;
  screen.handle = game_handle;
  screen.render = game_render;
  screen.ctx = &g_game;
  screen.opaque = 1;
  return &screen;
}
