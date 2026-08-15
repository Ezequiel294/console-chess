
/*
Ezequiel Buck
Programming Principles
Final Project
Console Chess Game
*/


#include "app/app.h"
#include "app/game.h"
#include "app/toosmall.h"
#include "core/history.h"
#include "core/position.h"
#include "types.h"
#include "ui/glyphs.h"
#include "ui/input.h"
#include "ui/render.h"
#include "ui/term.h"
#include "version.h"

#include <stdio.h>
#include <string.h>

#define PROGRAM_NAME "Console Chess"

static void print_version(void) {
  printf("%s %s\n", PROGRAM_NAME, chess_version());
}

// Lists every accepted option, including --version, so --help stays the
// single place usage has to be updated when an option is added.
static void print_usage(void) {
  printf("Usage: console-chess [OPTION]\n");
  printf("\n");
  printf("      --ascii     draw pieces as letters instead of icons,\n");
  printf("                  for terminals without a Nerd Font\n");
  printf("  -v, --version   print the version and exit\n");
  printf("  -h, --help      print this help and exit\n");
}

int main(int argc, char **argv) {
  int ascii = 0;

  // Handled before terminal setup or file access, so a broken environment
  // cannot affect these options.
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
      print_version();
      return 0;
    }
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage();
      return 0;
    }
    if (strcmp(argv[i], "--ascii") == 0) {
      ascii = 1;
      continue;
    }
    printf("Unrecognised option: %s\n\n", argv[i]);
    print_usage();
    return 1;
  }

  // Checked before anything is written, so a redirected run leaves no escape
  // sequences in the redirected stream.
  if (!term_is_interactive()) {
    fprintf(stderr, "%s needs an interactive terminal.\n", PROGRAM_NAME);
    return 1;
  }

  // Restoration is armed before the first mode change, so there is no window in
  // which the program can die having changed the terminal but not yet arranged
  // to change it back.
  if (!term_init()) {
    fprintf(stderr, "Could not read the terminal's settings.\n");
    return 1;
  }

  glyphs_use_ascii(ascii);
  if (!ascii) {
    // Asked on the primary screen, before the alternate one is entered, so the
    // question and its answer never appear in the game display.
    glyphs_set_width(term_probe_glyph_width(glyph_probe_sample()));
  }

  if (!term_enter()) {
    fprintf(stderr, "Could not put the terminal into full-screen mode.\n");
    return 1;
  }

  // main owns the game. The screen borrows it by address, so there is never a
  // second copy of a list head to get out of step.
  GameState state = {0};
  position_init(&state.position);
  push_hash(&state.p_hash_history_head, state.position.hash);

  app_set_too_small_screen(toosmall_screen());
  int status = app_run(game_screen(&state));

  free_captures(state.p_captures_white_head);
  free_captures(state.p_captures_black_head);
  free_history(state.p_history_head);
  free_hash_history(state.p_hash_history_head);

  input_shutdown();
  render_shutdown();
  term_restore();

  return status;
}
