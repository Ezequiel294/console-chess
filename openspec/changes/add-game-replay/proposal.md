## Why

A finished game currently vanishes. The result screen offers to review the move *list*, but there is no way to walk the *position* back and forth. Meanwhile `move-undo` has been sitting dormant since it was pulled out of live play, explicitly reserved for "a future mode that replays a finished game". This is that mode.

## Depends on

`add-named-saves`, which lands first. Everything about how a game gets onto disk — that a finished game can be saved at all, that its file says `finished`, and that the file records how the game ended — belongs to that change. This one assumes those files exist and does nothing to `game-persistence`.

## What Changes

- **New replay mode.** Choosing a finished game from the Load Game list opens it on the game screen in replay mode: the same board, the same panels, driven differently.
  - Commands are flip (`F`), undo (`u`), redo (`r`), history (`H`), help (`?`), quit (`q`). Save, resign and offer-draw are gone — there is nothing left to save, resign, or agree.
  - Left and right arrows step backward and forward through the played moves, the same two operations as `u` and `r`.
  - No move can be made. The only positions reachable are the ones the game actually passed through.
  - The board's orientation is fixed until `F` turns it: there is no turn handover, no SPACE step, and no automatic flip between moves. A reviewer is one person looking at one board, not two players handing it over.
  - Stepping to the last move shows the result in the status line and stops there. The result *screen* is deliberately not shown — it would end the replay and make stepping back out of the final position clunky.
  - History (`H`) lists the whole game, not only the moves stepped through, with the move currently on the board marked. In a replay the moves ahead are not a spoiler to be withheld; they are the thing being reviewed.
- Ongoing saves keep opening in live play. Replay is for finished games only.
- **Undo and redo come out of dormancy.** The mechanism in `core/history.c` and `unmake` has been specified and implemented since it was pulled from live play, and has never run. It is what stepping is built on.

## Capabilities

### New Capabilities
- `game-replay`: reviewing a finished game on the board — entering replay from the saved-game list, stepping through the played moves in both directions, what is and is not available while doing so, and how the replay ends.

### Modified Capabilities
- `app-shell`: the "no flip command" rule is scoped to live play, since a replay has no turn to hand over, and undo/redo are named as replay commands rather than as nothing at all.
- `move-undo`: lifted from dormant to live — undo and redo are reachable again, in replay mode, applied to a finished game rather than a game in progress.
- `move-history-view`: in a replay the history lists the whole game with the current move marked, rather than truncating at the moves played so far.

## Impact

- `src/app/game.c`: a mode flag on the game screen — command set, hint line, arrow-key meaning, orientation policy, and the guard that refuses to originate a move.
- `src/app/savedgames.c` / `mainmenu.c`: routing a finished game into replay instead of live play.
- `src/app/history_view.c`: whole-game listing with a marked move.
- `src/core/history.c` and `movegen.c`'s `unmake`: the stepping mechanism, exercised for the first time.
- `src/app/help.c`: the replay's keys.
- `tests/`: a new suite for stepping, which has never been run.
