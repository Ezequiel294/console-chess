## 1. Save format

- [ ] 1.1 Rewrite `save.c/h` around a text format: a FEN line followed by a coordinate-notation move list
- [ ] 1.2 `save_write()` — emit the starting position and every move played
- [ ] 1.3 `save_read()` — parse the position, then replay each move through the legal move generator
- [ ] 1.4 Reject a file whose position is malformed, whose move text does not parse, or that contains a move illegal in the position it would be played from
- [ ] 1.5 Distinguish "not a save file" from "illegal move at move N" in the message, so a generator bug is not misreported as file corruption
- [ ] 1.6 Reject files from earlier versions with a message saying so
- [ ] 1.7 Never leave a partially loaded game on rejection, and never delete the rejected file
- [ ] 1.8 Round-trip test: several games including castling, en passant, promotion, and a checkmate
- [ ] 1.9 Verify a file produced on one build loads on another built with different settings

## 2. Autosave and resume

- [ ] 2.1 Save after every completed move, with no prompt
- [ ] 2.2 Write to a temporary file and rename into place, so an interruption cannot corrupt the existing save
- [ ] 2.3 Save after an undo or redo as well as after a move
- [ ] 2.4 Detect an existing autosave at startup and offer to resume
- [ ] 2.5 Declining to resume must not delete the save until a new game is actually started
- [ ] 2.6 Report a write failure without interrupting play
- [ ] 2.7 Remove the every-five-moves prompt and the `exit(0)` on save
- [ ] 2.8 Test: kill the process mid-game and confirm every completed move is recoverable

## 3. Named slots

- [ ] 3.1 Save to a named slot without ending the game
- [ ] 3.2 Load from a slot
- [ ] 3.3 `SaveSlot` overlay for choosing one
- [ ] 3.4 Confirm before overwriting an occupied slot

## 4. Undo and redo

- [ ] 4.1 Treat the move list as the undo stack; undo pops and unmakes
- [ ] 4.2 Redo re-applies; a new move truncates the redo tail
- [ ] 4.3 Update board, captured pieces, history, check indicator, and last-move marking together
- [ ] 4.4 Undoing the final move of a finished game returns it to in progress
- [ ] 4.5 Show undo and redo as unavailable when there is nothing to undo or redo
- [ ] 4.6 Test undo of a capture, castling, en passant, and a promotion, asserting full state restoration each time
- [ ] 4.7 Test undo back to the starting position and forward again

## 5. Main menu and result

These close the navigation loop; build them before the other screens.

- [ ] 5.1 `MainMenu` screen: new game, resume (only when a save exists), load slot, how to play, settings, quit
- [ ] 5.2 Change the initial screen from Game to MainMenu
- [ ] 5.3 Return to the menu when a game is left without quitting the program
- [ ] 5.4 `GameOver` screen: result, reason, final position, and options to start a new game, review history, or return to the menu
- [ ] 5.5 Accept no further moves once a game has ended
- [ ] 5.6 `ConfirmQuit` overlay, required whenever a game is in progress

## 6. Status bar and commands

- [ ] 6.1 Persistent status bar showing side to move, game state, and available commands
- [ ] 6.2 Bind `u` undo, `s` save to slot, history (**not `h`** — it is a file name; choose a shifted key or one from `i`-`z`), `F` flip, `?` help, `q` quit. See the key-binding note in `proposal.md`.
- [ ] 6.3 Show unavailable commands dimmed rather than omitting them
- [ ] 6.4 Announce check in the status bar as well as on the board
- [ ] 6.5 Allow position-neutral commands on either side's turn
- [ ] 6.6 Verify no prompt ever appears that the player did not trigger

## 7. History screen

- [ ] 7.1 `History` screen listing moves in SAN, numbered and paired by turn
- [ ] 7.2 Handle White having moved with Black yet to reply
- [ ] 7.3 Scrolling by arrow keys, page keys, home and end
- [ ] 7.4 **Consume wheel events as scrolling** — the same events discarded on the game screen
- [ ] 7.5 Clamp at both ends
- [ ] 7.6 Open scrolled to the most recent moves
- [ ] 7.7 Relayout on resize while keeping a sensible scroll position
- [ ] 7.8 Show an explicit message when no moves have been played
- [ ] 7.9 Reflect undone moves as not played
- [ ] 7.10 Reachable from both the game screen and the result screen

## 8. Help and settings

- [ ] 8.1 `Help` screen covering piece movement, command keys, and mouse usage
- [ ] 8.2 Reachable from the main menu and as an overlay during play
- [ ] 8.3 `Settings` as a single flat overlay: glyph set, auto-flip, colour scheme
- [ ] 8.4 Apply changes immediately
- [ ] 8.5 Persist settings to a text file beside the save
- [ ] 8.6 On a write failure, apply for the session and say it will not persist

## 9. Turn handover

- [ ] 9.1 Replace any remaining timed pause with a readiness prompt before the board flips
- [ ] 9.2 Skip the handover entirely when auto-flip is off
- [ ] 9.3 Keep the last move visible throughout the handover

## 10. Resignation and draw offers

- [ ] 10.1 Resign, with confirmation, ending the game with the opponent as winner
- [ ] 10.2 Available to either player regardless of whose turn it is
- [ ] 10.3 Offer a draw; opponent accepts or declines
- [ ] 10.4 An offer does not persist past a decline
- [ ] 10.5 Extend the termination reasons with resignation and agreed draw
- [ ] 10.6 Distinguish player-chosen terminations from rules-forced ones on the result screen

## 11. Verification

- [ ] 11.1 Play a full game using only commands, confirming no unsolicited prompt appears
- [ ] 11.2 Quit and resume at several points; confirm the position and history are exact each time
- [ ] 11.3 Hand-corrupt a save file and confirm rejection with a useful message and no crash
- [ ] 11.4 Navigate every screen and overlay, confirming each returns to where it was entered from
- [ ] 11.5 Resize while each screen is open
- [ ] 11.6 Run under a leak checker across a session covering every screen
- [ ] 11.7 Update `README.md`: commands, save format and location, resume behaviour, settings
