> This delta builds on the `new-game-setup` capability introduced by `add-chess-clock`, which must land first. The MODIFIED block below is copied from that change's delta.

## MODIFIED Requirements

### Requirement: New Game opens a setup screen

Choosing to start a new game SHALL open a setup screen rather than a board. The game SHALL begin only when the player confirms the settings on it — immediately for a pass-and-play game, and for a LAN game once an opponent has joined and agreed (see lan-match): confirming a LAN game leads to the match ID and the waiting message, not yet to a board.

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
- **WHEN** the player confirms the settings for a pass-and-play game
- **THEN** a new game begins under exactly those settings

#### Scenario: Confirming a LAN game
- **WHEN** the player confirms the settings with LAN mode chosen
- **THEN** no board appears yet: the match is created and the screen waits for an opponent, showing the match ID

## ADDED Requirements

### Requirement: The setup screen offers the mode

The setup screen SHALL offer the game's mode as its first row: pass-and-play (both players at this keyboard) or LAN (an opponent joining from another machine). The rows below SHALL follow the mode — the time rows are offered in both, the color row only in LAN mode — and switching modes SHALL keep the time settings as they were, so trying both modes costs nothing.

#### Scenario: Two modes
- **WHEN** the setup screen is shown
- **THEN** a mode row offers pass-and-play and LAN, driven by the same gestures as every other row

#### Scenario: The rows follow the mode
- **WHEN** the player switches the mode row to LAN
- **THEN** the color row appears below the time rows; switching back removes it, and the time settings are unchanged either way

### Requirement: LAN mode asks which color the host plays

In LAN mode the setup screen SHALL offer a color row: White, Black, or Random. The host plays the chosen color, the joiner the other; Random is decided only once an opponent agrees to the terms (see lan-match).

#### Scenario: Choosing a color
- **WHEN** the player moves through the color row's values
- **THEN** it offers White, Black, and Random, and the chosen value is what the joiner is later shown
