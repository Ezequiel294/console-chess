#ifndef SETTINGS_H
#define SETTINGS_H

#include "app/app.h"

/* The settings the game has: piece glyph set and the board's highlight
 * colours. Deliberately not a general preferences framework — two
 * booleans/enums do not need one. Stored as a small text file beside the
 * save; a write failure never blocks the change from applying for the
 * session, only from persisting.
 *
 * There is no setting for the turn handover: the board always waits for
 * Space between turns (see game.c), since that gesture is also the
 * "I'm ready" signal a future timed mode needs and must not be optional.
 */

typedef enum { PALETTE_CLASSIC, PALETTE_OCEAN, PALETTE_COUNT } Palette_id_t;

/* xterm-256 indices for the board's highlight tints — the C_SQUARE_* and
 * C_MARK_CHECK_FG constants game.c used to hardcode, now one of these two
 * presets, switchable at runtime. */
typedef struct {
  int square_last;
  int square_selected;
  int square_capture;
  int square_check;
  int mark_check_fg;
} Palette_t;

/* Loads settings.txt if present, else leaves every setting at its default
 * (icon glyphs, classic palette). Call once at startup, before any of the
 * getters below are read. */
void settings_load(void);

/* The glyph width term_probe_glyph_width found at startup, for restoring icon
 * mode's width after a session that switched to ASCII and back. Ignored while
 * ASCII is in effect, since ASCII glyphs are always one cell. */
void settings_set_icon_width(int width);

int settings_ascii(void);
Palette_id_t settings_palette_id(void);
const Palette_t *settings_palette(void);

/* Applies immediately and attempts to persist. *persist_failed is set to 1 on
 * a write failure (the change still applies for the session) and to 0 on
 * success; pass NULL if the caller does not need to know. */
void settings_set_ascii(int on, int *persist_failed);
void settings_set_palette_id(Palette_id_t id, int *persist_failed);

Screen *settings_screen(void);

#endif /* SETTINGS_H */
