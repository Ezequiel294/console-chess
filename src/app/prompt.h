#ifndef PROMPT_H
#define PROMPT_H

#include "app/app.h"

#include <stddef.h>

/* A titled text-input overlay: a message line, a field, Enter submits, Escape
 * cancels. Generic — it knows nothing about saving or naming; the caller
 * supplies the initial value, an optional validator, and what happens on
 * submit or cancel, the same contract confirm_screen uses for its yes/no
 * answers. Composites over whatever is beneath it.
 *
 * It accepts letter keys, and that is not a violation of app-shell's "no
 * letter shortcuts" rule — that rule is about screens offering a choice,
 * where a letter would shortcut an option. This screen has no options;
 * typing is what it is for.
 *
 * Only printable codepoints are accepted, up to max_codepoints of them;
 * further characters are not accepted rather than silently dropped later.
 * Backspace removes one whole codepoint, never a partial one. There is no
 * cursor movement, no selection, no history — text input is where terminal
 * handling gets fiddly, so this is kept deliberately small.
 *
 * validate is called on submit, not on every keystroke (the length cap is
 * the one thing enforced as the player types). Pass NULL to accept anything
 * that fits. On a rejected value, err is shown as the prompt's message line
 * and the prompt stays open with what the player typed untouched.
 *
 * on_submit is called with the field's contents exactly as typed — not
 * trimmed, not checked for emptiness, only known to have passed validate —
 * and returns the Cmd_t this screen itself returns. on_cancel is called on
 * Escape and defaults to CMD_POP when NULL. ctx is passed to whichever is
 * called.
 */
Screen *prompt_screen(const char *title, const char *initial_value, int max_codepoints,
                       int (*validate)(const char *text, char *err, size_t err_len),
                       Cmd_t (*on_submit)(void *ctx, const char *text),
                       Cmd_t (*on_cancel)(void *ctx), void *ctx);

#endif /* PROMPT_H */
