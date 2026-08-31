#include "app/savedgames.h"

#include "app/prompt.h"
#include "app/save.h"
#include "core/chessclock.h"
#include "core/history.h"
#include "ui/render.h"

#include <stdio.h>
#include <string.h>

#define C_BOX_BG 236
#define C_BOX_FG 250
#define C_LABEL 246

#define MAX_ENTRIES 512

typedef struct {
  Saved_game_entry_t entries[MAX_ENTRIES];
  int count;
  int selected;

  /* Where the list was last drawn, so a click can be hit-tested against what
   * is actually on screen — the same reasoning game.c's layout cache uses
   * for the board. */
  int list_y;
  int visible_rows;

  Cmd_t (*on_loaded)(void *ctx, GameState loaded);
  void *ctx;
  char message[96];
} SavedGames_t;

static SavedGames_t g_savedgames;

/* What the rename prompt needs once the player answers — captured at the
 * moment 'r' is pressed rather than re-read from g_savedgames.selected later,
 * since the list itself is what gets rebuilt once the rename completes. */
typedef struct {
  SavedGames_t *list;
  char path[300];
  Save_status_t status;
} Rename_ctx_t;

static Rename_ctx_t g_rename_ctx;

static void clamp_selected(SavedGames_t *s) {
  if (s->selected < 0) {
    s->selected = 0;
  }
  if (s->selected >= s->count) {
    s->selected = s->count - 1;
  }
}

static Cmd_t load_selected(SavedGames_t *s) {
  if (s->count == 0 || s->selected < 0 || s->selected >= s->count) {
    return CMD_STAY;
  }
  Saved_game_entry_t *e = &s->entries[s->selected];
  GameState loaded = {0};
  Save_read_result_t r = save_read(e->path, &loaded);
  if (r.status != SAVE_READ_OK) {
    save_read_message(r, s->message, sizeof(s->message));
    return CMD_STAY;
  }
  /* Carries the path and name forward so continuing this game and saving it
   * again updates this same file instead of starting a new one (see
   * GameState.save_path and game.c's save_game_now). The name comes from the
   * filename (e->name), not the file's contents — save_read never sets it. */
  snprintf(loaded.save_path, sizeof(loaded.save_path), "%s", e->path);
  snprintf(loaded.name, sizeof(loaded.name), "%s", e->name);
  return s->on_loaded(s->ctx, loaded);
}

static int rename_name_char_validate(const char *text, char *err, size_t err_len) {
  char trimmed[SAVE_NAME_BUF_LEN];
  save_name_trim(text, trimmed, sizeof(trimmed));
  if (trimmed[0] == '\0') {
    return 1; /* empty means today's date, always valid */
  }
  return save_name_validate(trimmed, err, err_len);
}

static Cmd_t on_rename_submit(void *ctx, const char *text) {
  Rename_ctx_t *rc = (Rename_ctx_t *)ctx;
  SavedGames_t *s = rc->list;

  char trimmed[SAVE_NAME_BUF_LEN];
  save_name_trim(text, trimmed, sizeof(trimmed));

  GameState loaded = {0};
  Save_read_result_t r = save_read(rc->path, &loaded);
  if (r.status != SAVE_READ_OK) {
    save_read_message(r, s->message, sizeof(s->message));
    return (Cmd_t){CMD_POP, NULL};
  }
  snprintf(loaded.save_path, sizeof(loaded.save_path), "%s", rc->path);

  int ok = save_perform(&loaded, rc->status, trimmed, trimmed[0] != '\0', s->message,
                         sizeof(s->message));
  free_captures(loaded.p_captures_white_head);
  free_captures(loaded.p_captures_black_head);
  free_history(loaded.p_history_head);
  free_hash_history(loaded.p_hash_history_head);
  free_history(loaded.p_redo_head);

  if (ok) {
    snprintf(s->message, sizeof(s->message), "Renamed.");
  }
  s->count = save_list_games(s->entries, MAX_ENTRIES);
  clamp_selected(s);
  return (Cmd_t){CMD_POP, NULL};
}

static Cmd_t rename_selected(SavedGames_t *s) {
  if (s->count == 0 || s->selected < 0 || s->selected >= s->count) {
    return CMD_STAY;
  }
  Saved_game_entry_t *e = &s->entries[s->selected];
  g_rename_ctx.list = s;
  snprintf(g_rename_ctx.path, sizeof(g_rename_ctx.path), "%s", e->path);
  g_rename_ctx.status = e->status;
  return (Cmd_t){CMD_PUSH,
                 prompt_screen("Rename to:", e->name, SAVE_NAME_MAX_CODEPOINTS,
                               rename_name_char_validate, on_rename_submit, NULL, &g_rename_ctx)};
}

static void savedgames_on_enter(void *ctx) {
  SavedGames_t *s = (SavedGames_t *)ctx;
  s->count = save_list_games(s->entries, MAX_ENTRIES);
  s->selected = 0;
  s->message[0] = '\0';
}

static void savedgames_render(void *ctx, Rect r) {
  SavedGames_t *s = (SavedGames_t *)ctx;

  draw_fill(r, ' ', COLOR_DEFAULT, C_BOX_BG, ATTR_NONE);
  draw_text(r, 1, 0, "Load Game", C_BOX_FG, C_BOX_BG, ATTR_BOLD);
  /* PgUp/PgDn/Home/End still work (see savedgames_handle); they are simply
   * not advertised — a hint line naming every key that does something is one
   * nobody reads. */
  draw_text(r, 1, r.h - 1, "↑/↓ + Enter, or click to select  ·  r rename  ·  Esc menu", C_LABEL,
            C_BOX_BG, ATTR_DIM);

  Rect list = rect_sub(r, 1, 2, r.w - 2, r.h - 3);
  s->list_y = list.y;
  s->visible_rows = list.h;
  if (list.h < 1) {
    return;
  }

  if (s->count == 0) {
    draw_text(list, 0, 0, "No saved games yet — save one from a game in progress with 's'.",
              C_LABEL, C_BOX_BG, ATTR_NONE);
    if (s->message[0] != '\0') {
      draw_text(list, 0, 2, s->message, C_BOX_FG, C_BOX_BG, ATTR_NONE);
    }
    return;
  }

  clamp_selected(s);
  int scroll_row = 0;
  if (s->selected >= list.h) {
    scroll_row = s->selected - list.h + 1;
  }

  for (int row = 0; row < list.h && scroll_row + row < s->count; row++) {
    int k = scroll_row + row;
    Saved_game_entry_t *e = &s->entries[k];
    const char *status_str = (e->status == SAVE_STATUS_FINISHED) ? "finished" : "ongoing";
    char line[80 + SAVE_NAME_BUF_LEN];
    if (e->readable) {
      /* Empty for an untimed game, which is then shown exactly as it was
       * before saves carried a clock — no placeholder in place of a control
       * it does not have. */
      char control[CLOCK_CONTROL_MAX];
      clock_format_control(&e->clock, control, sizeof(control));
      snprintf(line, sizeof(line), "%-8s  %-24s  %3d moves  %s", status_str, e->name,
               e->move_count, control);
    } else {
      snprintf(line, sizeof(line), "%-8s  %-24s  (could not be read)", status_str, e->name);
    }
    uint8_t attr = (k == s->selected) ? ATTR_REVERSE : ATTR_NONE;
    draw_text(list, 0, row, line, COLOR_DEFAULT, C_BOX_BG, attr);
  }

  if (s->message[0] != '\0') {
    draw_text(r, 1, r.h - 2, s->message, C_BOX_FG, C_BOX_BG, ATTR_NONE);
  }
}

static Cmd_t savedgames_handle(void *ctx, const Event_t *ev) {
  SavedGames_t *s = (SavedGames_t *)ctx;

  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_WHEEL) {
    s->selected -= ev->mouse.wheel;
    clamp_selected(s);
    return CMD_STAY;
  }
  /* A click only moves the highlight, the same as an arrow key — it does not
   * load the game, so a stray click cannot be mistaken for a confirmed
   * choice. Enter is the one way to load whatever is selected. */
  if (ev->type == EV_MOUSE && ev->mouse.kind == MOUSE_PRESS && ev->mouse.button == 0) {
    int row = ev->mouse.row - s->list_y;
    if (row < 0 || row >= s->visible_rows) {
      return CMD_STAY;
    }
    int scroll_row = (s->selected >= s->visible_rows) ? s->selected - s->visible_rows + 1 : 0;
    int k = scroll_row + row;
    if (k < 0 || k >= s->count) {
      return CMD_STAY;
    }
    s->selected = k;
    return CMD_STAY;
  }
  if (ev->type != EV_KEY) {
    return CMD_STAY;
  }
  if (ev->key.name == KEY_ESCAPE) {
    return (Cmd_t){CMD_POP, NULL};
  }
  if (ev->key.name == KEY_CHAR && (ev->key.ch == 'r' || ev->key.ch == 'R')) {
    return rename_selected(s);
  }
  switch (ev->key.name) {
  case KEY_UP:
    s->selected--;
    clamp_selected(s);
    return CMD_STAY;
  case KEY_DOWN:
    s->selected++;
    clamp_selected(s);
    return CMD_STAY;
  case KEY_PAGE_UP:
    s->selected -= s->visible_rows > 0 ? s->visible_rows : 10;
    clamp_selected(s);
    return CMD_STAY;
  case KEY_PAGE_DOWN:
    s->selected += s->visible_rows > 0 ? s->visible_rows : 10;
    clamp_selected(s);
    return CMD_STAY;
  case KEY_HOME:
    s->selected = 0;
    return CMD_STAY;
  case KEY_END:
    s->selected = s->count - 1;
    clamp_selected(s);
    return CMD_STAY;
  case KEY_ENTER:
    return load_selected(s);
  default:
    return CMD_STAY;
  }
}

Screen *savedgames_screen(Cmd_t (*on_loaded)(void *ctx, GameState loaded), void *ctx) {
  static Screen screen;

  g_savedgames.on_loaded = on_loaded;
  g_savedgames.ctx = ctx;
  g_savedgames.count = 0;
  g_savedgames.selected = 0;
  g_savedgames.message[0] = '\0';

  screen.on_enter = savedgames_on_enter;
  screen.on_exit = NULL;
  screen.handle = savedgames_handle;
  screen.render = savedgames_render;
  screen.ctx = &g_savedgames;
  screen.opaque = 1;
  return &screen;
}
