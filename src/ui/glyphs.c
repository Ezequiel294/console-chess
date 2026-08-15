#include "ui/glyphs.h"

/* Indexed [color][type]. FREE is present in both rows so that an empty square
 * needs no special case at the call sites. */
static const uint32_t ICON_GLYPHS[2][7] = {
    [WHITE] = {[PAWN] = 0xF0859u, [ROOK] = 0xF085Bu, [KNIGHT] = 0xF0858u, [BISHOP] = 0xF085Cu, [QUEEN] = 0xF085Au, [KING] = 0xF0857u, [FREE] = ' '},
    [BLACK] = {[PAWN] = 0xED64u, [ROOK] = 0xED66u, [KNIGHT] = 0xED63u, [BISHOP] = 0xED60u, [QUEEN] = 0xED65u, [KING] = 0xED62u, [FREE] = ' '},
};

/* Case is the colour marker, as in FEN: uppercase white, lowercase black. */
static const uint32_t ASCII_GLYPHS[2][7] = {
    [WHITE] = {[PAWN] = 'P', [ROOK] = 'R', [KNIGHT] = 'N', [BISHOP] = 'B', [QUEEN] = 'Q', [KING] = 'K', [FREE] = ' '},
    [BLACK] = {[PAWN] = 'p', [ROOK] = 'r', [KNIGHT] = 'n', [BISHOP] = 'b', [QUEEN] = 'q', [KING] = 'k', [FREE] = ' '},
};

static int g_ascii = 0;
static int g_width = 1;

void glyphs_use_ascii(int on) {
  g_ascii = on ? 1 : 0;
  if (g_ascii) {
    g_width = 1;
  }
}

int glyphs_is_ascii(void) { return g_ascii; }

uint32_t piece_glyph(Piece_type_t type, Color color) {
  /* NONE is the colour of an empty square, and the only other Color there is. */
  if (color != WHITE && color != BLACK) {
    return ' ';
  }
  return g_ascii ? ASCII_GLYPHS[color][type] : ICON_GLYPHS[color][type];
}

/* U+F0857, the white king: a Private Use Area codepoint, which is exactly the
 * kind wcwidth() and the terminal disagree about. Measuring any other piece
 * would measure the same disagreement. */
const char *glyph_probe_sample(void) { return "\xF3\xB0\xA1\x97"; }

void glyphs_set_width(int width) {
  if (g_ascii) {
    return;
  }
  g_width = (width == 2) ? 2 : 1;
}

int glyphs_width(void) { return g_width; }
