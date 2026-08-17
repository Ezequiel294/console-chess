## MODIFIED Requirements

### Requirement: Initial screen

On startup the system SHALL push the main menu as the initial screen. The game screen SHALL be reached from it, not entered directly.

#### Scenario: Launching the game
- **WHEN** the program starts
- **THEN** the main menu is displayed

#### Scenario: Starting play
- **WHEN** the player chooses to start or resume a game from the menu
- **THEN** the game screen is pushed over the menu

#### Scenario: Leaving a game
- **WHEN** the player leaves a game without quitting the program
- **THEN** the main menu is shown again, with its state intact
