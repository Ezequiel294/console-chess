## Purpose

Lets an interrupted LAN match continue: every unfinished LAN game is kept automatically in a local rejoin cache, listed for both its players, and resumable by its ID — and every dead match has a way out, by deletion or by resigning it without connecting.

## ADDED Requirements

### Requirement: An unfinished LAN game is cached automatically

Throughout a LAN game, the system SHALL keep an up-to-date local record of the match — enough to resume it: the position, the moves, the clocks as of the last completed move, the match ID, the local player's role and color, and the terms. The record SHALL be current as of the last completed move without either player doing anything.

This cache is not a saved game. It SHALL NOT appear in Load Game, SHALL NOT be openable for review or replay while the game is unfinished, and SHALL be removed when its game finishes or is deleted. It exists so the match can be rejoined, and for nothing else.

The explicit-saving principle (see game-persistence) is not weakened by this: the cache can never silently reopen a finished game as live, because a finished game has no cache.

#### Scenario: The cache keeps up
- **WHEN** any move completes in a LAN game
- **THEN** the local record reflects it, with no action from either player

#### Scenario: Invisible to Load Game
- **WHEN** the player opens Load Game while LAN games are unfinished
- **THEN** none of them is listed there

#### Scenario: Finishing removes the cache
- **WHEN** a LAN game ends by any means
- **THEN** its rejoin record is removed, and only an explicit save from the result screen keeps the game

### Requirement: Ongoing LAN games are listed for rejoining

"Join an ongoing LAN game" SHALL list every unfinished LAN game this machine took part in — hosted and joined alike — showing enough to tell them apart: when it was last played, the match ID, the local player's color, and how many moves it has. Choosing one SHALL rejoin it in the role this machine originally had: the original host hosts again, the original joiner searches again, with neither player needing to remember which they were.

#### Scenario: Both roles in one list
- **WHEN** this machine has an unfinished match it hosted and another it joined
- **THEN** both appear in the same list, and choosing either resumes it correctly

#### Scenario: Rejoining resumes the role
- **WHEN** a player picks an ongoing game from the list
- **THEN** their machine takes the same role it had — the ID answers again on the host's side, the search runs again on the joiner's — without asking

### Requirement: A rejoined match continues where it stopped

When both players have rejoined a match, play SHALL continue from exactly where it stopped: same position, same turn, and in a timed game the clocks resuming from where the pause left them. Until the other player arrives, the rejoined player SHALL see the same "waiting for the other player" message a new match shows, with its ID, and may leave again freely.

If the two machines' records disagree by the final move — one side moved and the other never received it — rejoining SHALL reconcile them so both hold the longer, verified game before play resumes. If the records cannot be reconciled into one game, the rejoin SHALL be refused with an explanation rather than played out as two different games.

#### Scenario: Second player arrives
- **WHEN** both players have rejoined the same match
- **THEN** the board shows the position as it stood, it is the same side's turn, and a timed game's clocks hold the values they had when play stopped

#### Scenario: Waiting for the other player
- **WHEN** one player rejoins and the other has not yet
- **THEN** they see the waiting message with the match ID, and can leave again without consequence

#### Scenario: One machine is a move ahead
- **WHEN** one player's record holds a final move the other's does not
- **THEN** after rejoining, both machines hold the game including that move, verified like any other

### Requirement: An ongoing game can be deleted

From the ongoing list, a game SHALL be deletable after confirmation. Deleting abandons the match on this machine only: the rejoin record is removed, no result is recorded, nothing appears in Load Game, and the other player's machine is unaffected — their copy remains ongoing until they act on it themselves.

#### Scenario: Deleting
- **WHEN** the player deletes an ongoing game and confirms
- **THEN** it leaves the list, its record is gone, and no game or result exists for it on this machine

#### Scenario: Deletion is local
- **WHEN** one player deletes an ongoing match
- **THEN** the other player's list still shows it, exactly as before

### Requirement: An ongoing game can be resigned without connecting

From the ongoing list, a game SHALL also offer resign-and-delete: after a confirmation naming what is being given up, the match ends on this machine as a loss by resignation for the local player, without connecting or waiting. The finished game SHALL then be offered for saving under the standard save-and-name flow, and the rejoin record SHALL be removed either way.

The absent opponent does not receive this resignation: their copy stays ongoing until they too delete it or resign it. That is inherent to resigning while the other machine is unreachable, and SHALL NOT be presented as anything else.

#### Scenario: Resigning from the list
- **WHEN** the player chooses resign-and-delete on an ongoing game and confirms
- **THEN** the game is recorded on this machine as their loss by resignation, they are offered to save and name it, and it leaves the ongoing list

#### Scenario: Saved like any finished game
- **WHEN** the player saves after resigning from the list
- **THEN** the save appears in Load Game as a finished game and opens as a replay

#### Scenario: The confirmation opens safe
- **WHEN** the resign-and-delete confirmation is shown
- **THEN** the option that does nothing is highlighted, as on every destructive choice

### Requirement: A finished LAN game is kept only by choice

When a LAN game ends, each machine SHALL show the standard result screen and offer the standard save — each player deciding independently, on their own machine, whether to keep the game. A kept game SHALL be a normal finished save: named by its player, listed in Load Game, and opened as a replay.

#### Scenario: Independent decisions
- **WHEN** a LAN game finishes and one player saves while the other does not
- **THEN** the game exists as a finished save on the first machine, and nowhere on the second

#### Scenario: The saved game replays
- **WHEN** a player saves a finished LAN game and later chooses it in Load Game
- **THEN** it opens as a replay, exactly as any finished save does
