#ifndef CONFIRM_H
#define CONFIRM_H

#include "app/app.h"

/* A yes/no overlay: one message, two answers.
 *
 * Composites over whatever is beneath it, same as the promotion overlay.
 * Every mid-game confirmation — quitting, resigning, overwriting a slot — is
 * this screen with a different message and a different pair of callbacks, so
 * there is exactly one place Enter/Esc and Y/N are wired up rather than one
 * per confirmation.
 *
 * Driven like every other choice screen: ←/→ moves the highlight, a click
 * moves it too, and Enter is the only thing that answers. Escape is always
 * "no".
 *
 * Exactly one of on_yes/on_no is called, and this screen returns whatever it
 * returns — not always CMD_POP, since quitting needs CMD_QUIT to take effect
 * immediately rather than waiting for the next event once this overlay has
 * already closed. A NULL callback defaults to CMD_POP. ctx is passed to
 * whichever is called.
 */
Screen *confirm_screen(const char *message, Cmd_t (*on_yes)(void *ctx),
                        Cmd_t (*on_no)(void *ctx), void *ctx);

#endif /* CONFIRM_H */
