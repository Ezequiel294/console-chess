# move-undo Specification

## Purpose

Lets a finished game's position be walked back and forth — undo and redo applied to a copy of a game already over, not to a game in progress. Chess does not actually allow taking back a move you have already made, so live play never offers this; a replay is review, not a take-back, and stepping through one is exactly this mechanism doing its job.

core/history.c's `history_pop_last`, `history_push_node`, `captures_pop_last`, and `hash_history_pop_last`, together with core/replay.c's `replay_step_back` and `replay_step_forward`, implement it; `GameState.p_redo_head` carries the redo stack. It is reachable only through game-replay's stepping commands — never as a live-play command, since chess grants no take-backs mid-game.

## Requirements

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

In a replay no new move can be made (see game-replay), so nothing there can discard the redo stack: every move of the game stays reachable in both directions for as long as the replay is open.

#### Scenario: Redo
- **WHEN** the player undoes a move and then redoes it
- **THEN** the position is identical to before the undo

#### Scenario: New move discards redo
- **WHEN** the player undoes a move and then plays a different one
- **THEN** the previously undone move can no longer be redone

#### Scenario: Nothing to redo
- **WHEN** no move has been undone
- **THEN** redo is unavailable and is shown as such

#### Scenario: Nothing discards redo in a replay
- **WHEN** the player steps backward through a replay by any number of moves
- **THEN** every one of them remains available to step forward again, since no new move can be played to discard them

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

Undo is reachable only while reviewing a finished game, which is read from its file and never written back. A replay SHALL NOT modify the saved game it was opened from, no matter how far back it is stepped: the file records the game that was played, not the position a reviewer happened to stop at.

This replaces the earlier "dormant, nothing to reconcile" position. The conflict that requirement guarded against — a save disagreeing with an undo — cannot arise, because the one place undo now exists is the one place nothing is saved.

#### Scenario: Nothing to reconcile today
- **WHEN** a game is in progress
- **THEN** no undo command exists to reach it, so there is no saved position this requirement can contradict

#### Scenario: If undo returns
- **WHEN** a later change makes undo reachable during a live game
- **THEN** the saved game SHALL reflect the undo, and reloading SHALL restore the position left behind rather than the undone one — a rule that binds live play only, since a replay writes nothing

#### Scenario: A replay does not write
- **WHEN** the player steps backward through a replay and leaves it
- **THEN** the saved game is byte-for-byte what it was before the replay opened

#### Scenario: Reopening after stepping
- **WHEN** a replay is stepped forward and then reopened later
- **THEN** it opens at the start of the game again, because nothing about the stepping was recorded
