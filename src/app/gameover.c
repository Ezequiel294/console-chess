#include "app/gameover.h"

#include "app/game.h"
#include "app/history_view.h"
#include "app/settings.h"
#include "core/history.h"
#include "core/position.h"
#include "ui/glyphs.h"
#include "ui/render.h"
#include "ui/term.h"

#include <stdio.h>
#include <string.h>

#define C_PIECE_WHITE 231
#define C_PIECE_BLACK 245
#define C_RULE 244
#define C_LABEL 246
#define C_TITLE 250
#define MARK_CHECK '!'

#define GAMEOVER_ITEM_COUNT 3

typedef struct {
  GameState *state;
  Outcome_t outcome;
  int flipped;

  int selected;
  int row_y[GAMEOVER_ITEM_COUNT]; /* where each option was last drawn, for a click */
} GameOver_t;

static GameOver_t g_gameover;

static void free_state_lists(GameState *s) {
  free_captures(s->p_captures_white_head);
  free_captures(s->p_captures_black_head);
  free_history(s->p_history_head);
  free_hash_history(s->p_hash_history_head);
  free_history(s->p_redo_head);
}

static const char *result_text(Outcome_t oc, char *buf, size_t n) {
  const char *who = (oc.winner == WHITE) ? "White" : (oc.winner == BLACK) ? "Black" : NULL;
  switch (oc.reason) {
  case OUTCOME_CHECKMATE:
    snprintf(buf, n, "Checkmate — %s wins", who);
    break;
  case OUTCOME_STALEMATE:
    snprintf(buf, n, "Draw — stalemate");
    break;
  case OUTCOME_DRAW_FIFTY_MOVE:
    snprintf(buf, n, "Draw — fifty-move rule");
    break;
  case OUTCOME_DRAW_INSUFFICIENT_MATERIAL:
    snprintf(buf, n, "Draw — insufficient material");
    break;
  case OUTCOME_DRAW_REPETITION:
    snprintf(buf, n, "Draw — threefold repetition");
    break;
  case OUTCOME_RESIGNATION:
    snprintf(buf, n, "%s wins — by resignation", who);
    break;
  case OUTCOME_DRAW_AGREEMENT:
    snprintf(buf, n, "Draw — by agreement");
    break;
  case OUTCOME_IN_PROGRESS:
    snprintf(buf, n, "");
    break;
  }
  return buf;
}

/* The one square per side that can be "in check" — a duplicate of game.c's
 * own find_king rather than a shared export, the same call it made to keep
 * draw_final_board self-contained. */
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

/* A plain board: pieces and rank rules, no selection or legal-move
 * highlights, no labels, no minimum-size requirement — the result the player
 * is looking at, not a board they can act on — but the checked king (a
 * checkmate always leaves one) gets the same tinted square and "!" mark the
 * live board uses, since the player was looking at exactly that highlight
 * the instant before the game ended and losing it here would look like the
 * position had quietly changed. */
static void draw_final_board(const Position *pos, Rect r, int flipped, int checked_i,
                              int checked_j) {
  int gw = glyphs_width();
  int sq_w = gw + 2; /* one cell of padding either side, as on the game board */
  int grid_w = 8 * (sq_w + 1) + 1;
  const Palette_t *pal = settings_palette();
  int use_color = term_supports_color();

  for (int row = 0; row <= 8; row++) {
    int y = row * 2;
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
      draw_glyph(r, x, y, ch, 1, C_RULE, COLOR_DEFAULT, ATTR_NONE);
    }
  }

  for (int row = 0; row < 8; row++) {
    int i = flipped ? 7 - row : row;
    int y = row * 2 + 1;
    for (int col = 0; col < 8; col++) {
      int j = flipped ? 7 - col : col;
      int x = col * (sq_w + 1);
      draw_glyph(r, x, y, 0x2502u, 1, C_RULE, COLOR_DEFAULT, ATTR_NONE);

      int is_check_sq = (i == checked_i && j == checked_j);
      int bg = (is_check_sq && use_color) ? pal->square_check : COLOR_DEFAULT;

      Piece_t piece = pos->board[i][j];
      if (bg != COLOR_DEFAULT) {
        for (int k = 0; k < sq_w; k++) {
          draw_glyph(r, x + 1 + k, y, ' ', 1, COLOR_DEFAULT, bg, ATTR_NONE);
        }
      }
      if (piece.type != FREE) {
        int fg = (piece.color == BLACK) ? C_PIECE_BLACK : C_PIECE_WHITE;
        draw_glyph(r, x + 1 + (sq_w - gw) / 2, y, piece_glyph(piece.type, piece.color), gw, fg,
                   bg, ATTR_NONE);
      }
      /* Additive in both modes, same as the live board: the one marking that
       * is never only a colour fallback. */
      if (is_check_sq) {
        int mark_fg = use_color ? pal->mark_check_fg : COLOR_DEFAULT;
        draw_glyph(r, x + sq_w, y, MARK_CHECK, 1, mark_fg, bg, ATTR_BOLD);
      }
    }
    draw_glyph(r, grid_w - 1, y, 0x2502u, 1, C_RULE, COLOR_DEFAULT, ATTR_NONE);
  }
}

static const char *const MENU_LABELS[GAMEOVER_ITEM_COUNT] = {
    "New Game",
    "Review History",
    "Return to Menu",
};

static void gameover_render(void *ctx, Rect r) {
  GameOver_t *g = (GameOver_t *)ctx;

  draw_fill(r, ' ', COLOR_DEFAULT, COLOR_DEFAULT, ATTR_NONE);
  draw_text(r, 1, 0, "Game Over", C_TITLE, COLOR_DEFAULT, ATTR_BOLD);

  char text[64];
  result_text(g->outcome, text, sizeof(text));
  draw_text(r, 1, 2, text, COLOR_DEFAULT, COLOR_DEFAULT, ATTR_BOLD);
  if (outcome_is_player_chosen(g->outcome.reason)) {
    draw_text(r, 1, 3, "(the players' choice, not forced by the rules)", C_LABEL, COLOR_DEFAULT,
              ATTR_DIM);
  }

  /* Checkmate always leaves the losing side's king in check; every other
   * ending leaves no king in check at all. */
  int checked_i = -1, checked_j = -1;
  if (g->outcome.reason == OUTCOME_CHECKMATE) {
    Color loser = (g->outcome.winner == WHITE) ? BLACK : WHITE;
    find_king(&g->state->position, loser, &checked_i, &checked_j);
  }

  /* The menu (GAMEOVER_ITEM_COUNT rows) plus its hint line claim the bottom
   * of the screen; the board gets whatever is left above them, with one
   * blank row of separation. */
  int menu_y = r.h - 1 - GAMEOVER_ITEM_COUNT;
  Rect board_area = rect_sub(r, 1, 5, r.w - 2, menu_y - 5 - 1);
  draw_final_board(&g->state->position, board_area, g->flipped, checked_i, checked_j);

  for (int k = 0; k < GAMEOVER_ITEM_COUNT; k++) {
    uint8_t attr = (k == g->selected) ? ATTR_REVERSE : ATTR_NONE;
    draw_text(r, 1, menu_y + k, MENU_LABELS[k], COLOR_DEFAULT, COLOR_DEFAULT, attr);
    g->row_y[k] = menu_y + k;
  }
  app_draw_bottom_hint(r, "↑/↓ + Enter, or click to select");
}

static void reset_for_new_game(GameState *s) {
  free_state_lists(s);
  *s = (GameState){0};
  position_init(&s->position);
  s->start_position = s->position;
  push_hash(&s->p_hash_history_head, s->position.hash);
}

static Cmd_t activate(GameOver_t *g, int k) {
  switch (k) {
  case 0:
    reset_for_new_game(g->state);
    return (Cmd_t){CMD_REPLACE, game_screen(g->state)};
  case 1:
    return (Cmd_t){CMD_PUSH, history_view_screen(g->state)};
  case 2:
    return (Cmd_t){CMD_POP, NULL};
  default:
    return CMD_STAY;
  }
}

static void gameover_on_enter(void *ctx) { ((GameOver_t *)ctx)->selected = 0; }

static Cmd_t gameover_handle(void *ctx, const Event_t *ev) {
  GameOver_t *g = (GameOver_t *)ctx;

  /* A click only moves the highlight, the same as an arrow key — it does not
   * choose the option, so a stray click during play cannot be mistaken for a
   * confirmed choice. Enter is the one way to act on whatever is selected. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    for (int k = 0; k < GAMEOVER_ITEM_COUNT; k++) {
      if (ev->mouse.row == g->row_y[k]) {
        g->selected = k;
        break;
      }
    }
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_UP) {
    g->selected = (g->selected - 1 + GAMEOVER_ITEM_COUNT) % GAMEOVER_ITEM_COUNT;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_DOWN) {
    g->selected = (g->selected + 1) % GAMEOVER_ITEM_COUNT;
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ENTER) {
    return activate(g, g->selected);
  }
  if (ev->key.name == KEY_ESCAPE) {
    return activate(g, 2);
  }
  return CMD_STAY;
}

Screen *gameover_screen(GameState *state, Outcome_t outcome, int flipped) {
  static Screen screen;

  g_gameover.state = state;
  g_gameover.outcome = outcome;
  g_gameover.flipped = flipped;
  g_gameover.selected = 0;

  screen.on_enter = gameover_on_enter;
  screen.on_exit = NULL;
  screen.handle = gameover_handle;
  screen.render = gameover_render;
  screen.ctx = &g_gameover;
  screen.opaque = 1;
  return &screen;
}
