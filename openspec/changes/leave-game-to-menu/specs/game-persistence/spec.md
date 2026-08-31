## MODIFIED Requirements

### Requirement: Saving is explicit

The system SHALL save a game only when the player asks it to — with the save command during play, by choosing to save when leaving a game, or by choosing to save a finished game from the result screen — never automatically and never on a fixed schedule. Saving an opening position with no moves played SHALL be refused, since there is nothing in it worth keeping.

This now covers finished games as well as games in progress. A finished game is worth keeping — it is the only kind that can be reviewed move by move — but keeping it is still the player's decision, made on the screen that tells them the game is over.

The three moments are unchanged in number and in character by a game being left to the main menu rather than to the shell: what is written, and what is refused, does not depend on where the player goes next.

"Pass-and-play" is said explicitly because a mode where the game does not live on this machine alone would have nothing to offer here — see add-lan-multiplayer, which relies on this qualifier to keep the save question out of a LAN game. Every game today is a pass-and-play game, so the qualifier narrows nothing that currently exists.

#### Scenario: Save command
- **WHEN** the player saves during a game
- **THEN** the game is written to disk and play continues, unaffected

#### Scenario: No moves to save
- **WHEN** the player tries to save before any move has been played
- **THEN** the save is refused with an explanation, and no file is written

#### Scenario: Quitting offers a save
- **WHEN** the player leaves a pass-and-play game in progress
- **THEN** they are offered the choice to save before leaving, leave without saving, or cancel

#### Scenario: No unsolicited prompting
- **WHEN** a game runs for any number of moves without the player saving
- **THEN** the player is never prompted to save outside of leaving the game

#### Scenario: Saving a finished game
- **WHEN** the player chooses to save from the result screen
- **THEN** the finished game is written to disk and the result screen remains

#### Scenario: A finished game not saved
- **WHEN** the player leaves the result screen without choosing to save
- **THEN** nothing is written, and the game is gone

#### Scenario: A game saved on the way out
- **WHEN** the player chooses to save while leaving a game, and later loads it
- **THEN** it is the same file the save command would have written at that moment, with the same name and the same contents
