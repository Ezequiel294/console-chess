## MODIFIED Requirements

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
- **WHEN** a replay is stepped back and then reopened later
- **THEN** it opens at the end of the game again, because nothing about the stepping was recorded
