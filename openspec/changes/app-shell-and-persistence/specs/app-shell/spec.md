## Purpose

Gives the game somewhere to put everything that is not the board — a menu to start from, help to read, settings to change, and a result to end on — and replaces mid-game prompting with commands the player issues when they want them.

## ADDED Requirements

### Requirement: Main menu

The system SHALL open on a main menu offering: a new game, resuming a saved game when one exists, loading a named slot, how to play, settings, and quitting.

#### Scenario: Launching
- **WHEN** the program starts
- **THEN** the main menu is shown

#### Scenario: Resume offered conditionally
- **WHEN** a saved game exists
- **THEN** resuming appears in the menu; when none exists, it does not

#### Scenario: Returning to the menu
- **WHEN** the player leaves a game without quitting the program
- **THEN** the main menu is shown again

### Requirement: Commands replace prompting

In-game actions SHALL be invoked by the player through single-key commands. The system SHALL NOT interrupt play to ask questions the player did not initiate.

The available commands SHALL be: undo, save to a slot, view history, flip the board, help, and quit.

#### Scenario: Command issued
- **WHEN** the player presses a command key during their turn
- **THEN** that action is taken without disturbing the position

#### Scenario: No unsolicited prompts
- **WHEN** a game runs for any number of moves
- **THEN** no prompt appears that the player did not trigger

#### Scenario: Command during opponent's turn
- **WHEN** a command that does not alter the position is issued at any point in a turn
- **THEN** it works, and the side to move is unchanged afterwards

### Requirement: Available commands are visible

The system SHALL display the currently available commands on screen during play, and SHALL show whose turn it is and the state of the game.

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
- **WHEN** a command cannot currently be used, such as undo with no moves played
- **THEN** it is shown as unavailable rather than silently doing nothing

### Requirement: Help

The system SHALL provide help covering how to move pieces, the command keys, and mouse usage, reachable both from the main menu and during play.

#### Scenario: Help during play
- **WHEN** the player requests help mid-game
- **THEN** it is shown over the board, and dismissing it returns to the game unchanged

#### Scenario: Help from the menu
- **WHEN** the player requests help from the main menu
- **THEN** the same content is available before any game starts

### Requirement: Settings

The system SHALL let the player change the piece glyph set, whether the board flips between turns, and the colour scheme. Changes SHALL take effect immediately and SHALL persist across sessions.

#### Scenario: Glyph set changed
- **WHEN** the player switches to the ASCII glyph set
- **THEN** the board redraws with letters immediately

#### Scenario: Auto-flip disabled
- **WHEN** the player turns off automatic flipping
- **THEN** the board keeps one orientation between turns, and manual flipping still works

#### Scenario: Settings persist
- **WHEN** the player changes a setting and restarts the program
- **THEN** the setting is still in effect

#### Scenario: Settings unavailable
- **WHEN** settings cannot be written to disk
- **THEN** the change still applies for the session, and the player is told it will not persist

### Requirement: Turn handover

When the board changes orientation between turns, the system SHALL wait for the incoming player to indicate they are ready, rather than changing after a fixed delay.

#### Scenario: Handover
- **WHEN** a move completes and the board is set to flip
- **THEN** the completed move stays visible until the next player signals readiness
- **AND** the board then flips

#### Scenario: No timed wait
- **WHEN** a player takes any amount of time between turns
- **THEN** nothing changes on screen until they act

#### Scenario: Flipping disabled
- **WHEN** automatic flipping is off
- **THEN** play passes directly to the next turn with no handover step

### Requirement: Game result

When a game ends, the system SHALL display the result, the reason, and the final position, and SHALL offer a new game, reviewing the move history, and returning to the main menu.

#### Scenario: Result shown
- **WHEN** a game ends by any means
- **THEN** the winner or draw, the reason, and the final position are displayed

#### Scenario: Review from the result
- **WHEN** the player chooses to review from the result screen
- **THEN** the full move history is available

#### Scenario: No further moves
- **WHEN** a game has ended
- **THEN** no move can be made in it

### Requirement: Quitting is confirmed and lossless

Quitting SHALL require confirmation when a game is in progress, and SHALL NOT lose any completed move.

#### Scenario: Quit mid-game
- **WHEN** the player quits during a game
- **THEN** confirmation is requested

#### Scenario: Quit cancelled
- **WHEN** the player cancels the confirmation
- **THEN** the game resumes exactly as it was

#### Scenario: Quit confirmed
- **WHEN** the player confirms
- **THEN** the program exits, the terminal is restored, and the game is recoverable on next launch
