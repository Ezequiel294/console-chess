#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <stdint.h>

/* The byte stream from the terminal, turned into events.
 *
 * This module owns the only read of standard input in the program. Nothing else
 * reads it, directly or through buffered library calls: two readers each hold
 * their own buffer, and bytes then go missing or arrive out of order depending
 * on which buffer happened to swallow them.
 *
 * Game logic never sees a raw byte. Every escape sequence is consumed through
 * its terminator, and one this parser does not recognise produces no event at
 * all. That is what makes stray terminal output — a wheel report, a device
 * status answer, a sequence from a terminal nobody tested on — structurally
 * incapable of arriving as a keystroke, rather than merely unlikely to.
 */

typedef enum {
  EV_KEY,
  EV_MOUSE,
  EV_RESIZE,
  EV_PASTE,
  EV_EOF
} Event_type_t;

typedef enum {
  KEY_CHAR = 0, /* a printable character; the codepoint is in ch */
  KEY_ENTER,
  KEY_ESCAPE,
  KEY_BACKSPACE,
  KEY_TAB,
  KEY_UP,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_HOME,
  KEY_END,
  KEY_INSERT,
  KEY_DELETE,
  KEY_PAGE_UP,
  KEY_PAGE_DOWN,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12
} Key_name_t;

typedef enum {
  MOUSE_PRESS,
  MOUSE_RELEASE,
  MOUSE_MOTION,
  MOUSE_WHEEL
} Mouse_kind_t;

typedef struct {
  Event_type_t type;

  struct {
    Key_name_t name;
    uint32_t ch; /* codepoint, when name is KEY_CHAR */
    int alt;     /* the key arrived with Alt held */
  } key;

  struct {
    Mouse_kind_t kind;
    int button;    /* 0 left, 1 middle, 2 right */
    int col;       /* zero-based, matching render coordinates */
    int row;
    int wheel;     /* +1 up, -1 down; only when kind is MOUSE_WHEEL */
  } mouse;

  struct {
    /* Owned by input.c and valid until the next input_next call. Pasted text is
     * delivered whole, once, so it can never arrive as a run of keystrokes. */
    const char *text;
    size_t len;
  } paste;
} Event_t;

/* Blocks until an event is available. A terminal resize interrupts the wait and
 * arrives as EV_RESIZE without a key being pressed; a partly read escape
 * sequence survives the interruption and is resumed, not restarted.
 *
 * End of input is EV_EOF, which the caller is expected to treat as a request to
 * quit. This is also what makes the game scriptable. */
Event_t input_next(void);

/* Releases the paste buffer. */
void input_shutdown(void);

#endif /* INPUT_H */
