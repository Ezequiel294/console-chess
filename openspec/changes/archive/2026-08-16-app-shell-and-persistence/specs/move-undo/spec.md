## Purpose

Lets players take back a move — the ordinary courtesy of a game played across a table — and redo it if the take-back was itself a mistake.

**Status: deferred from live play.** Chess does not actually allow taking back a move you have already made, and undo/redo as a command during an ongoing game turned out to encourage exactly that — a rules violation dressed up as a courtesy. The requirements below describe the mechanism (the move list as an undo stack, full state restoration, redo discarded by a new move) and remain correct at that level; core/history.c's `history_pop_last`, `history_push_node`, `captures_pop_last`, and `hash_history_pop_last` implement it and `GameState.p_redo_head` carries the redo stack. None of it is currently wired to a command a player can reach mid-game. It is reserved for a future "replay a finished game" capability, which would apply the same mechanism to a copy of a finished game's state rather than the game in progress.

## ADDED Requirements

### Requirement: Moves can be undone

The player SHALL be able to undo the most recent move, restoring the position in full: piece placement, side to move, castling rights, en passant availability, and the draw clocks.

Undo SHALL be repeatable back to the start of the game.

#### Scenario: Undo a move
- **WHEN** the player undoes the most recent move
- **THEN** the position returns to what it was before that move, and it is that side's turn again

#### Scenario: Undo a capture
- **WHEN** a capturing move is undone
- **THEN** the captured piece returns to the board and leaves the captured list

#### Scenario: Undo castling
- **WHEN** a castling move is undone
- **THEN** both king and rook return to their squares and the castling right is restored

#### Scenario: Undo en passant
- **WHEN** an en passant capture is undone
- **THEN** the captured pawn returns to its own square, not to the capturing pawn's destination

#### Scenario: Undo a promotion
- **WHEN** a promotion is undone
- **THEN** a pawn stands on the origin square, not the promoted piece

#### Scenario: Repeated undo
- **WHEN** the player undoes repeatedly
- **THEN** the game unwinds move by move to the starting position

#### Scenario: Nothing to undo
- **WHEN** no move has been played
- **THEN** undo is unavailable and is shown as such

### Requirement: Undone moves can be redone

The player SHALL be able to redo an undone move. Making a new move SHALL discard anything available to redo.

#### Scenario: Redo
- **WHEN** the player undoes a move and then redoes it
- **THEN** the position is identical to before the undo

#### Scenario: New move discards redo
- **WHEN** the player undoes a move and then plays a different one
- **THEN** the previously undone move can no longer be redone

#### Scenario: Nothing to redo
- **WHEN** no move has been undone
- **THEN** redo is unavailable and is shown as such

### Requirement: Undo is reflected everywhere

An undo SHALL update every view of the game consistently: the board, the captured pieces, the move history, the check indicator, and the last-move marking.

#### Scenario: Views agree
- **WHEN** a move is undone
- **THEN** the board, captured pieces, and history all reflect the earlier state
- **AND** the last-move marking shows the move before the undone one, or nothing if there is none

#### Scenario: Undoing out of check
- **WHEN** a move that answered a check is undone
- **THEN** the check indicator is shown again

#### Scenario: Undoing into a finished game
- **WHEN** a game has ended and the final move is undone
- **THEN** the game is in progress again and moves are accepted

### Requirement: Undo interacts correctly with saving

Dormant, not withdrawn: undo is not reachable during a live game and there is no autosave for it to disagree with (see Status above, and app-shell's Saving is explicit). Should a future change expose undo mid-game, an undo SHALL be reflected in whatever save represents the game, so reloading lands on the position the player actually left rather than one they had already taken back.

#### Scenario: Nothing to reconcile today
- **WHEN** a game is in progress
- **THEN** no undo command exists to reach it, so there is no saved position this requirement can contradict

#### Scenario: If undo returns
- **WHEN** a later change makes undo reachable during play
- **THEN** the saved game SHALL reflect the undo, and reloading SHALL restore the position left behind rather than the undone one
