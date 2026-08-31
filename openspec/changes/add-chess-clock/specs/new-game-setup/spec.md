## Purpose

Puts a screen between "New Game" and the board where the game is described before it is started, so that settings a game needs — its time control now, its opponent later — are asked for once, in one place, rather than being assumed.

## ADDED Requirements

### Requirement: New Game opens a setup screen

Choosing to start a new game SHALL open a setup screen rather than a board. The game SHALL begin only when the player confirms the settings on it.

This SHALL apply wherever a new game is started from — the main menu and the result screen alike — so that there is exactly one way a game comes into existence and exactly one screen that describes it.

Leaving the setup screen without confirming SHALL return to whatever it was opened from, and SHALL NOT start a game, end one, or discard anything.

#### Scenario: From the main menu
- **WHEN** the player chooses New Game from the main menu
- **THEN** the setup screen is shown and no game has started yet

#### Scenario: From the result screen
- **WHEN** the player chooses a new game from the result screen
- **THEN** the same setup screen is shown, offering the same settings

#### Scenario: Backing out
- **WHEN** the player leaves the setup screen without confirming
- **THEN** they are returned to the screen they came from, with nothing started and nothing changed

#### Scenario: Confirming
- **WHEN** the player confirms the settings
- **THEN** a new game begins under exactly those settings

### Requirement: The setup screen offers the time control

The setup screen SHALL offer the initial time and the increment as two separate settings, and SHALL allow an untimed game to be chosen.

Each SHALL offer a set of common values to choose between, and SHALL allow a value outside that set to be entered. An entered value that is not a time SHALL be rejected with an explanation rather than accepted as something else.

When an untimed game is chosen, the increment setting SHALL be shown as inapplicable rather than removed, so the screen does not change shape as it is used.

#### Scenario: Choosing a time
- **WHEN** the player changes the time setting
- **THEN** it moves through the offered values, and the screen shows which is currently chosen

#### Scenario: Choosing an increment
- **WHEN** the player changes the increment setting
- **THEN** it moves through the offered values independently of the time setting

#### Scenario: A value not offered
- **WHEN** the player chooses to enter their own time or increment
- **THEN** they may type one, and the game starts with it

#### Scenario: An entered value that is not a time
- **WHEN** the player enters something that is not a valid amount of time
- **THEN** it is rejected with an explanation and the setup screen stays open with the settings untouched

#### Scenario: Untimed
- **WHEN** the player chooses an untimed game
- **THEN** the increment setting is shown as not applying, and confirming starts a game with no clock

#### Scenario: Defaults
- **WHEN** the setup screen is first opened
- **THEN** each setting already holds a sensible value, so confirming immediately starts a playable game

### Requirement: The setup screen is driven by rows and values

The setup screen SHALL present its settings as a list of rows, one setting per row, with a separate action that starts the game.

Moving between rows and changing the value on the highlighted row SHALL be two distinct gestures, so that changing a setting can never start the game and moving between settings can never change one.

The screen SHALL state how it is driven, in the same one place every other screen does.

#### Scenario: Moving between settings
- **WHEN** the player moves between rows
- **THEN** the highlight moves and no setting's value changes

#### Scenario: Changing a value
- **WHEN** the player changes the value on the highlighted row
- **THEN** only that setting changes, the highlight stays where it is, and the game does not start

#### Scenario: Starting
- **WHEN** the player confirms the start action
- **THEN** the game begins with the settings as shown

#### Scenario: A click only highlights
- **WHEN** the player clicks a row
- **THEN** it is highlighted and nothing is chosen or changed by the click alone

#### Scenario: How to drive it
- **WHEN** the setup screen is shown
- **THEN** the gestures for moving between rows, changing a value, and starting are all stated

### Requirement: The setup screen holds settings it does not yet have

The setup screen SHALL be the place any future per-game setting is asked for — notably whether the opponent is another player at the same keyboard or a computer. Adding such a setting SHALL be the addition of a row, and SHALL NOT require the screen's structure, its gestures, or the way it starts a game to change.

A setting the screen does not offer SHALL NOT be implied by it: the screen SHALL NOT show a row for something that cannot yet be chosen.

#### Scenario: Adding a setting later
- **WHEN** a further per-game setting is introduced
- **THEN** it appears as another row, driven by the same gestures, and every existing row behaves as it did

#### Scenario: Nothing is promised early
- **WHEN** the setup screen is shown
- **THEN** every row on it is a setting that can actually be chosen, with no placeholder or disabled row for a feature that does not exist
