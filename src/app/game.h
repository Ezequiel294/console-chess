#ifndef GAME_H
#define GAME_H

#include "app/app.h"
#include "types.h"

/* The game as a screen.
 *
 * Turn flow lives in handle, the board and its panels in render. Neither
 * prints, neither prompts, and neither knows how large the terminal is: the
 * region arrives as a parameter every frame.
 *
 * The rules are untouched by this — the game is exactly as complete as it was,
 * drawn properly. A bug appearing here is a rendering or input bug, never a
 * rules bug.
 */

/* The screen borrows state; it does not own it and does not free it. */
Screen *game_screen(GameState *state);

/* The same screen, driven by stepping instead of play: no move can be made,
 * the arrows and u/r step through the game's own moves, f flips the board,
 * and q leaves directly with no save prompt. state's result is read as-is —
 * the caller is responsible for only opening a replay on a finished game. */
Screen *replay_screen(GameState *state);

#endif /* GAME_H */
