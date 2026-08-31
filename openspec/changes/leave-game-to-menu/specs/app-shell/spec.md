## RENAMED Requirements

- FROM: `### Requirement: Quitting offers to save`
- TO: `### Requirement: Leaving a game offers to save`

## MODIFIED Requirements

### Requirement: Main menu

The system SHALL open on a main menu offering: a new game, loading a saved game, how to play, settings, and quitting. The main menu SHALL be the only place the program is exited from, apart from the terminal's own interrupt — every screen leads back here rather than out.

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

#### Scenario: Leaving a live game
- **WHEN** the player leaves a game that is still in progress
- **THEN** the main menu is shown again, exactly as it is after a finished game — a live game is not the one screen with no way back

#### Scenario: Exiting the program
- **WHEN** the player wants to exit the program rather than the game
- **THEN** that choice is on the main menu, and no screen inside a game offers it

### Requirement: Menu navigation is consistent

Every screen that offers a choice SHALL be navigable the same way: the arrow keys move a highlight, Enter chooses the highlighted option, and clicking an option with the mouse only highlights it, never chooses it outright. These screens SHALL NOT offer letter-key shortcuts for their options.

This covers every one of them, with no exceptions — the main menu, the result screen, Settings, the saved-game list, and the in-game overlays alike: the resignation confirmation, the choice to leave a game, a draw-offer response, and the promotion choice. The overlays were previously exempt, on the grounds that in-game interaction should be fast. That was the wrong trade twice over. The letters made each overlay a small vocabulary to learn on the spot (`s`/`q` here, `y`/`n` there, `1`-`4` somewhere else) rather than one gesture that works everywhere; and these are the most consequential choices in the program — resigning, ending the game in a draw, leaving a game without saving — so a single click acting immediately is exactly the accident worth preventing. Speed is still available: the safe option is where the highlight starts (see Irreversible choices open on the safe option) and the other is one arrow key away.

In-game *commands* (save, history, resign, offer a draw, help, leave) are a different thing and remain single-key: they open a screen, they do not decide anything.

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
- **WHEN** the choice to leave a game is shown, with or without a game worth saving
- **THEN** "cancel" is highlighted, and pressing Enter without moving returns to the game

#### Scenario: The destructive option stays reachable
- **WHEN** the player means to resign, agree a draw, or leave a game
- **THEN** one arrow key reaches the option and Enter takes it

### Requirement: Commands replace prompting

In-game actions SHALL be invoked by the player through single-key commands. The system SHALL NOT interrupt play to ask questions the player did not initiate.

The available commands during a live game SHALL be: save, view history, resign, offer a draw, help, and leave the game.

There is deliberately no command to flip the board *during a live game*. The turn handover already turns it, and a manual flip only lets the side to move study the position from their opponent's seat — which is not something either player is entitled to mid-game. See Turn handover. A replay is the other case entirely: there is no turn, no handover, and no opponent to gain an advantage over, so a replay does have a flip command (see game-replay).

Undo and redo are likewise not live-play commands: chess does not allow taking back a move already made. They are reachable only in a replay of a finished game, where stepping backward changes nothing that was played (see move-undo and game-replay).

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
- **WHEN** the player looks for a way to turn the board mid-turn in a live game
- **THEN** there is none; the board's orientation changes only at the handover

#### Scenario: No undo in a live game
- **WHEN** the player presses the undo or redo key during a live game
- **THEN** nothing happens, and neither is listed among the live game's commands

### Requirement: Leaving a game offers to save

Leaving a game in progress SHALL require a choice: save and leave, leave without saving, or cancel. Since there is no automatic save, this is the moment losing an in-progress game becomes a deliberate choice rather than an accident.

Both choices that leave SHALL return to the main menu rather than exiting the program. The question is about the game — whether to keep it — and the answer says nothing about whether the player is finished with the program. Exiting is a separate decision, made in the one place that offers it (see Main menu).

The state of the abandoned game SHALL NOT survive the choice: leaving without saving discards the game exactly as thoroughly as exiting the program would have, and the main menu that follows offers no way back into it.

#### Scenario: Quit mid-game
- **WHEN** the player leaves during a game
- **THEN** they are asked whether to save and leave, leave without saving, or cancel

#### Scenario: Quit cancelled
- **WHEN** the player cancels
- **THEN** the game continues exactly as it was

#### Scenario: Quit with save
- **WHEN** the player chooses to save and leave
- **THEN** the game is saved (per game-persistence's Named saves), and the main menu is shown

#### Scenario: Quit without saving
- **WHEN** the player chooses to leave without saving
- **THEN** the main menu is shown, and any progress since the last save is lost

#### Scenario: Nothing to save yet
- **WHEN** the player leaves before any move has been played
- **THEN** there is nothing worth saving, and the choice is simply to leave or cancel

#### Scenario: The program is still running
- **WHEN** the player takes either of the choices that leave
- **THEN** the program is still running and the terminal is still in its full-screen display, since nothing about leaving a game is a request to exit

#### Scenario: The abandoned game is gone
- **WHEN** the player has left a game without saving it
- **THEN** the main menu offers no way to return to it, and starting or loading a game begins from nothing that game left behind
