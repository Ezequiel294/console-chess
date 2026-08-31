## MODIFIED Requirements

### Requirement: Main menu

The system SHALL open on a main menu offering: a new game, joining a LAN game, loading a saved game, how to play, settings, and quitting — in that order, so joining sits between starting a game and loading one.

Joining a LAN game SHALL open a choice between joining a new LAN game (by entering its match ID) and joining an ongoing one (from the list of this machine's unfinished LAN games — see lan-resume).

There is deliberately no "resume last game": see game-persistence's Status note — an automatic resume risked silently reopening a *finished* game as if it were still in progress. Every saved game, including one closed mid-play, is reached the same way: Load Game. Unfinished LAN games are the one exception in spirit and none in practice: they are reached through Join LAN Game, never through Load Game, and rejoining one can never present a finished game as live, because finishing removes it (see lan-resume).

#### Scenario: Launching
- **WHEN** the program starts
- **THEN** the main menu is shown

#### Scenario: Returning to the menu
- **WHEN** the player leaves a game without quitting the program
- **THEN** the main menu is shown again

#### Scenario: Join LAN Game
- **WHEN** the player chooses Join LAN Game
- **THEN** they are offered joining a new game by ID or an ongoing game from the list

### Requirement: Turn handover

In a pass-and-play game — both players at one keyboard — the system SHALL wait after every completed move for the incoming player to indicate they are ready before the board flips, rather than changing after a fixed delay. This handover is unconditional in pass-and-play — there is no setting to skip it — and in a timed game the same gesture is the clock press (see chess-clock).

A LAN game has no handover and no flip: each machine faces its own player throughout, and an opponent's move appears when it arrives (see lan-play). The handover exists because two players share one screen; it does not outlive that reason.

#### Scenario: Handover
- **WHEN** a move completes in a pass-and-play game
- **THEN** the completed move stays visible until the next player signals readiness
- **AND** the board then flips

#### Scenario: No timed wait
- **WHEN** a player takes any amount of time between turns in a pass-and-play game
- **THEN** nothing changes on screen until they act

#### Scenario: No handover over the network
- **WHEN** a move completes in a LAN game
- **THEN** neither player is asked for a readiness gesture, and neither board flips

### Requirement: Commands replace prompting

In-game actions SHALL be invoked by the player through single-key commands. The system SHALL NOT interrupt play to ask questions the player did not initiate — with one deliberate exception: in a LAN game, something the *other player* initiated (a draw offer, a resignation, a disconnection) SHALL be allowed to reach this player's screen as a modal, because it is a person asking, not the system, and a question from an opponent who cannot be seen must not go unnoticed (see lan-play).

The available commands during a live pass-and-play game SHALL be: save, view history, resign, offer a draw, help, and quit. In a live LAN game they SHALL be the same minus save (see lan-play's The LAN command set has no save and no handover).

There is deliberately no command to flip the board *during a live game*. In pass-and-play the turn handover already turns it, and a manual flip only lets the side to move study the position from their opponent's seat — which is not something either player is entitled to mid-game. In a LAN game each player already permanently has their own seat. A replay is the other case entirely: there is no turn, no handover, and no opponent to gain an advantage over, so a replay does have a flip command (see game-replay).

Undo and redo are likewise not live-play commands: chess does not allow taking back a move already made. They are reachable only in a replay of a finished game, where stepping backward changes nothing that was played (see move-undo and game-replay).

#### Scenario: Command issued
- **WHEN** the player presses a command key during their turn
- **THEN** that action is taken without disturbing the position

#### Scenario: No unsolicited prompts
- **WHEN** a game runs for any number of moves
- **THEN** no prompt appears that a player did not trigger — either player, in a LAN game

#### Scenario: Command during opponent's turn
- **WHEN** a command that does not alter the position is issued at any point in a turn
- **THEN** it works, and the side to move is unchanged afterwards

#### Scenario: No manual flip
- **WHEN** the player looks for a way to turn the board mid-turn in a live game
- **THEN** there is none; in pass-and-play the orientation changes only at the handover, and in a LAN game it never changes

#### Scenario: No undo in a live game
- **WHEN** the player presses the undo or redo key during a live game
- **THEN** nothing happens, and neither is listed among the live game's commands

### Requirement: Quitting offers to save

Quitting SHALL require a choice when a game is in progress. In a pass-and-play game the choice is: save and quit, quit without saving, or cancel — since there is no automatic save, this is the moment losing an in-progress game becomes a deliberate choice rather than an accident. In a LAN game the choice is to leave or cancel, and leaving SHALL state that the game stays open and rejoinable by its ID — the rejoin cache already preserves it, so there is nothing to lose and no save question to ask (see lan-play, lan-resume).

#### Scenario: Quit mid-game
- **WHEN** the player quits during a pass-and-play game
- **THEN** they are asked whether to save and quit, quit without saving, or cancel

#### Scenario: Quit a LAN game
- **WHEN** the player quits during a live LAN game
- **THEN** they are asked to confirm leaving, told the game remains rejoinable, and offered no save

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
- **WHEN** the player quits a pass-and-play game before any move has been played
- **THEN** there is nothing worth saving, and the choice is simply to quit or cancel
