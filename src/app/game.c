#include "app/game.h"

#include "app/confirm.h"
#include "app/gameover.h"
#include "app/help.h"
#include "app/history_view.h"
#include "app/promotion.h"
#include "app/prompt.h"
#include "app/save.h"
#include "app/settings.h"
#include "core/board.h"
#include "core/chessclock.h"
#include "core/history.h"
#include "core/movegen.h"
#include "core/notation.h"
#include "core/outcome.h"
#include "core/position.h"
#include "core/replay.h"
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

/* The two clock rectangles. Fixed rather than drawn from settings_palette():
 * these are the *pieces'* colours — light with dark digits for White, dark
 * with light digits for Black — rather than the board's tint scheme, so
 * which clock belongs to which side reads the same under every colour
 * scheme Settings offers. */
#define C_CLOCK_WHITE_FG 232
#define C_CLOCK_WHITE_BG 254
#define C_CLOCK_BLACK_FG 252
#define C_CLOCK_BLACK_BG 236

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

/* GAME_MODE_LIVE plays; GAME_MODE_REPLAY steps. game_handle branches on this
 * before any live-play handling, so a replay never reaches the code that
 * could originate a move. */
typedef enum { GAME_MODE_LIVE, GAME_MODE_REPLAY } Game_mode_t;

typedef struct {
  GameState *state;
  Game_mode_t mode;

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
   * board changes hands, deliberately, and it is never skipped or made
   * optional. In a timed game it is also the clock press: the mover's own
   * time runs until it (see press_clock_and_flip). */
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

/* How often a running clock asks to be woken. Fast enough for the tenths
 * shown under ten seconds, and slow enough to be free: the reading is a
 * function of two timestamps rather than something a tick accumulates, so
 * the work per wake is one clock reading, one frame composition, and a diff
 * that writes nothing at all when the digits have not changed. */
#define CLOCK_WAKE_MS 100

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

static Chess_clock_t *game_clock(Game_t *g) { return &g->state->clock; }
static const Chess_clock_t *game_clock_const(const Game_t *g) { return &g->state->clock; }

/* Whether the game can be drawn at all — the same question app.c asks before
 * it shows the too-small screen in place of the whole stack. Asked here
 * because a game that cannot be displayed is not a game either player can
 * play, and neither may be charged for the time it is unplayable. */
static int game_fits(void) {
  Rect bounds = render_bounds();
  int width = glyphs_width();
  return bounds.w >= layout_min_cols(width) && bounds.h >= layout_min_rows(width);
}

/* Both sides' readings at now_ms, in the order History_node_t stores them. */
static void clock_snapshot(const Game_t *g, uint64_t now_ms, int32_t out[2]) {
  out[WHITE] = clock_remaining(game_clock_const(g), WHITE, now_ms);
  out[BLACK] = clock_remaining(game_clock_const(g), BLACK, now_ms);
}

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

/* --- The clocks ---------------------------------------------------------- */

static const char *side_name(Color side) { return (side == WHITE) ? "White" : "Black"; }

/* What each clock reads, indexed by Color.
 *
 * In live play this is the running reading, computed from the time control
 * and two timestamps. In a replay it is what the players actually had at the
 * position being shown, read off the history node the step moved to — and
 * the game's initial time at the starting position, since that is what both
 * sides had before anything was played. Nothing runs in a replay and nothing
 * is recomputed from the time control, so a replay left open for an hour
 * shows the same pair of readings it did at the start. */
static void clock_readings(const Game_t *g, int32_t out[2]) {
  if (g->mode == GAME_MODE_REPLAY) {
    /* The final position is where the clocks actually stopped, which is not
     * always the reading the last move left: a game lost on time ended
     * partway through the turn after it, with the loser at zero. The file
     * records that reading separately for exactly this reason, and for every
     * other ending the two agree. */
    if (g->state->p_redo_head == NULL) {
      out[WHITE] = game_clock_const(g)->remaining_ms[WHITE];
      out[BLACK] = game_clock_const(g)->remaining_ms[BLACK];
      return;
    }
    const History_node_t *last = NULL;
    for (const History_node_t *p = g->state->p_history_head; p != NULL; p = p->p_next) {
      last = p;
    }
    if (last == NULL) {
      out[WHITE] = game_clock_const(g)->initial_ms;
      out[BLACK] = game_clock_const(g)->initial_ms;
    } else {
      out[WHITE] = last->remaining_ms[WHITE];
      out[BLACK] = last->remaining_ms[BLACK];
    }
    return;
  }
  clock_snapshot(g, term_now_ms(), out);
}

static void draw_centred(Rect r, int y, const char *text, int fg, int bg, uint8_t attr) {
  int x = (r.w - (int)strlen(text)) / 2;
  if (x < 0) {
    x = 0;
  }
  draw_text(r, x, y, text, fg, bg, attr);
}

/* One clock: a box carrying its side's name and its formatted time.
 *
 * The name is there whether or not there is colour to distinguish them by,
 * because on a terminal drawing none the backgrounds are dropped and the
 * name is the only thing left saying which clock this is. */
static void draw_clock_box(const Game_t *g, Rect r, Color side, int32_t ms) {
  if (r.w < 4 || r.h < 3) {
    return;
  }
  int fg = (side == WHITE) ? C_CLOCK_WHITE_FG : C_CLOCK_BLACK_FG;
  int bg = (side == WHITE) ? C_CLOCK_WHITE_BG : C_CLOCK_BLACK_BG;
  if (!g->use_color) {
    fg = COLOR_DEFAULT;
    bg = COLOR_DEFAULT;
  }

  draw_fill(r, ' ', fg, bg, ATTR_NONE);
  draw_box(r, fg, bg, ATTR_NONE);

  Rect inner = rect_inset(r, 1, 1);
  if (inner.h < 1) {
    return;
  }
  char time_text[CLOCK_FORMAT_MAX];
  clock_format(ms, time_text, sizeof(time_text));

  int top = (inner.h - 2) / 2;
  if (top < 0) {
    top = 0;
  }
  draw_centred(inner, top, side_name(side), fg, bg, ATTR_NONE);
  draw_centred(inner, top + 1, time_text, fg, bg, ATTR_BOLD);
}

/* The pair, spanning the board's height between them with a one-row gap, so
 * they grow and shrink with the board and line up with its top and bottom
 * edges. The lower one always belongs to the side the board currently faces
 * — the near player's clock is nearest them, exactly as their pieces are —
 * which follows g->flipped and so turns with the board at a handover and at
 * a replay's manual flip without any state of its own. */
static void draw_clocks(const Game_t *g, Rect r, int board_h) {
  if (r.w <= 0 || board_h < 3) {
    return;
  }
  int box_h = (board_h - 1) / 2;
  if (box_h < 3) {
    return;
  }

  int32_t reading[2];
  clock_readings(g, reading);

  Color near = g->flipped ? BLACK : WHITE;
  Color far = (near == WHITE) ? BLACK : WHITE;

  draw_clock_box(g, rect_sub(r, 0, 0, r.w, box_h), far, reading[far]);
  draw_clock_box(g, rect_sub(r, 0, board_h - box_h, r.w, box_h), near, reading[near]);
}

/* The narrow fallback: two compact lines at the top of the panel, for the
 * terminal that is wide enough for the game but not for a clock column
 * beside a usable panel. Returns the rows used. */
static int draw_panel_clocks(const Game_t *g, Rect inner) {
  int32_t reading[2];
  clock_readings(g, reading);

  Color near = g->flipped ? BLACK : WHITE;
  Color far = (near == WHITE) ? BLACK : WHITE;
  const Color order[2] = {far, near};

  for (int k = 0; k < 2; k++) {
    Color side = order[k];
    int fg = (side == WHITE) ? C_CLOCK_WHITE_FG : C_CLOCK_BLACK_FG;
    int bg = (side == WHITE) ? C_CLOCK_WHITE_BG : C_CLOCK_BLACK_BG;
    if (!g->use_color) {
      fg = COLOR_DEFAULT;
      bg = COLOR_DEFAULT;
    }
    char time_text[CLOCK_FORMAT_MAX];
    clock_format(reading[side], time_text, sizeof(time_text));
    char line[32];
    snprintf(line, sizeof(line), " %-5s %8s ", side_name(side), time_text);
    draw_text(inner, 0, k, line, fg, bg, ATTR_NONE);
  }
  return 2;
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

/* top_rows is the compact clock fallback's two lines, or 0 — which is what
 * an untimed game always passes, so its panel is exactly what it was before
 * a clock existed. */
static void draw_panel(const Game_t *g, Rect r, int top_rows) {
  if (r.w < 8 || r.h < 4) {
    return;
  }
  Rect inner = rect_sub(r, 1, 0, r.w - 1, r.h);

  int y = 0;
  if (top_rows > 0) {
    y = draw_panel_clocks(g, inner);
  }
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

/* Defined below, in Turn flow — forward-declared here since the replay status
 * line states the same result text a live game's ending does. */
static const char *outcome_message(Outcome_t oc);

/* The result on the state, read the same way regardless of whether it came
 * off a save file or a game that just ended — a replay's state always
 * carries one, since only a finished game is ever opened as a replay. */
static Outcome_t state_result(const GameState *state) {
  return (Outcome_t){.reason = (Outcome_reason_t)state->result_reason,
                      .winner = state->result_winner};
}

static void draw_status_replay(const Game_t *g, Rect r) {
  char line[160];
  char mover_label[24];
  const char *mover = (side_to_move(g) == WHITE) ? "White" : "Black";
  int checked = in_check(&g->state->position, side_to_move(g));
  snprintf(mover_label, sizeof(mover_label), "%s%s", mover, checked ? " (check)" : "");

  draw_hline(r, 0, 0, r.w, 0x2500u, C_RULE, COLOR_DEFAULT, ATTR_NONE);

  /* The final position is the one with nothing left to step forward to, and
   * the only place the result is stated — in place of the side to move,
   * since the game is over and there is no turn left to name. */
  if (g->state->p_redo_head == NULL) {
    snprintf(line, sizeof(line), "%s", outcome_message(state_result(g->state)));
  } else {
    snprintf(line, sizeof(line), "%s to move", mover_label);
  }
  draw_text(r, 0, 1, line, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);

  Hint_t hints[] = {
      {"f", "flip", 1},
      {"u/←", "back", g->state->p_history_head != NULL},
      {"r/→", "forward", g->state->p_redo_head != NULL},
      {"h", "history", 1},
      {"?", "help", 1},
      {"q", "quit", 1},
  };
  draw_hints(r, 2, hints, (int)(sizeof(hints) / sizeof(hints[0])));
}

static void draw_status(const Game_t *g, Rect r) {
  if (g->mode == GAME_MODE_REPLAY) {
    draw_status_replay(g, r);
    return;
  }

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
  if (!layout_compute(r, glyphs_width(), clock_is_timed(game_clock(g)), &lay)) {
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

  /* An untimed game draws no clock at all, and its panel keeps the whole
   * width beside the board — the screen is exactly what it is without this
   * capability. A timed game gets the two rectangles when there is a column
   * for them, and the compact fallback inside the panel when there is not:
   * a timed game whose clocks are invisible must not be possible. */
  int timed = clock_is_timed(game_clock(g));
  int panel_top_rows = 0;
  if (timed) {
    if (lay.clock.w > 0) {
      draw_clocks(g, lay.clock, lay.board.h);
    } else {
      panel_top_rows = 2;
    }
  }
  draw_panel(g, lay.panel, panel_top_rows);
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
  case OUTCOME_TIMEOUT:
    return (oc.winner == WHITE) ? "Black ran out of time — White wins!"
                                : "White ran out of time — Black wins!";
  case OUTCOME_DRAW_TIMEOUT_INSUFFICIENT_MATERIAL:
    return "Draw — a clock ran out, with nothing left on the board to mate with.";
  case OUTCOME_IN_PROGRESS:
    break;
  }
  return "";
}

/* Every move begins the handover gesture: the board is about to flip and the
 * next player confirms with Space before it does. */
static void begin_turn(Game_t *g) { g->awaiting_handoff = 1; }

static void apply_outcome(Game_t *g, Outcome_t oc);

/* Whether a clock has reached zero, and the ending if it has.
 *
 * Consulted from the tick, which is the only place it can be discovered in
 * the case that matters — nobody is pressing anything — and again from
 * finish_move, so a move submitted in the same instant a clock ran out is
 * refused rather than played by a side that no longer has the time for it.
 *
 * Whether the opponent could ever mate with what they hold decides win or
 * draw. That is a question about one side, which is why it is
 * outcome_can_mate and not the both-sides insufficient-material test. */
static int check_flag_fall(Game_t *g, uint64_t now_ms) {
  if (g->mode != GAME_MODE_LIVE || g->game_over) {
    return 0;
  }
  Color loser = clock_expired_side(game_clock(g), now_ms);
  if (loser == NONE) {
    return 0;
  }
  Color winner = (loser == WHITE) ? BLACK : WHITE;
  clock_stop(game_clock(g), now_ms);

  Outcome_t oc;
  if (outcome_can_mate(&g->state->position, winner)) {
    oc = (Outcome_t){.reason = OUTCOME_TIMEOUT, .winner = winner};
  } else {
    oc = (Outcome_t){.reason = OUTCOME_DRAW_TIMEOUT_INSUFFICIENT_MATERIAL, .winner = NONE};
  }
  apply_outcome(g, oc);
  return 1;
}

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
  uint64_t now = term_now_ms();

  /* A side whose clock has reached zero cannot complete a move: the game
   * ended the moment the flag fell, whether or not a tick had got to it
   * first. */
  if (check_flag_fall(g, now)) {
    return;
  }

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
  /* The reading as of the move itself. The mover's clock keeps running from
   * here until the handover, which is the clock press, and the press
   * overwrites this node with what it produced — see press_clock_and_flip.
   * A move that ends the game never reaches a handover, and the reading
   * stored here is already the right one: both clocks stop at the move. */
  int32_t remaining[2];
  clock_snapshot(g, now, remaining);
  update_history(&state->p_history_head, from, to, move, remaining);

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
    /* Both clocks stop at the move, and no handover is asked for: there is
     * nobody left for a press to start. */
    clock_stop(game_clock(g), now);
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
static void do_save(Game_t *g, const char *name, int name_given) {
  /* The file records what the clocks read at the moment of writing, which
   * for a game saved partway through a turn is not what they read at the
   * start of it. Syncing charges the mover for the interval so far and
   * carries straight on, so saving neither costs nor returns any time. */
  clock_sync(game_clock(g), term_now_ms());
  save_perform(g->state, SAVE_STATUS_ONGOING, name, name_given, g->message, sizeof(g->message));
}

/* Shared by the name prompt this screen pushes and the one the quit picker
 * pushes: an empty (or all-space) submission always validates, since it just
 * means today's date. */
static int save_name_char_validate(const char *text, char *err, size_t err_len) {
  char trimmed[SAVE_NAME_BUF_LEN];
  save_name_trim(text, trimmed, sizeof(trimmed));
  if (trimmed[0] == '\0') {
    return 1;
  }
  return save_name_validate(trimmed, err, err_len);
}

static Cmd_t on_save_name_submit(void *ctx, const char *text) {
  Game_t *g = (Game_t *)ctx;
  char trimmed[SAVE_NAME_BUF_LEN];
  save_name_trim(text, trimmed, sizeof(trimmed));
  do_save(g, trimmed, trimmed[0] != '\0');
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t on_save_and_quit_name_submit(void *ctx, const char *text) {
  Game_t *g = (Game_t *)ctx;
  char trimmed[SAVE_NAME_BUF_LEN];
  save_name_trim(text, trimmed, sizeof(trimmed));
  do_save(g, trimmed, trimmed[0] != '\0');
  return (Cmd_t){CMD_QUIT, NULL};
}

/* Saving a game with no file yet opens the name prompt and saves from its
 * callback; a game that already has one is saved straight to it, exactly as
 * before. after_direct is what to return once a plain (no-prompt) save is
 * done — CMD_STAY for the save command during play, CMD_QUIT for "Save and
 * quit", which is why this is shared rather than duplicated between them. */
static Cmd_t save_flow(Game_t *g, Cmd_t (*on_submit)(void *ctx, const char *text),
                        Cmd_t after_direct) {
  if (!can_save(g)) {
    snprintf(g->message, sizeof(g->message), "Nothing to save yet — play a move first.");
    return CMD_STAY;
  }
  if (g->state->save_path[0] != '\0') {
    do_save(g, NULL, 0);
    return after_direct;
  }
  return (Cmd_t){CMD_PUSH, prompt_screen("Save game as:", "", SAVE_NAME_MAX_CODEPOINTS,
                                          save_name_char_validate, on_submit, NULL, g)};
}

static Cmd_t save_game_now(Game_t *g) { return save_flow(g, on_save_name_submit, CMD_STAY); }

static Cmd_t save_and_quit(Game_t *g) {
  return save_flow(g, on_save_and_quit_name_submit, (Cmd_t){CMD_QUIT, NULL});
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
      return save_and_quit(g);
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
/* Goes straight to the result screen — CMD_POP_REPLACE pops this confirm
 * overlay and replaces the game screen beneath it in one step, rather than
 * setting game_over and waiting for this screen's next event to notice (the
 * "press any key to continue" pause that path needs, and which resigning has
 * no reason to add: the player just confirmed exactly this). */
static Cmd_t on_resign_confirm(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  Outcome_t oc = {.reason = OUTCOME_RESIGNATION,
                  .winner = (side_to_move(g) == WHITE) ? BLACK : WHITE};
  return (Cmd_t){CMD_POP_REPLACE, gameover_screen(g->state, oc, g->flipped)};
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
/* Same reasoning as on_resign_confirm: straight to the result screen, no
 * extra keypress to acknowledge an outcome the player just agreed to. */
static Cmd_t on_draw_accept(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  Outcome_t oc = {.reason = OUTCOME_DRAW_AGREEMENT, .winner = NONE};
  return (Cmd_t){CMD_POP_REPLACE, gameover_screen(g->state, oc, g->flipped)};
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

/* The handover, which is the clock press: one gesture, one moment. The
 * mover's increment goes on, the incoming player's clock starts, the move's
 * history node records what that left both sides with, and the board flips
 * — in that order, all at the same now.
 *
 * The time between the move and this press is the mover's, exactly as it is
 * on a real clock: the handover is what stops their clock, so a slow handoff
 * is spent out of their own time and can flag them (see check_flag_fall,
 * which the tick runs throughout).
 *
 * In an untimed game every clock call here is a no-op and this is the
 * handover exactly as it has always been. */
static void press_clock_and_flip(Game_t *g) {
  uint64_t now = term_now_ms();

  clock_press(game_clock(g), now);

  History_node_t *last = NULL;
  for (History_node_t *p = g->state->p_history_head; p != NULL; p = p->p_next) {
    last = p;
  }
  if (last != NULL) {
    clock_snapshot(g, now, last->remaining_ms);
  }

  g->awaiting_handoff = 0;
  g->flipped = (side_to_move(g) == BLACK);
}

/* --- Input ---------------------------------------------------------------- */

/* Only f, u, r, the arrows, h, ?, q, Ctrl-L and F5 do anything here — every
 * other key, every click, and the wheel are dropped before any square could
 * be named, so no code path that could originate a move is reachable from a
 * replay. */
static Cmd_t replay_handle(Game_t *g, const Event_t *ev) {
  if (ev->type == EV_MOUSE) {
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }

  if (ev->key.name == KEY_LEFT) {
    if (replay_step_back(g->state)) {
      highlight_last_move(g);
    }
    return CMD_STAY;
  }
  if (ev->key.name == KEY_RIGHT) {
    if (replay_step_forward(g->state)) {
      highlight_last_move(g);
    }
    return CMD_STAY;
  }
  if (ev->key.name == KEY_F5) {
    render_force_repaint();
    return CMD_STAY;
  }
  if (ev->key.name != KEY_CHAR) {
    return CMD_STAY;
  }

  switch (ev->key.ch) {
  case 'f':
    g->flipped = !g->flipped;
    break;
  case 'u':
    if (replay_step_back(g->state)) {
      highlight_last_move(g);
    }
    break;
  case 'r':
    if (replay_step_forward(g->state)) {
      highlight_last_move(g);
    }
    break;
  case 'h':
    return (Cmd_t){CMD_PUSH, history_view_screen(g->state, 1)};
  case '?':
    return (Cmd_t){CMD_PUSH, replay_help_screen()};
  case 'q':
    return (Cmd_t){CMD_POP, NULL};
  case 12: /* Ctrl-L, same full repaint as live play */
    render_force_repaint();
    break;
  default:
    break;
  }
  return CMD_STAY;
}

static Cmd_t game_handle(void *ctx, const Event_t *ev) {
  Game_t *g = (Game_t *)ctx;

  if (g->mode == GAME_MODE_REPLAY) {
    return replay_handle(g, ev);
  }

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
      return save_game_now(g);
    case 'H':
      /* Shift-H, not h: h is a file name and belongs to the move field. */
      return (Cmd_t){CMD_PUSH, history_view_screen(g->state, 0)};
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
      press_clock_and_flip(g);
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

/* --- Time ----------------------------------------------------------------- */

/* How soon this screen needs waking. 100 ms while a clock is actually
 * running in live play, and -1 — a true indefinite block, no frames and no
 * processor time — in every other case: an untimed game has no clock, a
 * replay's clocks are read rather than run, a game that has ended has
 * stopped both, and a terminal too small to draw the game is one where no
 * time may be charged at all. */
static int game_wake_in_ms(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  if (g->mode != GAME_MODE_LIVE || g->game_over) {
    return -1;
  }
  if (!clock_is_timed(game_clock(g))) {
    return -1;
  }
  if (!game_fits()) {
    return -1;
  }
  return (clock_running_side(game_clock(g)) == NONE) ? -1 : CLOCK_WAKE_MS;
}

/* Time passing, which this screen is told about whether or not it is on top
 * — an overlay covering the board does not stop the turn, so help, the move
 * list, the promotion picker, a resignation or draw confirmation, the save
 * prompt and the quit picker all leave the clock running underneath them.
 *
 * The one thing that does stop it is the game being undrawable: while the
 * terminal is too small the clock is held, and when the space comes back the
 * running side's interval restarts from now, so nothing is charged for the
 * gap. Restarting the interval rather than subtracting the gap is what keeps
 * every reading a function of two timestamps. */
static void game_tick(void *ctx, uint64_t now_ms) {
  Game_t *g = (Game_t *)ctx;
  if (g->mode != GAME_MODE_LIVE || g->game_over) {
    return;
  }
  if (!clock_is_timed(game_clock(g))) {
    return;
  }
  if (!game_fits()) {
    clock_hold(game_clock(g), now_ms);
    return;
  }
  clock_resume(game_clock(g), now_ms);

  /* Sets game_over and pending_outcome rather than moving the stack: this
   * runs for a covered screen too, and a screen that is not on top must not
   * push, pop or replace anything. The status line states the result at
   * once, and the result screen arrives on the next event this screen
   * handles — the path every ending discovered under an overlay already
   * takes. */
  check_flag_fall(g, now_ms);
}

/* --- Construction ------------------------------------------------------- */

static void game_on_enter(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  clear_entry(g);
  g->message[0] = '\0';
  g->flipped = (side_to_move(g) == BLACK);
  g->use_color = term_supports_color();
  highlight_last_move(g);
  /* The side to move is running from the moment the board appears, with no
   * press needed to begin — a fresh game and a loaded one alike. A loaded
   * game resumes from what it had left, mid-turn included, since that is
   * what its remaining times already hold. */
  if (!g->game_over) {
    clock_start(game_clock(g), side_to_move(g), term_now_ms());
  }
}

/* A replay's own orientation and turn rules: fixed until F turns it (no
 * side-to-move handoff, since there is no handover in a replay), and the
 * board opens on the starting position with the whole game still ahead to
 * step forward through — rewound with the same replay_step_back() stepping
 * uses, rather than a second way to move history onto the redo list. */
static void replay_on_enter(void *ctx) {
  Game_t *g = (Game_t *)ctx;
  clear_entry(g);
  g->message[0] = '\0';
  g->flipped = 0;
  /* Nothing runs in a replay: the readings come off the history nodes the
   * step moved to, not from a clock being charged. */
  clock_stop(game_clock(g), 0);
  g->use_color = term_supports_color();
  while (replay_step_back(g->state)) {
  }
  highlight_last_move(g);
}

static Screen *construct_game_screen(GameState *state, Game_mode_t mode) {
  static Screen screen;

  memset(&g_game, 0, sizeof(g_game));
  g_game.state = state;
  g_game.mode = mode;
  g_game.sel_i = -1;
  g_game.sel_j = -1;
  g_game.last_from_i = -1;
  g_game.last_from_j = -1;
  g_game.last_to_i = -1;
  g_game.last_to_j = -1;
  /* No cursor until an arrow key asks for one; see move_cursor. Unused in a
   * replay, which never activates the cursor at all. */
  g_game.cursor_row = CURSOR_HOME_ROW;
  g_game.cursor_col = CURSOR_HOME_COL;
  g_game.cursor_active = 0;

  screen.on_enter = (mode == GAME_MODE_REPLAY) ? replay_on_enter : game_on_enter;
  screen.on_exit = NULL;
  screen.handle = game_handle;
  screen.render = game_render;
  screen.ctx = &g_game;
  screen.opaque = 1;
  /* Set for both modes rather than only for live play: a replay answers -1
   * and does nothing on a tick, which is stated in one place here instead of
   * left to whichever mode happened to be constructed last. */
  screen.wake_in_ms = game_wake_in_ms;
  screen.tick = game_tick;
  return &screen;
}

Screen *game_screen(GameState *state) { return construct_game_screen(state, GAME_MODE_LIVE); }

Screen *replay_screen(GameState *state) {
  return construct_game_screen(state, GAME_MODE_REPLAY);
}
