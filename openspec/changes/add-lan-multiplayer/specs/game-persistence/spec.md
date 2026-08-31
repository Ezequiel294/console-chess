## MODIFIED Requirements

### Requirement: Saving is explicit

The system SHALL save a game only when the player asks it to — with the save command during play, by choosing to save when quitting, or by choosing to save a finished game from the result screen — never automatically and never on a fixed schedule. Saving an opening position with no moves played SHALL be refused, since there is nothing in it worth keeping.

This now covers finished games as well as games in progress. A finished game is worth keeping — it is the only kind that can be reviewed move by move — but keeping it is still the player's decision, made on the screen that tells them the game is over.

The LAN rejoin cache (see lan-resume) is the one automatic record, and it is not a save: it never appears in Load Game, cannot be opened for review, and is removed the moment its game finishes. The danger this requirement exists to prevent — an automatic record silently reopening a *finished* game as if it were live — cannot arise from it, because a finished game has no cache. In a LAN game the save command does not exist during play (see lan-play); saving a LAN game is offered where every finished game's save is offered, on the result screen.

#### Scenario: Save command
- **WHEN** the player saves during a pass-and-play game
- **THEN** the game is written to disk and play continues, unaffected

#### Scenario: No moves to save
- **WHEN** the player tries to save before any move has been played
- **THEN** the save is refused with an explanation, and no file is written

#### Scenario: Quitting offers a save
- **WHEN** the player quits a pass-and-play game in progress
- **THEN** they are offered the choice to save before quitting, quit without saving, or cancel

#### Scenario: No unsolicited prompting
- **WHEN** a game runs for any number of moves without the player saving
- **THEN** the player is never prompted to save outside of quitting

#### Scenario: Saving a finished game
- **WHEN** the player chooses to save from the result screen
- **THEN** the finished game is written to disk and the result screen remains

#### Scenario: A finished game not saved
- **WHEN** the player leaves the result screen without choosing to save
- **THEN** nothing is written, and the game is gone

#### Scenario: The cache is not a save
- **WHEN** a LAN game is in progress or interrupted
- **THEN** its automatic rejoin record appears nowhere in Load Game, and the only way the game ever becomes a saved game is an explicit save from the result screen after it finishes
