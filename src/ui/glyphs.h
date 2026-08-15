#ifndef GLYPHS_H
#define GLYPHS_H

#include "types.h"

#include <stdint.h>

/* The one place a piece is turned into something a terminal can show.
 *
 * Two tables behind one lookup: Nerd Font icons, and plain letters for
 * terminals without that font. Which is in use is a launch-time choice, and it
 * is the only difference between the two modes — every other part of the
 * display is identical, which is what keeps the fallback honest.
 */

/* Selects the ASCII table. Call before the glyph width is decided: ASCII
 * letters are always one cell, so the width probe is skipped in this mode. */
void glyphs_use_ascii(int on);
int glyphs_is_ascii(void);

/* The codepoint for a piece. Empty squares and unknown colours are a space. */
uint32_t piece_glyph(Piece_type_t type, Color color);

/* A piece glyph as UTF-8, for the terminal's width probe. */
const char *glyph_probe_sample(void);

/* Cells a piece glyph occupies: the probed width, or 1 in ASCII mode. */
void glyphs_set_width(int width);
int glyphs_width(void);

#endif /* GLYPHS_H */
