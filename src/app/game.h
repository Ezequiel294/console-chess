#ifndef GAME_H
#define GAME_H

#include "types.h"

/* Turn flow: whose move it is, asking for it, applying it, deciding when the
 * game is over. Becomes the Game screen in the terminal-ui-foundation change.
 */

void game_loop(GameState *p_state);
void get_move(GameState *p_state, int *captured_king);

#endif /* GAME_H */
