## Purpose

The game as played across the connection: each player on their own machine seeing their own side, moves carried as messages and verified by both ends, draw offers and resignations that travel, and a disconnection that pauses the match instead of destroying it.

## ADDED Requirements

### Requirement: Each player sees the board from their own side

In a LAN game the board SHALL always face the local player and SHALL never flip. There SHALL be no turn handover: an opponent's move SHALL appear on the board when it arrives, and the local player may answer it whenever they are ready, with no readiness gesture in between.

The handover exists in pass-and-play because two players share one screen and the board must turn between them. Neither is true here.

#### Scenario: Orientation is fixed
- **WHEN** any number of moves are played in a LAN game
- **THEN** the local player's pieces stay at the bottom of their own screen throughout

#### Scenario: A move arrives
- **WHEN** the opponent completes a move
- **THEN** it appears on the local board without the local player pressing anything

#### Scenario: No readiness gesture
- **WHEN** it becomes the local player's turn
- **THEN** they can move immediately; nothing asks them to signal that they are ready

### Requirement: Both machines verify every move

A move SHALL be accepted onto the board only after the receiving machine has verified it against its own rules — the same legality it applies to its own player. After every move, the two machines SHALL confirm they hold the same position; the moment they do not, the match SHALL stop with an error saying the games have diverged, rather than continue as two different games that look alike.

An illegal move arriving over the connection is a defect, not a play: it SHALL be treated as divergence, not shown on the board.

#### Scenario: A legal move
- **WHEN** a legal move arrives
- **THEN** it is verified locally and played on the board exactly as if made at this keyboard

#### Scenario: Divergence is caught at once
- **WHEN** after some move the two machines no longer hold the same position
- **THEN** both stop the match at that move with an error saying so — not twenty moves later, and not silently

#### Scenario: An illegal move arrives
- **WHEN** a move arrives that is not legal in the current position
- **THEN** it is not played, and the match stops with the same divergence error

### Requirement: A received draw offer opens a modal

A draw offer SHALL travel to the opponent and open as a modal choice on their screen — always, whoever's turn it is — with the decline option pre-highlighted. The offer SHALL be answered from that modal: accepting ends the game as an agreed draw on both machines; declining removes the modal and play continues untouched.

This replaces the pass-and-play convention that playing a move declines: behind a modal there is no move to play, so the answer is always explicit. The offering player SHALL see that the offer is pending until it is answered.

#### Scenario: Offer arrives
- **WHEN** a player offers a draw
- **THEN** a modal opens on the opponent's screen naming the offer, with decline highlighted

#### Scenario: Accepted
- **WHEN** the opponent accepts from the modal
- **THEN** the game ends as an agreed draw on both machines, on the standard result screen

#### Scenario: Declined
- **WHEN** the opponent declines from the modal
- **THEN** both players are back in the game exactly as it was, and the offerer can see the offer was declined

### Requirement: Resignation travels

Resigning SHALL work as it does in pass-and-play — confirmed first, naming who it hands the win to — and SHALL end the game on both machines at once, each showing the standard result screen.

#### Scenario: One side resigns
- **WHEN** a player confirms a resignation
- **THEN** both machines end the game as a win for the opponent, with resignation as the reason

### Requirement: Disconnection pauses the match

When the connection is lost, the remaining player SHALL be told within a few seconds by a modal saying the opponent has disconnected and the game is waiting — not by silence, and not by an error that ends the game. From that modal the player may keep waiting or leave; leaving keeps the game unfinished and rejoinable (see lan-resume). If the connection returns while they wait, play SHALL resume where it stopped and the modal SHALL close.

No moves SHALL be playable while disconnected, and in a timed game neither clock SHALL run (see chess-clock). A game SHALL never be lost, won, or drawn *because* the connection dropped.

#### Scenario: The opponent drops
- **WHEN** the connection is lost mid-game
- **THEN** within a few seconds the remaining player sees a modal saying the opponent disconnected and the game is waiting

#### Scenario: The opponent returns
- **WHEN** the opponent reconnects while the modal is up
- **THEN** the modal closes and play resumes exactly where it stopped

#### Scenario: Leaving while disconnected
- **WHEN** the waiting player chooses to leave
- **THEN** they return to the menu; the game is unfinished, and both players can later rejoin it by its ID

#### Scenario: A drop decides nothing
- **WHEN** a connection drops, however long the wait
- **THEN** no result is recorded for either side by the drop itself

### Requirement: The LAN command set has no save and no handover

During a live LAN game the available commands SHALL be: view history, resign, offer a draw, help, and quit. There SHALL be no save command — the game is preserved automatically for rejoining (see lan-resume), and a manual mid-game save on one of two machines would describe a match that does not belong to either alone.

Quitting a live LAN game SHALL ask for confirmation that says the game stays open and can be rejoined by its ID — not the pass-and-play save-and-quit choice, which asks a question that no longer applies.

#### Scenario: No save command
- **WHEN** a LAN game is on the board
- **THEN** save is not among the commands offered, and the save key does nothing

#### Scenario: Quitting a LAN game
- **WHEN** the player quits during a live LAN game
- **THEN** they are asked to confirm leaving, told the game remains rejoinable under its ID, and no save question is asked
