#ifndef TOOSMALL_H
#define TOOSMALL_H

#include "app/app.h"

/* Shown instead of the game when the terminal cannot hold it.
 *
 * It replaces the display rather than clipping it, so there is never a
 * half-drawn board on screen to be misread as the game having broken.
 */
Screen *toosmall_screen(void);

#endif /* TOOSMALL_H */
