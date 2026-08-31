## MODIFIED Requirements

### Requirement: Main menu

The system SHALL open on a main menu offering: a new game, loading a saved game, how to play, settings, and quitting.

Choosing a new game SHALL open the new-game setup screen rather than a board — a game now has settings, and there is one screen that asks for them (see new-game-setup).

There is deliberately no "resume last game": see game-persistence's Status note — an automatic resume risked silently reopening a *finished* game as if it were still in progress. Every saved game, including one closed mid-play, is reached the same way: Load Game.

#### Scenario: Launching
- **WHEN** the program starts
- **THEN** the main menu is shown

#### Scenario: Returning to the menu
- **WHEN** the player leaves a game without quitting the program
- **THEN** the main menu is shown again

#### Scenario: Starting a new game
- **WHEN** the player chooses New Game
- **THEN** the setup screen is shown, and a board appears only once the settings are confirmed

### Requirement: Turn handover

After every completed move, the system SHALL wait for the incoming player to indicate they are ready before the board flips, rather than changing after a fixed delay. This handover is unconditional — there is no setting to skip it, timed or untimed — because it is two things at once: the moment a player gets to look at the move that was just made, and the press of the clock (see chess-clock's The handover is the clock press).

In a timed game the handover SHALL be the only thing that stops the mover's clock and starts the opponent's. In an untimed game it SHALL do exactly what it does today.

A move that ends the game SHALL NOT ask for a handover.

#### Scenario: Handover
- **WHEN** a move completes
- **THEN** the completed move stays visible until the next player signals readiness
- **AND** the board then flips

#### Scenario: No timed wait
- **WHEN** a player takes any amount of time between turns
- **THEN** nothing changes on screen until they act, except a running clock counting down

#### Scenario: The handover is the clock press
- **WHEN** a move completes in a timed game and the incoming player signals readiness
- **THEN** the mover's clock stops and takes its increment, and the incoming player's clock starts

#### Scenario: Handover after a game-ending move
- **WHEN** a move ends the game
- **THEN** no handover is asked for and the result is reached directly

### Requirement: Available commands are visible

The system SHALL display the currently available commands on screen during play, and SHALL show whose turn it is and the state of the game.

The state of the game SHALL be able to change with no input at all: in a timed game a clock running out ends the game while the player is looking at it, and the screen SHALL say so at that moment rather than at the next keystroke.

#### Scenario: Commands shown
- **WHEN** the board is displayed
- **THEN** the available command keys and their meanings are visible

#### Scenario: Turn shown
- **WHEN** the board is displayed
- **THEN** the side to move is stated

#### Scenario: Check announced
- **WHEN** the side to move is in check
- **THEN** this is stated in addition to being marked on the board

#### Scenario: Unavailable command
- **WHEN** a command cannot currently be used
- **THEN** it is shown as unavailable rather than silently doing nothing

#### Scenario: The game ends without a keystroke
- **WHEN** a clock reaches zero while nobody is pressing anything
- **THEN** the screen states that the game is over and stops offering a move, without waiting for input

### Requirement: Game result

When a game ends, the system SHALL display the result, the reason, and the final position, and SHALL offer saving the finished game, a new game, reviewing the move history, and returning to the main menu.

Choosing a new game from here SHALL open the setup screen, the same one the main menu opens, so that the settings of the next game are chosen rather than inherited.

Saving is offered here because this is the only screen a finished game is ever seen from, and leaving it without saving discards the game for good. It is offered rather than performed: keeping a game is the player's decision, the same as it is during play (see game-persistence's Saving is explicit).

#### Scenario: Result shown
- **WHEN** a game ends by any means
- **THEN** the winner or draw, the reason, and the final position are displayed

#### Scenario: Saving the finished game
- **WHEN** the player chooses to save from the result screen
- **THEN** the finished game is written to disk — asking for a name if it has never been saved — and the result screen remains, reporting what happened

#### Scenario: Saving a game that was already saved
- **WHEN** the player saves a game from the result screen that had been saved while in progress
- **THEN** it is not asked to be named again, and the finished save replaces the in-progress one

#### Scenario: Leaving without saving
- **WHEN** the player leaves the result screen without saving
- **THEN** nothing is written, and no prompt asks them to reconsider

#### Scenario: Board orientation is continuous
- **WHEN** a game ends
- **THEN** the final position is shown in the same orientation the player was already looking at, not one recomputed from whose turn it is — nothing about ending the game looks like the board flipped on its own

#### Scenario: Checkmate marks the losing king
- **WHEN** a game ends in checkmate
- **THEN** the checked king's square is marked the same way it is during play, so the result screen answers "which king" as well as "who won"

#### Scenario: Review from the result
- **WHEN** the player chooses to review from the result screen
- **THEN** the full move history is available

#### Scenario: No further moves
- **WHEN** a game has ended
- **THEN** no move can be made in it

#### Scenario: A new game from the result
- **WHEN** the player chooses a new game from the result screen
- **THEN** the setup screen is shown, with its settings to choose again, rather than a board
