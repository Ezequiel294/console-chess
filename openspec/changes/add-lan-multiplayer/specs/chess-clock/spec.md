> This delta builds on the `chess-clock` capability introduced by `add-chess-clock`, which must land first. The MODIFIED blocks below are copied from that change's delta.

## MODIFIED Requirements

### Requirement: Exactly one clock runs at a time

During live play of a timed game, the clock of the side whose turn it is SHALL run and the other side's SHALL be still. The two SHALL never run together, and SHALL never both be stopped while the game is in progress and neither side is between turns — with one exception: while a LAN game is paused by a lost connection, both clocks SHALL be held until play resumes (see lan-play), because time a player cannot use is not time they should lose.

A clock SHALL run against elapsed real time, and SHALL NOT depend on the player producing input: time passes while the player thinks, and the displayed time SHALL follow it without a keystroke.

Time SHALL be measured from a source that only ever moves forward, so that a change to the machine's wall clock — a time zone change, a clock correction — SHALL NOT add time to a player's clock or take it away.

#### Scenario: Time passes without input
- **WHEN** the player to move touches nothing for ten seconds
- **THEN** their clock has counted down by about ten seconds and their opponent's is unchanged

#### Scenario: The display keeps up
- **WHEN** a clock is running
- **THEN** the number on screen changes as the time does, with no keystroke needed to refresh it

#### Scenario: The other clock is still
- **WHEN** one side's clock is running
- **THEN** the other side's reading does not change at all

#### Scenario: The system clock changes
- **WHEN** the machine's wall clock jumps forward or backward during a game
- **THEN** neither player's remaining time is affected

#### Scenario: Disconnection holds both clocks
- **WHEN** a timed LAN game loses its connection
- **THEN** both clocks hold their values until play resumes, and resume from exactly those values

### Requirement: The handover is the clock press

In a pass-and-play game, the turn handover SHALL act as the press of a chess clock. The player who has just moved SHALL keep spending their own time until the handover gesture is made; that gesture SHALL, in one step, add the mover's increment to the mover's clock, start the incoming player's clock, and flip the board.

The handover SHALL remain in an untimed pass-and-play game, where it does nothing to any clock and exists only so a player can see their move before the board turns.

A move that ends the game SHALL NOT require a handover: the game is over, so both clocks stop at the move rather than waiting for a press that would start nobody.

A LAN game has no handover; there, completing the move is the press (see In a LAN game the move is the press).

#### Scenario: Time until the press
- **WHEN** a player completes a move in a pass-and-play game and does not immediately hand over
- **THEN** their own clock keeps counting down and the opponent's stays still

#### Scenario: The press
- **WHEN** the handover gesture is made
- **THEN** the mover's increment is added to the mover's clock, the incoming player's clock starts, and the board flips — all at that moment

#### Scenario: Handover in an untimed game
- **WHEN** a move completes in an untimed pass-and-play game
- **THEN** the handover still waits for the incoming player exactly as before, and no clock is involved

#### Scenario: A move that ends the game
- **WHEN** a move delivers checkmate, stalemate, or any other conclusion
- **THEN** both clocks stop at once and no handover is asked for

#### Scenario: The first turn
- **WHEN** a timed game opens on the board
- **THEN** the side to move is already running, with no press needed to begin

#### Scenario: Resuming a loaded game
- **WHEN** a saved timed game is loaded and appears on the board
- **THEN** the side to move resumes running from the time they had left, with no press needed

## ADDED Requirements

### Requirement: In a LAN game the move is the press

In a timed LAN game, completing a move SHALL do what the handover's press does in pass-and-play: add the mover's increment and end the mover's turn, in that one step — there is no separate gesture. The local player's clock SHALL start when the opponent's move appears on their board, whether or not they are looking, exactly as a pressed clock runs against an absent player at a real board.

#### Scenario: The move is the press
- **WHEN** a player completes a move in a timed LAN game
- **THEN** their increment is applied and their spending of time ends at that moment, with no further gesture

#### Scenario: An arriving move starts the clock
- **WHEN** the opponent's move arrives while the local player is away from the keyboard
- **THEN** the local player's clock is already running when they return

### Requirement: Each machine times its own player

In a timed LAN game, the time a move took SHALL be measured on the machine where it was played and carried with the move, and the receiving machine SHALL charge exactly that measured time — so network delay costs neither player anything. Between moves, the opponent's running clock on the local screen is an estimate, and SHALL be corrected to the reported truth when the move arrives; the correction SHALL be at most the network's delay, never a player's thinking time.

A flag SHALL fall where the clock runs: each machine declares its own player's time expired and reports it, ending the game on both machines as a loss on time — the same ending, reasons and exceptions included, that running out of time already has.

#### Scenario: Latency costs nothing
- **WHEN** a move takes ten seconds of thought and one second to travel
- **THEN** ten seconds come off the mover's clock, not eleven

#### Scenario: The estimate snaps to the truth
- **WHEN** the opponent's move arrives
- **THEN** the opponent's clock shows their reported remaining time, and any difference from the estimate is no more than the delivery delay

#### Scenario: Own flag, own machine
- **WHEN** a player's clock reaches zero on their own machine
- **THEN** their machine declares the flag and both machines end the game as their loss on time, under the same insufficient-material exception as any flag fall
