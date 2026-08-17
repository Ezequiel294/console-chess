#ifndef GAMEOVER_H
#define GAMEOVER_H

#include "app/app.h"
#include "core/outcome.h"
#include "types.h"

/* The result screen: winner or draw, the reason, the final position, and the
 * choice of a new game, reviewing the history, or returning to the menu.
 *
 * Replaces the finished game screen on the stack (CMD_REPLACE), rather than
 * sitting on top of it — the finished game has nothing left to do, so leaving
 * it on the stack beneath this one would only be a screen that can never be
 * reached again. state is borrowed the same way game_screen borrows it, and
 * "new game" resets it in place before pushing a fresh game screen.
 *
 * flipped is the orientation the player was already looking at when the game
 * ended, carried over rather than recomputed from side_to_move — the result
 * screen should never look like the board just flipped on its own.
 */
Screen *gameover_screen(GameState *state, Outcome_t outcome, int flipped);

#endif /* GAMEOVER_H */
