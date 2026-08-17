# app-shell Specification

## Purpose

Gives the game somewhere to put everything that is not the board — a menu to start from, help to read, settings to change, and a result to end on — and replaces mid-game prompting with commands the player issues when they want them.

## Requirements

### Requirement: Main menu

The system SHALL open on a main menu offering: a new game, loading a saved game, how to play, settings, and quitting.

There is deliberately no "resume last game": see game-persistence's Status note — an automatic resume risked silently reopening a *finished* game as if it were still in progress. Every saved game, including one closed mid-play, is reached the same way: Load Game.

#### Scenario: Launching
- **WHEN** the program starts
- **THEN** the main menu is shown

#### Scenario: Returning to the menu
- **WHEN** the player leaves a game without quitting the program
- **THEN** the main menu is shown again

### Requirement: Menu navigation is consistent

Every screen that offers a choice SHALL be navigable the same way: the arrow keys move a highlight, Enter chooses the highlighted option, and clicking an option with the mouse only highlights it, never chooses it outright. These screens SHALL NOT offer letter-key shortcuts for their options.

This covers every one of them, with no exceptions — the main menu, the result screen, Settings, the saved-game list, and the in-game overlays alike: the resignation confirmation, the quit choice, a draw-offer response, and the promotion choice. The overlays were previously exempt, on the grounds that in-game interaction should be fast. That was the wrong trade twice over. The letters made each overlay a small vocabulary to learn on the spot (`s`/`q` here, `y`/`n` there, `1`-`4` somewhere else) rather than one gesture that works everywhere; and these are the most consequential choices in the program — resigning, ending the game in a draw, quitting without saving — so a single click acting immediately is exactly the accident worth preventing. Speed is still available: the safe option is where the highlight starts (see Irreversible choices open on the safe option) and the other is one arrow key away.

In-game *commands* (save, history, resign, offer a draw, help, quit) are a different thing and remain single-key: they open a screen, they do not decide anything.

#### Scenario: Keyboard navigation
- **WHEN** the player presses an arrow key on a screen offering a choice
- **THEN** the highlighted option moves, and no option is chosen until Enter is pressed

#### Scenario: A click only selects
- **WHEN** the player clicks an option on a screen offering a choice
- **THEN** that option becomes highlighted, and nothing else happens until Enter is pressed

#### Scenario: No letter shortcuts for options
- **WHEN** the player presses a letter key on a screen offering a choice
- **THEN** nothing happens, since options are reached only by arrow keys, Enter, and clicking

#### Scenario: Both directions of travel
- **WHEN** the player presses the up arrow on a screen offering a choice
- **THEN** the highlight moves to the previous option, wrapping to the last — never in the same direction as the down arrow

### Requirement: Irreversible choices open on the safe option

A screen whose options include one that ends the game or discards work SHALL open with the highlight on the option that does nothing — "no", or "cancel" — not on the destructive one.

Once a click only moves the highlight and Enter is the only thing that acts, the opening highlight is the last remaining way to end a game by accident: one reflexive Enter after the command key. Placing it on the safe option costs the deliberate player a single arrow key and costs the distracted one nothing at all. Screens with no destructive option (Settings, the promotion choice, the main menu) are unaffected and open on their first entry.

#### Scenario: Confirmation opens on "no"
- **WHEN** a confirmation for resigning or agreeing a draw is shown
- **THEN** "no" is highlighted, and pressing Enter without moving leaves the game unchanged

#### Scenario: Quitting opens on "cancel"
- **WHEN** the quit choice is shown, with or without a game worth saving
- **THEN** "cancel" is highlighted, and pressing Enter without moving returns to the game

#### Scenario: The destructive option stays reachable
- **WHEN** the player means to resign, agree a draw, or quit
- **THEN** one arrow key reaches the option and Enter takes it

### Requirement: One hint line

Each screen SHALL state how to drive it on the screen's last row, and SHALL NOT repeat that text anywhere else. An overlay's hint SHALL replace the hint of the screen beneath it for as long as the overlay is up, so what is on that row always describes what currently has the keyboard.

A hint SHALL name the keys that matter rather than every key that works: a line listing each of arrows, page keys, Home and End is one nobody reads, and the unnamed keys still function.

#### Scenario: Hint is not duplicated
- **WHEN** an overlay with its own hint is open
- **THEN** the hint appears once, on the last row, and not also inside the overlay's box

#### Scenario: Hint follows the keyboard
- **WHEN** an overlay opens over a screen that has its own hint
- **THEN** the last row describes the overlay, and returns to describing the screen beneath once the overlay closes

### Requirement: Commands replace prompting

In-game actions SHALL be invoked by the player through single-key commands. The system SHALL NOT interrupt play to ask questions the player did not initiate.

The available commands SHALL be: save, view history, resign, offer a draw, help, and quit.

There is deliberately no command to flip the board. The turn handover already turns it, and a manual flip only lets the side to move study the position from their opponent's seat — which is not something either player is entitled to mid-game. See Turn handover.

Undo and redo are deliberately not among them: chess does not allow taking back a move already made. They remain a reusable capability (see move-undo) for a future mode that replays a finished game, not a live-play command.

#### Scenario: Command issued
- **WHEN** the player presses a command key during their turn
- **THEN** that action is taken without disturbing the position

#### Scenario: No unsolicited prompts
- **WHEN** a game runs for any number of moves
- **THEN** no prompt appears that the player did not trigger

#### Scenario: Command during opponent's turn
- **WHEN** a command that does not alter the position is issued at any point in a turn
- **THEN** it works, and the side to move is unchanged afterwards

#### Scenario: No manual flip
- **WHEN** the player looks for a way to turn the board mid-turn
- **THEN** there is none; the board's orientation changes only at the handover

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
- **WHEN** a command cannot currently be used
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

The system SHALL let the player change the piece glyph set and the colour scheme. Changes SHALL take effect immediately and SHALL persist across sessions.

There is deliberately no setting for the turn handover: it always happens, the same way, every turn — see Turn handover.

#### Scenario: Glyph set changed
- **WHEN** the player switches to the ASCII glyph set
- **THEN** the board redraws with letters immediately

#### Scenario: Settings persist
- **WHEN** the player changes a setting and restarts the program
- **THEN** the setting is still in effect

#### Scenario: Settings unavailable
- **WHEN** settings cannot be written to disk
- **THEN** the change still applies for the session, and the player is told it will not persist

### Requirement: Turn handover

After every completed move, the system SHALL wait for the incoming player to indicate they are ready before the board flips, rather than changing after a fixed delay. This handover is unconditional — there is no setting to skip it — since the same "I'm ready" gesture is intended to double as the cue a future timed mode needs to start the incoming player's clock.

#### Scenario: Handover
- **WHEN** a move completes
- **THEN** the completed move stays visible until the next player signals readiness
- **AND** the board then flips

#### Scenario: No timed wait
- **WHEN** a player takes any amount of time between turns
- **THEN** nothing changes on screen until they act

### Requirement: Game result

When a game ends, the system SHALL display the result, the reason, and the final position, and SHALL offer a new game, reviewing the move history, and returning to the main menu.

#### Scenario: Result shown
- **WHEN** a game ends by any means
- **THEN** the winner or draw, the reason, and the final position are displayed

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

### Requirement: Quitting offers to save

Quitting SHALL require a choice when a game is in progress: save and quit, quit without saving, or cancel. Since there is no automatic save, this is the moment losing an in-progress game becomes a deliberate choice rather than an accident.

#### Scenario: Quit mid-game
- **WHEN** the player quits during a game
- **THEN** they are asked whether to save and quit, quit without saving, or cancel

#### Scenario: Quit cancelled
- **WHEN** the player cancels
- **THEN** the game continues exactly as it was

#### Scenario: Quit with save
- **WHEN** the player chooses to save and quit
- **THEN** the game is saved (per game-persistence's Named saves), then the program exits and the terminal is restored

#### Scenario: Quit without saving
- **WHEN** the player chooses to quit without saving
- **THEN** the program exits and the terminal is restored, and any progress since the last save is lost

#### Scenario: Nothing to save yet
- **WHEN** the player quits before any move has been played
- **THEN** there is nothing worth saving, and the choice is simply to quit or cancel
